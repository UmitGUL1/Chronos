#include "chronos/io.hpp"
#include "chronos/engine.hpp"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <fstream>

using namespace chronos;
namespace {
Scenario sample() {
    Scenario s;s.horizon=8;s.tasks={{"A","A",3,0,3,0,0,{}},{"B","B",2,0,5,0,0,{}}};return s;
}
}
TEST(Configuration, RoundTripAllFields) {
    auto s=sample();s.policy=Policy::FixedPriority;s.resources={"R"};s.inheritance=true;
    s.tasks[0].actions={{0,ActionKind::Lock,"R"},{2,ActionKind::Unlock,"R"}};
    s.watchdog={true,5,{1,3},false};s.masks={{1,2}};s.interrupts={{"I",4,1,7}};
    auto json=scenarioToJson(s);
    EXPECT_EQ(scenarioToJson(scenarioFromJson(json)),json);
}
TEST(Configuration, RejectsMissingFractionalAndInvalidFields) {
    auto j=scenarioToJson(sample());j.erase("tasks");EXPECT_THROW(scenarioFromJson(j),std::exception);
    j=scenarioToJson(sample());j["horizon"]=3.5;EXPECT_THROW(scenarioFromJson(j),std::exception);
    j=scenarioToJson(sample());j["tasks"][0]["deadline"]=0;EXPECT_THROW(scenarioFromJson(j),std::exception);
    j=scenarioToJson(sample());j["scheduler"]="made-up";EXPECT_THROW(scenarioFromJson(j),std::exception);
    j=scenarioToJson(sample());j["horizon"]=Json::number_unsigned_t(-1);EXPECT_THROW(scenarioFromJson(j),std::exception);
}
TEST(Integration, ExportReplayAndNoOverwrite) {
    static std::atomic<unsigned> counter{};
    auto directory=std::filesystem::temp_directory_path()/
        ("chronos-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(counter++));
    struct Cleanup { std::filesystem::path p;~Cleanup(){std::error_code ec;std::filesystem::remove_all(p,ec);} } cleanup{directory};
    auto s=sample();SimulationEngine engine(s);engine.run();
    exportReport(engine.snapshot(),s,directory);
    auto loaded=loadScenario(directory/"scenario.json");SimulationEngine replay(loaded);replay.run();
    EXPECT_EQ(snapshotToJson(engine.snapshot(),s),snapshotToJson(replay.snapshot(),loaded));
    std::ifstream input(directory/"summary.json");Json result=Json::parse(input);
    EXPECT_EQ(result["metrics"]["completed"],2);
    EXPECT_EQ(result["timeline"][0]["end"],3);
    EXPECT_THROW(exportReport(engine.snapshot(),s,directory),std::invalid_argument);
    EXPECT_TRUE(std::filesystem::exists(directory/"timeline.svg"));
}
TEST(Integration, ComparisonPreservesWorkloadAndIsDeterministic) {
    auto s=sample();auto a=compare(s),b=compare(s);
    EXPECT_EQ(a,b);ASSERT_EQ(a.size(),7);
    EXPECT_TRUE(a[5].contains("unsupported")); // RMS refuses aperiodic input
    for(const auto&r:a)if(!r.contains("unsupported"))EXPECT_EQ(r["jobs"].size(),2);
}
