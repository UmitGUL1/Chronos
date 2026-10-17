#pragma once
#include "chronos/model.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <stop_token>

namespace chronos {
using Json = nlohmann::json;
Scenario scenarioFromJson(const Json& json);
Json scenarioToJson(const Scenario& scenario);
Scenario loadScenario(const std::filesystem::path& path);
void saveScenario(const Scenario& scenario, const std::filesystem::path& path);
Json snapshotToJson(const Snapshot& snapshot, const Scenario& scenario);
void exportReport(const Snapshot& snapshot, const Scenario& scenario,
                  const std::filesystem::path& directory);
Json compare(const Scenario& scenario, std::stop_token stop = {});
}
