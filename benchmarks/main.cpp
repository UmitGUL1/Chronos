#include "chronos/engine.hpp"
#include <chrono>
#include <iostream>

int main() {
    std::cout << "jobs,elapsed_ms,events,events_per_second,decisions,us_per_decision_amortized\n";
    for (int n : {10, 100, 1000, 10000}) {
        chronos::Scenario s;
        s.horizon = n + 1;
        for (int i = 0; i < n; ++i)
            s.tasks.push_back({std::to_string(i), "", 1, 0, n + 1, 0, 0, {}});
        const auto start = std::chrono::steady_clock::now();
        chronos::SimulationEngine engine(s);
        engine.run();
        const auto end = std::chrono::steady_clock::now();
        const auto result = engine.snapshot(); // snapshot serialization excluded
        const double seconds = std::chrono::duration<double>(end - start).count();
        std::cout << n << ',' << seconds * 1000 << ',' << result.events.size() << ','
                  << static_cast<double>(result.events.size()) / seconds << ','
                  << result.metrics.decisions << ',' << seconds * 1e6 / static_cast<double>(result.metrics.decisions) << '\n';
    }
}
