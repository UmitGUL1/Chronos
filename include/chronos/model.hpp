#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chronos {
using Tick = std::int64_t;
using JobId = std::uint64_t;
constexpr Tick maxTime = 1'000'000'000'000LL;

enum class Policy { Fcfs, Sjf, Srtf, RoundRobin, FixedPriority, Rms, Edf };
enum class State { Ready, Running, Blocked, Completed, Terminated };
enum class ActionKind { Lock, Unlock, Heartbeat };
std::string toString(Policy policy);
Policy parsePolicy(const std::string& name);
std::string toString(State state);

struct Action {
    Tick offset{}; // Executed CPU ticks, independent of wall clock and blocking.
    ActionKind kind{ActionKind::Lock};
    std::string resource;
};
struct Task {
    std::string id;
    std::string name;
    Tick execution{1};
    Tick period{}; // zero denotes a single aperiodic job
    Tick deadline{1};
    Tick release{};
    int priority{}; // larger numbers have higher priority
    std::vector<Action> actions;
};
struct Interrupt {
    std::string id;
    Tick arrival{};
    Tick duration{1};
    int priority{1};
};
struct MaskWindow { Tick start{}; Tick end{}; };
struct Watchdog {
    bool enabled{};
    Tick timeout{10};
    std::vector<Tick> heartbeats;
    bool terminateJobs{};
};
struct Scenario {
    std::string name{"Untitled"};
    std::string description;
    Policy policy{Policy::Fcfs};
    Tick horizon{50};
    Tick quantum{2};
    bool inheritance{};
    bool nestedInterrupts{true};
    std::vector<Task> tasks;
    std::vector<std::string> resources;
    std::vector<Interrupt> interrupts;
    std::vector<MaskWindow> masks;
    Watchdog watchdog;
};
void validate(const Scenario& scenario);

struct Job {
    JobId id{};
    std::size_t task{};
    Tick release{};
    Tick deadline{};
    Tick remaining{};
    Tick executed{};
    std::optional<Tick> firstDispatch;
    std::optional<Tick> completion;
    State state{State::Ready};
    bool missed{};
    int basePriority{};
    int effectivePriority{};
    std::optional<std::string> waitingFor;
    std::size_t nextAction{};
    std::uint64_t readyOrder{};
    Tick readyTime{};
    Tick blockingTime{};
};
struct Interval {
    Tick begin{};
    Tick end{};
    std::string kind; // job, interrupt, idle
    std::string label;
    std::optional<JobId> job;
};
struct RecordedEvent {
    Tick time{};
    std::string kind;
    std::string detail;
    std::optional<JobId> job;
};
struct ResourceView {
    std::string id;
    std::optional<JobId> owner;
    std::vector<JobId> waiting;
};
struct InterruptView {
    std::string id;
    Tick arrival{};
    Tick duration{};
    Tick remaining{};
    int priority{};
    std::string state{"future"};
    std::optional<Tick> firstService;
};
struct Deadlock {
    Tick time{};
    std::vector<JobId> cycle; // final edge closes to the first job
    std::vector<std::string> resources;
};
struct Metrics {
    Tick jobTime{};
    Tick interruptTime{};
    Tick idleTime{};
    std::uint64_t completed{};
    std::uint64_t deadlineMisses{};
    std::uint64_t evaluatedDeadlines{};
    std::uint64_t contextSwitches{};
    std::uint64_t preemptions{};
    std::uint64_t decisions{};
    std::uint64_t watchdogTimeouts{};
    double utilization{};
    double throughput{};
    double averageWaiting{};
    double averageTurnaround{};
    double averageResponse{};
    Tick maximumResponse{};
    Tick maximumTurnaround{}; // RT literature response = completion - release
    Tick totalBlocking{};
    double averageInterruptLatency{};
    double missRatio{};
};
struct Analysis {
    double utilization{};
    double rmsBound{};
    bool applicable{};
    std::string result;
    std::string assumptions;
};
struct Snapshot {
    Tick time{};
    bool finished{};
    std::vector<Job> jobs;
    std::vector<Interval> timeline;
    std::vector<RecordedEvent> events;
    std::vector<ResourceView> resources;
    std::vector<InterruptView> interrupts;
    std::vector<Deadlock> deadlocks;
    Metrics metrics;
};
Analysis analyze(const Scenario& scenario, Policy policy);
}
