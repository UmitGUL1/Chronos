#include "chronos/engine.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>

namespace chronos {
SimulationEngine::SimulationEngine(Scenario scenario)
    : scenario_(std::move(scenario)), scheduler_(makeScheduler(scenario_.policy)),
      quantumRemaining_(scenario_.quantum) {
    validate(scenario_);
    for (const auto& id : scenario_.resources) resources_.emplace(id, ResourceView{id, {}, {}});
    for (std::size_t i = 0; i < scenario_.tasks.size(); ++i)
        if (scenario_.tasks[i].release < scenario_.horizon)
            enqueue(scenario_.tasks[i].release, EventKind::Release, i);
    for (std::size_t i = 0; i < scenario_.interrupts.size(); ++i) {
        const auto& source = scenario_.interrupts[i];
        interrupts_.push_back({source.id, source.arrival, source.duration,
                               source.duration, source.priority, "future", {}});
        enqueue(source.arrival, EventKind::InterruptArrival, i);
    }
    for (const auto& mask : scenario_.masks) {
        enqueue(mask.start, EventKind::MaskBoundary, 0);
        enqueue(mask.end, EventKind::MaskBoundary, 0);
    }
    if (scenario_.watchdog.enabled) {
        enqueue(scenario_.watchdog.timeout, EventKind::Watchdog, watchdogGeneration_);
        for (auto time : scenario_.watchdog.heartbeats) enqueue(time, EventKind::Heartbeat, 0);
    }
}
Job& SimulationEngine::job(JobId id) {
    if (id == 0 || id > jobs_.size()) throw std::logic_error("Invalid job ID");
    return jobs_.at(static_cast<std::size_t>(id - 1));
}
void SimulationEngine::enqueue(Tick time, EventKind kind, std::uint64_t payload) {
    if (time < now_) throw std::logic_error("Event scheduled in the past");
    if (time <= scenario_.horizon) queue_.push({time, kind, sequence_++, payload});
}
void SimulationEngine::record(std::string kind, std::string detail, std::optional<JobId> id) {
    if (events_.size() >= 2'000'000) throw std::runtime_error("Event recording limit reached (2000000)");
    events_.push_back({now_, std::move(kind), std::move(detail), id});
}
void SimulationEngine::releaseTask(std::size_t index) {
    const auto& task = scenario_.tasks.at(index);
    int priority = task.priority;
    if (scenario_.policy == Policy::Rms) {
        // Equal periods share a rank; stable ready order breaks ties.
        priority = 1;
        for (const auto& other : scenario_.tasks) if (other.period > task.period) ++priority;
    }
    Job instance;
    instance.id = jobs_.size() + 1;
    instance.task = index;
    instance.release = now_;
    instance.deadline = now_ + task.deadline; // bounded inputs keep sum <= 2e12
    instance.remaining = task.execution;
    instance.basePriority = instance.effectivePriority = priority;
    instance.readyOrder = readySequence_++;
    jobs_.push_back(instance);
    record("release", task.id, instance.id);
    enqueue(instance.deadline, EventKind::Deadline, instance.id);
    if (task.period > 0 && now_ + task.period < scenario_.horizon)
        enqueue(now_ + task.period, EventKind::Release, index);
}
bool SimulationEngine::masked() const {
    return std::any_of(scenario_.masks.begin(), scenario_.masks.end(), [this](auto mask) {
        return mask.start <= now_ && now_ < mask.end;
    });
}
Tick SimulationEngine::nextBoundary() const {
    Tick next = scenario_.horizon;
    if (!queue_.empty()) next = std::min(next, queue_.top().time);
    if (!interruptStack_.empty())
        next = std::min(next, now_ + interrupts_[interruptStack_.back()].remaining);
    else if (current_) {
        const auto& j = jobs_.at(static_cast<std::size_t>(*current_ - 1));
        next = std::min(next, now_ + j.remaining);
        const auto& task = scenario_.tasks[j.task];
        if (j.nextAction < task.actions.size())
            next = std::min(next, now_ + task.actions[j.nextAction].offset - j.executed);
        if (scenario_.policy == Policy::RoundRobin) next = std::min(next, now_ + quantumRemaining_);
    }
    return next;
}
void SimulationEngine::account(Tick duration) {
    if (duration <= 0) throw std::logic_error("Non-positive event interval");
    Interval interval{now_, now_ + duration, "idle", "IDLE", {}};
    std::string entity;
    if (!interruptStack_.empty()) {
        auto& irq = interrupts_[interruptStack_.back()];
        irq.remaining -= duration;
        interval.kind = "interrupt";
        interval.label = irq.id;
        entity = "irq:" + irq.id;
        counters_.interruptTime += duration;
    } else if (current_) {
        auto& j = job(*current_);
        if (j.state != State::Running) throw std::logic_error("CPU owns a nonrunning job");
        j.remaining -= duration;
        j.executed += duration;
        if (j.remaining < 0) throw std::logic_error("Negative remaining budget");
        if (scenario_.policy == Policy::RoundRobin) quantumRemaining_ -= duration;
        interval.kind = "job";
        interval.label = scenario_.tasks[j.task].id + "#" + std::to_string(j.id);
        interval.job = j.id;
        entity = "job:" + std::to_string(j.id);
        counters_.jobTime += duration;
    } else counters_.idleTime += duration;
    // Count changes between nonidle execution identities; idle does not create a switch.
    if (!entity.empty() && entity != lastEntity_) {
        if (!lastEntity_.empty()) ++counters_.contextSwitches;
        lastEntity_ = entity;
    }
    for (auto& j : jobs_) {
        if (j.state == State::Ready) j.readyTime += duration;
        else if (j.state == State::Blocked) j.blockingTime += duration;
    }
    if (!timeline_.empty() && timeline_.back().end == interval.begin &&
        timeline_.back().kind == interval.kind && timeline_.back().label == interval.label)
        timeline_.back().end = interval.end;
    else {
        if (timeline_.size() >= 2'000'000) throw std::runtime_error("Timeline limit reached");
        timeline_.push_back(std::move(interval));
    }
    now_ += duration;
}
void SimulationEngine::releaseResource(const std::string& name, JobId owner) {
    auto& resource = resources_.at(name);
    if (resource.owner != owner) throw std::logic_error("Unlock attempted by non-owner");
    resource.owner.reset();
    record("unlock", name, owner);
    if (!resource.waiting.empty()) {
        const auto next = resource.waiting.front();
        resource.waiting.erase(resource.waiting.begin());
        resource.owner = next;
        auto& woken = job(next);
        woken.waitingFor.reset();
        woken.state = State::Ready;
        woken.readyOrder = readySequence_++;
        record("wake", name, next);
    }
}
void SimulationEngine::releaseAll(JobId owner) {
    for (auto& [name, resource] : resources_)
        if (resource.owner == owner) releaseResource(name, owner);
}
void SimulationEngine::completeCurrent() {
    if (!current_) return;
    const auto id = *current_;
    auto& j = job(id);
    if (j.remaining != 0) return;
    j.state = State::Completed;
    j.completion = now_;
    ++counters_.completed;
    record("complete", scenario_.tasks[j.task].id, id);
    releaseAll(id);
    current_.reset();
}
void SimulationEngine::heartbeat() {
    if (!scenario_.watchdog.enabled) return;
    ++watchdogGeneration_;
    enqueue(now_ + scenario_.watchdog.timeout, EventKind::Watchdog, watchdogGeneration_);
    record("heartbeat", "subsystem");
}
bool SimulationEngine::actions(JobId id) {
    auto& j = job(id);
    const auto& task = scenario_.tasks[j.task];
    while (j.nextAction < task.actions.size() && task.actions[j.nextAction].offset == j.executed) {
        const auto& action = task.actions[j.nextAction++];
        if (action.kind == ActionKind::Heartbeat) { heartbeat(); continue; }
        auto& resource = resources_.at(action.resource);
        if (action.kind == ActionKind::Unlock) {
            releaseResource(action.resource, id);
        } else if (!resource.owner) {
            resource.owner = id;
            record("lock", action.resource, id);
        } else {
            if (resource.owner == id) throw std::logic_error("Recursive lock");
            resource.waiting.push_back(id);
            j.waitingFor = action.resource;
            j.state = State::Blocked;
            current_.reset();
            record("block", action.resource, id);
            return false;
        }
    }
    return true;
}
void SimulationEngine::donate() {
    std::vector<int> old;
    old.reserve(jobs_.size());
    for (auto& j : jobs_) {
        old.push_back(j.effectivePriority);
        j.effectivePriority = j.basePriority;
    }
    if (scenario_.inheritance) {
        for (std::size_t iteration = 0; iteration < jobs_.size(); ++iteration) {
            bool changed = false;
            for (auto& [name, resource] : resources_) {
                (void)name;
                if (!resource.owner) continue;
                auto& owner = job(*resource.owner);
                for (auto waiter : resource.waiting) {
                    const auto donated = job(waiter).effectivePriority;
                    if (donated > owner.effectivePriority) {
                        owner.effectivePriority = donated;
                        changed = true;
                    }
                }
            }
            if (!changed) break;
        }
    }
    for (std::size_t i = 0; i < jobs_.size(); ++i)
        if (old[i] != jobs_[i].effectivePriority)
            record("priority", std::to_string(old[i]) + " -> " +
                   std::to_string(jobs_[i].effectivePriority), jobs_[i].id);
}
void SimulationEngine::detectDeadlocks() {
    std::map<JobId, JobId> edges;
    for (const auto& j : jobs_)
        if (j.waitingFor && resources_.at(*j.waitingFor).owner)
            edges[j.id] = *resources_.at(*j.waitingFor).owner;
    std::set<JobId> checked;
    for (const auto& [start, unused] : edges) {
        (void)unused;
        if (checked.contains(start)) continue;
        std::vector<JobId> path;
        auto node = start;
        while (edges.contains(node) && !checked.contains(node)) {
            const auto found = std::find(path.begin(), path.end(), node);
            if (found != path.end()) {
                std::vector<JobId> cycle(found, path.end());
                std::rotate(cycle.begin(), std::min_element(cycle.begin(), cycle.end()), cycle.end());
                if (std::none_of(deadlocks_.begin(), deadlocks_.end(), [&](const auto& d) { return d.cycle == cycle; })) {
                    Deadlock d{now_, cycle, {}};
                    for (auto id : cycle) d.resources.push_back(*job(id).waitingFor);
                    deadlocks_.push_back(std::move(d));
                    record("deadlock", "wait-for cycle of " + std::to_string(cycle.size()) + " jobs");
                }
                break;
            }
            path.push_back(node);
            node = edges.at(node);
        }
        checked.insert(path.begin(), path.end());
    }
}
void SimulationEngine::dispatchInterrupt() {
    if (masked()) return; // masks defer new handlers; active handlers continue
    std::optional<std::size_t> best;
    for (std::size_t i = 0; i < interrupts_.size(); ++i) {
        const auto& irq = interrupts_[i];
        if (irq.state != "pending") continue;
        if (!best || std::tuple{-static_cast<Tick>(irq.priority), irq.arrival, i} <
                     std::tuple{-static_cast<Tick>(interrupts_[*best].priority), interrupts_[*best].arrival, *best}) best = i;
    }
    if (!best) return;
    if (!interruptStack_.empty()) {
        auto& active = interrupts_[interruptStack_.back()];
        if (!scenario_.nestedInterrupts || interrupts_[*best].priority <= active.priority) return;
        active.state = "suspended";
        ++counters_.preemptions;
    } else if (current_) {
        auto& j = job(*current_);
        if (j.state == State::Running) {
            j.state = State::Ready;
            ++counters_.preemptions;
            record("preempt", "interrupt", j.id);
        }
    }
    auto& irq = interrupts_[*best];
    irq.state = "active";
    irq.firstService = now_;
    interruptStack_.push_back(*best);
    record("interrupt_start", irq.id);
}
void SimulationEngine::dispatch() {
    donate();
    detectDeadlocks();
    dispatchInterrupt();
    if (!interruptStack_.empty()) return;
    // Each pass consumes at least one action or reaches a stable runnable job.
    for (;;) {
        ++counters_.decisions;
        const auto next = scheduler_->select(jobs_, current_);
        if (next != current_) {
            if (current_) {
                auto& old = job(*current_);
                if (old.state == State::Running) {
                    old.state = State::Ready;
                    ++counters_.preemptions;
                    record("preempt", "scheduler", old.id);
                }
            }
            current_ = next;
            quantumRemaining_ = scenario_.quantum;
        }
        if (!current_) return;
        auto& selected = job(*current_);
        selected.state = State::Running;
        const auto before = selected.nextAction;
        const auto id = selected.id;
        // Attempting a zero-offset lock requires no CPU service; firstDispatch
        // is recorded only once the job can consume a positive interval.
        if (!actions(id)) { donate(); detectDeadlocks(); continue; }
        donate();
        if (selected.remaining == 0) { completeCurrent(); continue; }
        const auto reconsider = scheduler_->select(jobs_, current_);
        if (reconsider != current_ && selected.nextAction != before) continue;
        if (!selected.firstDispatch) selected.firstDispatch = now_;
        return;
    }
}
void SimulationEngine::processBoundary() {
    if (!interruptStack_.empty() && interrupts_[interruptStack_.back()].remaining == 0) {
        auto& irq = interrupts_[interruptStack_.back()];
        irq.state = "completed";
        record("interrupt_complete", irq.id);
        interruptStack_.pop_back();
        if (!interruptStack_.empty()) interrupts_[interruptStack_.back()].state = "active";
    }
    if (interruptStack_.empty() && current_) {
        const auto id = *current_;
        if (job(id).state == State::Running) {
            actions(id);
            if (current_) completeCurrent();
        }
    }
    while (!queue_.empty() && queue_.top().time == now_) {
        const auto e = queue_.top();
        queue_.pop();
        switch (e.kind) {
        case EventKind::Release: releaseTask(static_cast<std::size_t>(e.payload)); break;
        case EventKind::InterruptArrival:
            interrupts_.at(static_cast<std::size_t>(e.payload)).state = "pending";
            record("interrupt_arrival", interrupts_[static_cast<std::size_t>(e.payload)].id);
            break;
        case EventKind::Heartbeat: heartbeat(); break;
        case EventKind::MaskBoundary: record("mask", masked() ? "masked" : "unmasked"); break;
        case EventKind::Deadline: {
            auto& j = job(e.payload);
            ++counters_.evaluatedDeadlines;
            if (j.state != State::Completed && j.state != State::Terminated) {
                j.missed = true;
                ++counters_.deadlineMisses;
                record("deadline_miss", scenario_.tasks[j.task].id, j.id);
            }
            break;
        }
        case EventKind::Watchdog:
            if (e.payload != watchdogGeneration_) break;
            ++counters_.watchdogTimeouts;
            record("watchdog_timeout", scenario_.watchdog.terminateJobs ? "terminate all current jobs" : "alert only");
            if (scenario_.watchdog.terminateJobs) {
                current_.reset();
                for (auto& [name, resource] : resources_) { (void)name; resource.waiting.clear(); }
                for (auto& j : jobs_) {
                    if (j.state == State::Completed || j.state == State::Terminated) continue;
                    j.state = State::Terminated;
                    j.waitingFor.reset();
                    record("terminate", "watchdog", j.id);
                    releaseAll(j.id);
                }
            }
            ++watchdogGeneration_;
            enqueue(now_ + scenario_.watchdog.timeout, EventKind::Watchdog, watchdogGeneration_);
            break;
        }
    }
    if (scenario_.policy == Policy::RoundRobin && current_ && quantumRemaining_ == 0 && interruptStack_.empty()) {
        auto& j = job(*current_);
        if (j.state == State::Running || j.state == State::Ready) {
            if (now_ < scenario_.horizon && std::any_of(jobs_.begin(), jobs_.end(), [&j](const auto& other) {
                return other.id != j.id && other.state == State::Ready;
            })) {
                ++counters_.preemptions;
                record("preempt", "quantum expiration", j.id);
            }
            j.state = State::Ready;
            j.readyOrder = readySequence_++;
            record("quantum", "expired", j.id);
        }
        current_.reset();
        quantumRemaining_ = scenario_.quantum;
    }
    if (now_ == scenario_.horizon) { finished_ = true; donate(); detectDeadlocks(); return; }
    dispatch();
}
bool SimulationEngine::advance(Tick limit) {
    if (finished_) return false;
    if (!started_) { started_ = true; processBoundary(); return true; }
    const auto next = std::min(nextBoundary(), limit);
    if (next <= now_) throw std::logic_error("Zero-time event loop");
    account(next - now_);
    processBoundary();
    return true;
}
bool SimulationEngine::step() { return advance(scenario_.horizon); }
bool SimulationEngine::tick() {
    if (!started_ && !step()) return false;
    if (finished_) return false;
    const auto target = std::min(now_ + 1, scenario_.horizon);
    while (!finished_ && now_ < target) advance(target);
    return true;
}
void SimulationEngine::run(std::stop_token stop) {
    while (!stop.stop_requested() && step()) {}
}
Metrics SimulationEngine::metrics() const {
    auto m = counters_;
    const auto elapsed = static_cast<double>(now_);
    m.utilization = now_ ? static_cast<double>(m.jobTime + m.interruptTime) / elapsed : 0;
    m.throughput = now_ ? static_cast<double>(m.completed) / elapsed : 0;
    m.missRatio = m.evaluatedDeadlines ? static_cast<double>(m.deadlineMisses) / static_cast<double>(m.evaluatedDeadlines) : 0;
    double waiting = 0, turnaround = 0, response = 0;
    std::size_t dispatched = 0, serviced = 0;
    for (const auto& j : jobs_) {
        m.totalBlocking += j.blockingTime;
        if (j.completion) {
            waiting += static_cast<double>(j.readyTime);
            turnaround += static_cast<double>(*j.completion - j.release);
            m.maximumTurnaround = std::max(m.maximumTurnaround, *j.completion - j.release);
        }
        if (j.firstDispatch) {
            const auto r = *j.firstDispatch - j.release;
            response += static_cast<double>(r);
            m.maximumResponse = std::max(m.maximumResponse, r);
            ++dispatched;
        }
    }
    m.averageWaiting = m.completed ? waiting / static_cast<double>(m.completed) : 0;
    m.averageTurnaround = m.completed ? turnaround / static_cast<double>(m.completed) : 0;
    m.averageResponse = dispatched ? response / static_cast<double>(dispatched) : 0;
    for (const auto& irq : interrupts_) if (irq.firstService) {
        m.averageInterruptLatency += static_cast<double>(*irq.firstService - irq.arrival);
        ++serviced;
    }
    if (serviced) m.averageInterruptLatency /= static_cast<double>(serviced);
    return m;
}
Snapshot SimulationEngine::snapshot() const {
    std::vector<ResourceView> resources;
    for (const auto& [name, value] : resources_) { (void)name; resources.push_back(value); }
    return {now_, finished_, jobs_, timeline_, events_, std::move(resources), interrupts_, deadlocks_, metrics()};
}
}
