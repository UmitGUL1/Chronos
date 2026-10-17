#include "chronos/io.hpp"
#include "chronos/engine.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <thread>

using namespace chronos;
TEST(Scenarios, EveryBundledScenarioReplaysAndConservesCpuTime) {
    std::size_t count=0;
    for(const auto& entry:std::filesystem::directory_iterator(CHRONOS_SCENARIO_DIR)) {
        if(entry.path().extension()!=".json")continue;
        const auto scenario=loadScenario(entry.path());
        SCOPED_TRACE(entry.path().filename().string());
        SimulationEngine first(scenario),second(scenario);
        first.run();second.run();
        const auto a=first.snapshot(),b=second.snapshot();
        EXPECT_EQ(snapshotToJson(a,scenario),snapshotToJson(b,scenario));
        EXPECT_EQ(a.time,scenario.horizon);
        EXPECT_EQ(a.metrics.jobTime+a.metrics.interruptTime+a.metrics.idleTime,a.time);
        Tick previous=0;
        for(const auto& interval:a.timeline){EXPECT_EQ(interval.begin,previous);EXPECT_GT(interval.end,interval.begin);previous=interval.end;}
        EXPECT_EQ(previous,a.time);
        for(const auto& job:a.jobs){EXPECT_GE(job.remaining,0);EXPECT_EQ(job.executed+job.remaining,scenario.tasks[job.task].execution);}
        ++count;
    }
    EXPECT_GE(count,12);
}
TEST(Concurrency, IndependentEnginesOnHostThreadsProduceIdenticalResults) {
    const auto scenario=loadScenario(std::filesystem::path(CHRONOS_SCENARIO_DIR)/"priority-inheritance.json");
    Json a,b;
    std::jthread first([&]{SimulationEngine e(scenario);e.run();a=snapshotToJson(e.snapshot(),scenario);});
    std::jthread second([&]{SimulationEngine e(scenario);e.run();b=snapshotToJson(e.snapshot(),scenario);});
    first.join();second.join();
    EXPECT_EQ(a,b);EXPECT_EQ(a["jobs"].size(),3);
}
TEST(Concurrency, AlreadyRequestedStopDoesNotAdvanceVirtualTime) {
    Scenario s;s.tasks={{"A","A",20,0,20,0,0,{}}};
    SimulationEngine engine(s);std::stop_source source;source.request_stop();engine.run(source.get_token());
    EXPECT_EQ(engine.time(),0);EXPECT_TRUE(engine.snapshot().jobs.empty());EXPECT_FALSE(engine.finished());
}
