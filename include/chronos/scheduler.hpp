#pragma once
#include "chronos/model.hpp"
#include <memory>
#include <span>

namespace chronos {
class IScheduler {
public:
    virtual ~IScheduler() = default;
    // The engine owns jobs, readiness and time. Policies only select.
    [[nodiscard]] virtual std::optional<JobId> select(
        std::span<const Job> jobs, std::optional<JobId> current) const = 0;
};
[[nodiscard]] std::unique_ptr<IScheduler> makeScheduler(Policy policy);
}
