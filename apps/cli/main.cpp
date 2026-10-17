#include "chronos/core/build_info.hpp"
#include "chronos/engine.hpp"
#include "chronos/io.hpp"
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

int main(int argc, char* argv[]) {
    if (argc == 1 || (argc == 2 && std::string_view(argv[1]) == "--help")) {
        std::cout << chronos::description() << '\n'
                  << "Usage: chronos-cli --scenario FILE [--scheduler fcfs|sjf|srtf|rr|fixed|rms|edf]\n"
                  << "                   [--export NEW_DIRECTORY] [--compare] [--headless]\n"
                  << "       chronos-cli --help | --version\n"
                  << "Integer virtual ticks; higher numbers mean higher fixed priority.\n";
        return 0;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--version") {
        std::cout << chronos::version() << '\n';
        return 0;
    }
    try {
        std::filesystem::path path, output;
        std::optional<chronos::Policy> policy;
        bool comparison = false;
        for (int i = 1; i < argc; ++i) {
            const std::string_view arg(argv[i]);
            const auto value = [&]() -> std::string {
                if (++i == argc) throw std::invalid_argument("Missing option value");
                return argv[i];
            };
            if (arg == "--scenario") path = value();
            else if (arg == "--export") output = value();
            else if (arg == "--scheduler") policy = chronos::parsePolicy(value());
            else if (arg == "--compare") comparison = true;
            else if (arg != "--headless") throw std::invalid_argument("Unknown option: " + std::string(arg));
        }
        if (path.empty()) throw std::invalid_argument("--scenario is required");
        auto scenario = chronos::loadScenario(path);
        if (policy) scenario.policy = *policy;
        chronos::validate(scenario);
        if (comparison) {
            if (!output.empty()) throw std::invalid_argument("--compare writes JSON to stdout; do not combine with --export");
            std::cout << chronos::compare(scenario).dump(2) << '\n';
        } else {
            chronos::SimulationEngine engine(scenario);
            engine.run();
            const auto result = engine.snapshot();
            if (!output.empty()) chronos::exportReport(result, scenario, output);
            std::cout << chronos::snapshotToJson(result, scenario).dump(2) << '\n';
        }
        return 0;
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Invalid JSON/schema: " << e.what() << '\n';
        return 2;
    } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid configuration: " << e.what() << '\n';
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Execution/I/O failure: " << e.what() << '\n';
        return 3;
    }
}
