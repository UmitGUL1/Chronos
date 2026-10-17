#include "chronos/model.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include <utility>

namespace chronos {
std::string toString(Policy p) {
    switch (p) {
    case Policy::Fcfs: return "fcfs";
    case Policy::Sjf: return "sjf";
    case Policy::Srtf: return "srtf";
    case Policy::RoundRobin: return "rr";
    case Policy::FixedPriority: return "fixed";
    case Policy::Rms: return "rms";
    case Policy::Edf: return "edf";
    }
    throw std::invalid_argument("Unknown scheduling policy");
}
Policy parsePolicy(const std::string& name) {
    for (auto p : {Policy::Fcfs, Policy::Sjf, Policy::Srtf, Policy::RoundRobin,
                   Policy::FixedPriority, Policy::Rms, Policy::Edf})
        if (name == toString(p)) return p;
    throw std::invalid_argument("Unsupported scheduling policy: " + name);
}
std::string toString(State state) {
    switch (state) {
    case State::Ready: return "ready";
    case State::Running: return "running";
    case State::Blocked: return "blocked";
    case State::Completed: return "completed";
    case State::Terminated: return "terminated";
    }
    throw std::invalid_argument("Unknown state");
}
void validate(const Scenario& s) {
    const auto require = [](bool value, const std::string& message) {
        if (!value) throw std::invalid_argument(message);
    };
    require(s.horizon > 0 && s.horizon <= maxTime, "horizon must be in [1, 1e12]");
    require(s.quantum > 0 && s.quantum <= maxTime, "quantum must be positive and bounded");
    require(s.tasks.size() <= 10000 && s.interrupts.size() <= 100000,
            "Scenario exceeds task/interrupt limits");
    require(s.resources.size() <= 10000 && s.masks.size() <= 100000,
            "Scenario exceeds resource/mask limits");
    require(s.watchdog.heartbeats.size() <= 100000, "Too many heartbeat events");
    require(!s.inheritance || s.policy == Policy::FixedPriority || s.policy == Policy::Rms,
            "Priority inheritance requires fixed-priority or RMS scheduling");
    std::set<std::string> ids;
    for (const auto& r : s.resources)
        require(!r.empty() && ids.insert(r).second, "Duplicate/empty resource ID");
    const auto resourceIds = ids;
    ids.clear();
    std::uint64_t count = 0;
    for (const auto& t : s.tasks) {
        require(!t.id.empty() && ids.insert(t.id).second, "Duplicate/empty task ID");
        require(t.execution > 0 && t.execution <= maxTime, "execution must be positive and bounded");
        require(t.period >= 0 && t.period <= maxTime, "period must be nonnegative and bounded");
        require(t.deadline > 0 && t.deadline <= maxTime, "deadline must be positive and bounded");
        require(t.release >= 0 && t.release <= s.horizon, "release outside simulation horizon");
        require(s.policy != Policy::Rms || t.period > 0, "RMS requires periodic tasks");
        count += t.release < s.horizon ?
            (t.period ? static_cast<std::uint64_t>((s.horizon - 1 - t.release) / t.period + 1) : 1) : 0;
        require(count <= 100000, "Scenario would release more than 100000 jobs");
        require(t.actions.size() <= 10000, "Too many task actions");
        Tick previous = -1;
        std::set<std::string> held;
        for (const auto& a : t.actions) {
            require(a.offset >= 0 && a.offset <= t.execution && a.offset >= previous,
                    "Action offsets must be ordered and within execution budget");
            previous = a.offset;
            if (a.kind == ActionKind::Heartbeat) continue;
            require(resourceIds.contains(a.resource), "Unknown resource: " + a.resource);
            if (a.kind == ActionKind::Lock)
                require(held.insert(a.resource).second, "Nonrecursive mutex locked twice");
            else
                require(held.erase(a.resource) == 1, "Unlock without preceding lock");
        }
    }
    ids.clear();
    for (const auto& irq : s.interrupts) {
        require(!irq.id.empty() && ids.insert(irq.id).second, "Duplicate/empty interrupt ID");
        require(irq.arrival >= 0 && irq.arrival < s.horizon, "Interrupt arrival outside horizon");
        require(irq.duration > 0 && irq.duration <= maxTime, "Invalid interrupt duration");
    }
    for (const auto& mask : s.masks)
        require(mask.start >= 0 && mask.end > mask.start && mask.end <= s.horizon,
                "Invalid half-open interrupt mask window");
    require(!s.watchdog.enabled || (s.watchdog.timeout > 0 && s.watchdog.timeout <= maxTime),
            "Invalid watchdog timeout");
    for (auto time : s.watchdog.heartbeats)
        require(time >= 0 && time < s.horizon, "Heartbeat outside horizon");
}
Analysis analyze(const Scenario& s, Policy policy) {
    Analysis a;
    const auto n = s.tasks.size();
    a.rmsBound = n ? static_cast<double>(n) * (std::pow(2.0, 1.0 / static_cast<double>(n)) - 1.0) : 1.0;
    a.assumptions = "Single CPU; independent preemptible periodic tasks; D=T; no locks, interrupts, watchdog recovery or overhead.";
    a.applicable = !s.tasks.empty() && s.resources.empty() && s.interrupts.empty() &&
                   !s.watchdog.enabled && (policy == Policy::Rms || policy == Policy::Edf);
    for (const auto& t : s.tasks) {
        if (t.period > 0) a.utilization += static_cast<double>(t.execution) / static_cast<double>(t.period);
        if (t.period <= 0 || t.deadline != t.period || !t.actions.empty()) a.applicable = false;
    }
    if (!a.applicable) a.result = "Not applicable: assumptions are not satisfied.";
    else if (policy == Policy::Rms)
        a.result = a.utilization <= a.rmsBound + 1e-12 ?
            "Pass: sufficient RMS utilization bound." : "Inconclusive: sufficient RMS bound exceeded.";
    else a.result = a.utilization <= 1.0 + 1e-12 ?
        "Feasible under stated EDF assumptions (U <= 1)." : "Infeasible under stated EDF assumptions (U > 1).";
    return a;
}
} // namespace chronos

