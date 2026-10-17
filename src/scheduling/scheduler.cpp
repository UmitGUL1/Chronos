#include "chronos/scheduler.hpp"
#include <tuple>
#include <stdexcept>

namespace chronos {
namespace {
bool runnable(const Job& job) { return job.state == State::Ready || job.state == State::Running; }
bool keepCurrent(std::span<const Job> jobs, std::optional<JobId> current) {
    for (const auto& job : jobs) if (current == job.id && runnable(job)) return true;
    return false;
}
template <typename Key>
std::optional<JobId> bestJob(std::span<const Job> jobs, Key key) {
    const Job* best = nullptr;
    for (const auto& job : jobs) {
        if (runnable(job) && (!best || key(job) < key(*best))) best = &job;
    }
    return best ? std::optional{best->id} : std::nullopt;
}
auto fifoKey(const Job& j) { return std::tuple{j.readyOrder,j.id}; }

class FcfsScheduler final : public IScheduler {
public:
    std::optional<JobId> select(std::span<const Job> jobs, std::optional<JobId> current) const override {
        return keepCurrent(jobs,current) ? current : bestJob(jobs,fifoKey);
    }
};
class SjfScheduler final : public IScheduler {
public:
    std::optional<JobId> select(std::span<const Job> jobs, std::optional<JobId> current) const override {
        if (keepCurrent(jobs,current)) return current;
        return bestJob(jobs,[](const Job& j){return std::tuple{j.executed+j.remaining,j.readyOrder,j.id};});
    }
};
class SrtfScheduler final : public IScheduler {
public:
    std::optional<JobId> select(std::span<const Job> jobs, std::optional<JobId>) const override {
        return bestJob(jobs,[](const Job& j){return std::tuple{j.remaining,j.readyOrder,j.id};});
    }
};
class RoundRobinScheduler final : public IScheduler {
public:
    std::optional<JobId> select(std::span<const Job> jobs, std::optional<JobId>) const override {
        // The engine consumes quantum and requeues only on expiration/blocking.
        return bestJob(jobs,fifoKey);
    }
};
class FixedPriorityScheduler final : public IScheduler {
public:
    std::optional<JobId> select(std::span<const Job> jobs, std::optional<JobId>) const override {
        return bestJob(jobs,[](const Job& j){
            return std::tuple{-static_cast<Tick>(j.effectivePriority),j.readyOrder,j.id};
        });
    }
};
class EdfScheduler final : public IScheduler {
public:
    std::optional<JobId> select(std::span<const Job> jobs, std::optional<JobId>) const override {
        return bestJob(jobs,[](const Job& j){return std::tuple{j.deadline,j.readyOrder,j.id};});
    }
};
}
std::unique_ptr<IScheduler> makeScheduler(Policy policy) {
    switch(policy) {
    case Policy::Fcfs: return std::make_unique<FcfsScheduler>();
    case Policy::Sjf: return std::make_unique<SjfScheduler>();
    case Policy::Srtf: return std::make_unique<SrtfScheduler>();
    case Policy::RoundRobin: return std::make_unique<RoundRobinScheduler>();
    // RMS assigns static period-based ranks on release, then uses this policy.
    case Policy::FixedPriority:
    case Policy::Rms: return std::make_unique<FixedPriorityScheduler>();
    case Policy::Edf: return std::make_unique<EdfScheduler>();
    }
    throw std::invalid_argument("Unknown scheduling policy");
}
}

