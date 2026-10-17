#pragma once
#include <string_view>

namespace chronos {
// Build metadata. Simulation behavior is implemented in SimulationEngine.
[[nodiscard]] std::string_view version() noexcept;
[[nodiscard]] std::string_view description() noexcept;
}
