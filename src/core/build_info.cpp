#include "chronos/core/build_info.hpp"

namespace chronos {
std::string_view version() noexcept { return "0.2.0-unverified"; }
std::string_view description() noexcept {
    return "ChronOS: deterministic RTOS concepts simulator (not a production kernel)";
}
}
