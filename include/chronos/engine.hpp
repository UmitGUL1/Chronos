#pragma once
#include "chronos/model.hpp"
#include "chronos/scheduler.hpp"
#include <functional>
#include <map>
#include <queue>
#include <stop_token>
#include <tuple>

namespace chronos {
class SimulationEngine {
public:
    explicit SimulationEngine(Scenario scenario);
    [[nodiscard]] const Scenario& scenario() const noexcept { return scenario_; }
    [[nodiscard]] Snapshot snapshot() const;
    // One timestamp batch (all simultaneous events) is atomic and deterministic.
    bool step();
    bool tick();
    void run(std::stop_token stop = {});
    [[nodiscard]] Tick time() const noexcept { return now_; }
    [[nodiscard]] bool finished() const noexcept { return finished_; }
private:
    enum class EventKind { Release, InterruptArrival, Heartbeat, MaskBoundary, Deadline, Watchdog };
    struct Event {
        Tick time;
        EventKind kind;
        std::uint64_t order;
        std::uint64_t payload;
        bool operator>(const Event& other) const {
            return std::tie(time, kind, order) > std::tie(other.time, other.kind, other.order);
        }
    };
    Scenario scenario_;
    std::unique_ptr<IScheduler> scheduler_;
    std::priority_queue<Event, std::vector<Event>, std::greater<>> queue_;
    Tick now_{};
    bool started_{};
    bool finished_{};
    std::uint64_t sequence_{};
    std::uint64_t readySequence_{};
    std::vector<Job> jobs_;
    std::map<std::string, ResourceView> resources_;
    std::vector<InterruptView> interrupts_;
    std::vector<std::size_t> interruptStack_;
    std::optional<JobId> current_;
    Tick quantumRemaining_{};
    std::vector<Interval> timeline_;
    std::vector<RecordedEvent> events_;
    std::vector<Deadlock> deadlocks_;
    std::string lastEntity_;
    std::uint64_t watchdogGeneration_{};
    Metrics counters_;

    Job& job(JobId id);
    void enqueue(Tick time, EventKind kind, std::uint64_t payload);
    void record(std::string kind, std::string detail, std::optional<JobId> id = {});
    bool advance(Tick limit);
    Tick nextBoundary() const;
    void account(Tick duration);
    void processBoundary();
    void releaseTask(std::size_t task);
    void completeCurrent();
    void releaseResource(const std::string& resource, JobId owner);
    void releaseAll(JobId owner);
    void donate();
    void detectDeadlocks();
    void dispatch();
    bool actions(JobId id);
    bool masked() const;
    void dispatchInterrupt();
    void heartbeat();
    Metrics metrics() const;
};
}
