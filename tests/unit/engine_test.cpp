#include "chronos/engine.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <stdexcept>

using namespace chronos;
namespace {
Task task(std::string id, Tick execution, Tick release = 0, int priority = 0,
          Tick deadline = 20, Tick period = 0, std::vector<Action> actions = {}) {
    return {id, id, execution, period, deadline, release, priority, std::move(actions)};
}
Scenario basic(Policy policy = Policy::Fcfs) {
    Scenario s;
    s.policy = policy;
    s.horizon = 8;
    s.tasks = {task("A", 3), task("B", 2)};
    return s;
}
Snapshot run(Scenario s) { SimulationEngine engine(std::move(s)); engine.run(); return engine.snapshot(); }
std::vector<std::tuple<Tick, Tick, std::string>> trace(const Snapshot& r) {
    std::vector<std::tuple<Tick, Tick, std::string>> result;
    for (const auto& i : r.timeline) result.emplace_back(i.begin, i.end, i.label);
    return result;
}
Tick completedTask(const Snapshot& r, std::size_t index) {
    for (const auto& j : r.jobs) if (j.task == index) return j.completion.value_or(-1);
    return -1;
}
Scenario inversion(bool pi) {
    Scenario s;
    s.policy = Policy::FixedPriority;
    s.inheritance = pi;
    s.horizon = 16;
    s.resources = {"R"};
    s.tasks = {task("L", 8, 0, 1, 20, 0, {{0,ActionKind::Lock,"R"},{6,ActionKind::Unlock,"R"}}),
               task("M", 4, 3, 2),
               task("H", 2, 2, 3, 8, 0, {{0,ActionKind::Lock,"R"},{1,ActionKind::Unlock,"R"}})};
    return s;
}
}
TEST(Scheduling, FcfsReferenceAndMetrics) {
    const auto r = run(basic());
    const std::vector<std::tuple<Tick,Tick,std::string>> expected{{0,3,"A#1"},{3,5,"B#2"},{5,8,"IDLE"}};
    EXPECT_EQ(trace(r), expected);
    EXPECT_DOUBLE_EQ(r.metrics.utilization, 0.625);
    EXPECT_DOUBLE_EQ(r.metrics.averageWaiting, 1.5);
    EXPECT_DOUBLE_EQ(r.metrics.averageResponse, 1.5);
    EXPECT_DOUBLE_EQ(r.metrics.averageTurnaround, 4);
    EXPECT_EQ(r.metrics.contextSwitches, 1);
}
TEST(Scheduling, SjfChoosesShortestAndRemainsNonpreemptive) {
    auto s = basic(Policy::Sjf);
    EXPECT_EQ(run(s).timeline.front().label, "B#2");
    s.tasks = {task("A",5),task("B",1,2)};
    EXPECT_EQ(completedTask(run(s),0),5);
}
TEST(Scheduling, SrtfPreemptsAtRelease) {
    auto s = basic(Policy::Srtf);
    s.tasks = {task("A",5),task("B",2,2)};
    const std::vector<std::tuple<Tick,Tick,std::string>> expected{{0,2,"A#1"},{2,4,"B#2"},{4,7,"A#1"},{7,8,"IDLE"}};
    const auto r=run(s);
    EXPECT_EQ(trace(r),expected);
    EXPECT_EQ(r.metrics.preemptions,1);
}
TEST(Scheduling, RoundRobinQuantumReference) {
    auto s=basic(Policy::RoundRobin);
    s.tasks={task("A",5),task("B",3)};
    const std::vector<std::tuple<Tick,Tick,std::string>> expected{{0,2,"A#1"},{2,4,"B#2"},{4,6,"A#1"},{6,7,"B#2"},{7,8,"A#1"}};
    EXPECT_EQ(trace(run(s)),expected);
}
TEST(Scheduling, ArrivalsPrecedeQuantumRequeue) {
    auto s=basic(Policy::RoundRobin);
    s.tasks={task("A",4),task("B",1,2)};
    EXPECT_EQ(completedTask(run(s),1),3);
}
TEST(Scheduling, FixedPriorityReference) {
    auto s=basic(Policy::FixedPriority);
    s.tasks={task("A",5,0,1),task("B",2,2,3)};
    auto r=run(s);
    EXPECT_EQ(completedTask(r,1),4);
    EXPECT_EQ(completedTask(r,0),7);
}
TEST(Scheduling, EqualPriorityUsesStableReadyOrder) {
    auto s=basic(Policy::FixedPriority);
    EXPECT_EQ(run(s).timeline.front().label,"A#1");
}
TEST(Scheduling, RmsRanksByPeriod) {
    auto s=basic(Policy::Rms);
    s.tasks={task("slow",1,0,99,8,8),task("fast",1,0,-99,4,4)};
    auto r=run(s);
    EXPECT_EQ(r.timeline.front().label,"fast#2");
    EXPECT_GT(r.jobs[1].basePriority,r.jobs[0].basePriority);
}
TEST(Scheduling, EdfUsesAbsoluteDeadlines) {
    auto s=basic(Policy::Edf);
    s.tasks={task("A",4,0,0,10),task("B",1,1,0,2)};
    EXPECT_EQ(completedTask(run(s),1),2);
}
TEST(Engine, DeterministicReplayAndTickAgree) {
    auto s=inversion(true);
    SimulationEngine stepped(s), ticked(s);
    while(stepped.step()){}
    while(ticked.tick()){}
    EXPECT_EQ(trace(stepped.snapshot()),trace(ticked.snapshot()));
    EXPECT_EQ(trace(run(s)),trace(run(s)));
    EXPECT_EQ(stepped.snapshot().events.size(),run(s).events.size());
}
TEST(Engine, ZeroTimeInitializationThenOneTick) {
    SimulationEngine e(basic());
    EXPECT_EQ(e.time(),0);
    ASSERT_TRUE(e.step());
    EXPECT_EQ(e.time(),0);
    ASSERT_TRUE(e.tick());
    EXPECT_EQ(e.time(),1);
    EXPECT_EQ(e.snapshot().jobs[0].executed,1);
}
TEST(Engine, OverlappingPeriodicJobsAreSeparateInstances) {
    auto s=basic();
    s.tasks={task("overlap",5,0,0,7,2)};
    auto r=run(s);
    EXPECT_EQ(r.jobs.size(),4);
    EXPECT_EQ(r.jobs[1].release,2);
    EXPECT_EQ(r.jobs[2].release,4);
    EXPECT_EQ(r.jobs[0].completion,5);
}
TEST(Engine, CompletionExactlyAtDeadlineIsOnTime) {
    auto s=basic(); s.tasks={task("A",3,0,0,3)};
    auto r=run(s);
    EXPECT_FALSE(r.jobs[0].missed);
    EXPECT_EQ(r.metrics.deadlineMisses,0);
    EXPECT_EQ(r.metrics.evaluatedDeadlines,1);
}
TEST(Engine, DeadlineMissDoesNotTerminateJob) {
    auto s=basic(); s.tasks={task("A",5,0,0,2)};
    auto r=run(s);
    EXPECT_TRUE(r.jobs[0].missed);
    EXPECT_EQ(r.jobs[0].state,State::Completed);
    EXPECT_EQ(r.jobs[0].completion,5);
}
TEST(Engine, HorizonIsHalfOpenForReleases) {
    auto s=basic(); s.tasks={task("A",1,8)};
    auto r=run(s);
    EXPECT_TRUE(r.jobs.empty());
    EXPECT_EQ(r.metrics.idleTime,8);
}
TEST(Engine, AccountingAcrossIdleAndInterrupts) {
    auto s=basic(); s.tasks={task("A",2,2)};
    s.interrupts={{"I",3,2,1}};
    auto r=run(s);
    EXPECT_EQ(r.metrics.jobTime,2);
    EXPECT_EQ(r.metrics.interruptTime,2);
    EXPECT_EQ(r.metrics.idleTime,4);
    EXPECT_EQ(r.metrics.jobTime+r.metrics.interruptTime+r.metrics.idleTime,r.time);
    for(const auto& j:r.jobs) EXPECT_EQ(j.executed+j.remaining,s.tasks[j.task].execution);
}
TEST(Resources, ContentionBlocksWithoutCpuConsumptionAndTransfersOwnership) {
    auto s=inversion(false); s.tasks.erase(s.tasks.begin()+1);
    auto r=run(s);
    EXPECT_EQ(r.jobs[1].blockingTime,4);
    EXPECT_EQ(r.jobs[1].executed,2);
    EXPECT_FALSE(r.resources[0].owner);
}
TEST(Resources, InheritanceChangesActualCompletionAndRestoresPriority) {
    auto without=run(inversion(false)), with=run(inversion(true));
    EXPECT_EQ(completedTask(without,2),12);
    EXPECT_EQ(completedTask(with,2),8);
    EXPECT_TRUE(without.jobs[1].missed); // H is job #2, released before M
    EXPECT_FALSE(with.jobs[1].missed);
    EXPECT_EQ(with.jobs[0].effectivePriority,1);
    EXPECT_TRUE(std::any_of(with.events.begin(),with.events.end(),[](auto& e){return e.kind=="priority" && e.detail=="1 -> 3";}));
}
TEST(Resources, TransitiveDonationAcrossNestedOwners) {
    Scenario s; s.policy=Policy::FixedPriority; s.inheritance=true; s.horizon=12; s.resources={"A","B"};
    s.tasks={task("low",6,0,1,20,0,{{0,ActionKind::Lock,"B"},{5,ActionKind::Unlock,"B"}}),
             task("middle",2,1,2,20,0,{{0,ActionKind::Lock,"A"},{0,ActionKind::Lock,"B"},{1,ActionKind::Unlock,"B"},{1,ActionKind::Unlock,"A"}}),
             task("high",1,2,5,20,0,{{0,ActionKind::Lock,"A"},{1,ActionKind::Unlock,"A"}})};
    SimulationEngine e(s); while(e.time()<2)e.step();
    auto r=e.snapshot();
    EXPECT_EQ(r.jobs[0].effectivePriority,5);
    EXPECT_EQ(r.jobs[1].effectivePriority,5);
    EXPECT_EQ(r.jobs[2].state,State::Blocked);
    e.run();
    EXPECT_TRUE(e.snapshot().deadlocks.empty());
    EXPECT_EQ(e.snapshot().jobs[0].effectivePriority,1);
}
TEST(Resources, CompletionReleasesOwnedResources) {
    auto s=basic();s.resources={"R"};s.tasks={task("A",1,0,0,20,0,{{0,ActionKind::Lock,"R"}})};
    EXPECT_FALSE(run(s).resources[0].owner);
}
TEST(Resources, InvalidUnlockAndRecursiveLockRejected) {
    auto s=basic();s.resources={"R"};
    s.tasks[0].actions={{0,ActionKind::Unlock,"R"}};
    EXPECT_THROW(SimulationEngine e(s),std::invalid_argument);
    s.tasks[0].actions={{0,ActionKind::Lock,"R"},{1,ActionKind::Lock,"R"}};
    EXPECT_THROW(SimulationEngine e(s),std::invalid_argument);
}
TEST(Deadlock, DetectsTwoJobCycleWithoutRecovery) {
    auto s=basic(Policy::RoundRobin);s.horizon=12;s.quantum=1;s.resources={"R1","R2"};
    s.tasks={task("A",4,0,0,10,0,{{0,ActionKind::Lock,"R1"},{2,ActionKind::Lock,"R2"}}),
             task("B",4,0,0,10,0,{{0,ActionKind::Lock,"R2"},{2,ActionKind::Lock,"R1"}})};
    auto r=run(s);
    ASSERT_EQ(r.deadlocks.size(),1);
    EXPECT_EQ(r.deadlocks[0].time,4);
    EXPECT_EQ(r.deadlocks[0].cycle,(std::vector<JobId>{1,2}));
    EXPECT_EQ(r.metrics.completed,0);
    EXPECT_EQ(r.jobs[0].state,State::Blocked);
}
TEST(Deadlock, DetectsThreeJobCycle) {
    auto s=basic(Policy::RoundRobin);s.horizon=12;s.quantum=1;s.resources={"R1","R2","R3"};
    s.tasks={task("A",4,0,0,20,0,{{0,ActionKind::Lock,"R1"},{2,ActionKind::Lock,"R2"}}),
             task("B",4,0,0,20,0,{{0,ActionKind::Lock,"R2"},{2,ActionKind::Lock,"R3"}}),
             task("C",4,0,0,20,0,{{0,ActionKind::Lock,"R3"},{2,ActionKind::Lock,"R1"}})};
    auto r=run(s);
    ASSERT_EQ(r.deadlocks.size(),1);
    EXPECT_EQ(r.deadlocks[0].cycle.size(),3);
    EXPECT_EQ(r.deadlocks[0].time,6);
}
TEST(Deadlock, ContentionIsNotACycle) { EXPECT_TRUE(run(inversion(true)).deadlocks.empty()); }
TEST(Interrupts, HigherPriorityNestsAndSuspendedHandlerResumes) {
    auto s=basic();s.tasks.clear();s.interrupts={{"low",1,4,1},{"high",2,1,3}};
    auto r=run(s);
    const std::vector<std::tuple<Tick,Tick,std::string>> expected{{0,1,"IDLE"},{1,2,"low"},{2,3,"high"},{3,6,"low"},{6,8,"IDLE"}};
    EXPECT_EQ(trace(r),expected);
    EXPECT_EQ(r.metrics.interruptTime,5);
}
TEST(Interrupts, NonNestedHandlerRemainsPending) {
    auto s=basic();s.tasks.clear();s.nestedInterrupts=false;
    s.interrupts={{"low",1,4,1},{"high",2,1,3}};
    auto r=run(s);
    EXPECT_EQ(r.interrupts[1].firstService,5);
    EXPECT_DOUBLE_EQ(r.metrics.averageInterruptLatency,1.5);
}
TEST(Interrupts, MaskDefersUntilWindowEnd) {
    auto s=basic();s.tasks.clear();s.masks={{0,4}};s.interrupts={{"I",1,2,1}};
    auto r=run(s);
    EXPECT_EQ(r.interrupts[0].firstService,4);
    EXPECT_DOUBLE_EQ(r.metrics.averageInterruptLatency,3);
}
TEST(Interrupts, InterruptCanCauseDeadlineMiss) {
    auto s=basic();s.tasks={task("A",3,0,0,4)};s.interrupts={{"I",1,2,1}};
    auto r=run(s);EXPECT_TRUE(r.jobs[0].missed);EXPECT_EQ(r.jobs[0].completion,5);
}
TEST(Watchdog, HeartbeatsInvalidateStaleTimers) {
    auto s=basic();s.horizon=15;s.watchdog={true,5,{2,4},false};
    auto r=run(s);
    std::vector<Tick> times;for(const auto&e:r.events)if(e.kind=="watchdog_timeout")times.push_back(e.time);
    EXPECT_EQ(times,(std::vector<Tick>{9,14}));
}
TEST(Watchdog, HeartbeatExactlyAtExpirationWins) {
    auto s=basic();s.watchdog={true,4,{4},false};
    EXPECT_EQ(run(s).metrics.watchdogTimeouts,1); // next timeout is at horizon 8
}
TEST(Watchdog, RecoveryTerminatesJobsAndReleasesLocks) {
    auto s=inversion(false);s.watchdog={true,4,{},true};
    auto r=run(s);
    EXPECT_EQ(r.jobs[0].state,State::Terminated);
    EXPECT_FALSE(r.resources[0].owner);
    EXPECT_TRUE(r.resources[0].waiting.empty());
}
TEST(Analysis, RmsExceededBoundIsInconclusive) {
    auto s=basic(Policy::Rms);s.tasks={task("A",2,0,0,4,4),task("B",2,0,0,5,5)};
    auto a=analyze(s,Policy::Rms);
    EXPECT_TRUE(a.applicable);
    EXPECT_NE(a.result.find("Inconclusive"),std::string::npos);
    EXPECT_NE(analyze(s,Policy::Edf).result.find("Feasible"),std::string::npos);
}
TEST(Analysis, RejectsConstrainedDeadlineAndBlockingAssumptions) {
    auto s=basic(Policy::Edf);s.tasks={task("A",1,0,0,3,4)};
    EXPECT_FALSE(analyze(s,Policy::Edf).applicable);
    s.tasks[0].deadline=4;s.resources={"R"};
    EXPECT_FALSE(analyze(s,Policy::Edf).applicable);
}
TEST(Validation, RejectsBadQuantumBudgetResourceAndExcessJobs) {
    auto s=basic();s.quantum=0;EXPECT_THROW(validate(s),std::invalid_argument);
    s=basic();s.tasks[0].execution=-1;EXPECT_THROW(validate(s),std::invalid_argument);
    s=basic();s.tasks[0].actions={{0,ActionKind::Lock,"missing"}};EXPECT_THROW(validate(s),std::invalid_argument);
    s=basic();s.horizon=maxTime;s.tasks[0].period=1;EXPECT_THROW(validate(s),std::invalid_argument);
}
