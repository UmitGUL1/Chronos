#include "chronos/io.hpp"
#include "chronos/engine.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace chronos {
namespace {
Tick integer(const Json& j, const char* name) {
    const auto& value = j.at(name);
    if (!value.is_number_integer() || (value.is_number_unsigned() && value.get<std::uint64_t>() > static_cast<std::uint64_t>(maxTime)))
        throw std::invalid_argument(std::string(name) + " must be an integer in range");
    const auto n = value.get<Tick>();
    if (n < -maxTime || n > maxTime) throw std::invalid_argument(std::string(name) + " out of range");
    return n;
}
Tick optionalInteger(const Json& j, const char* name, Tick fallback) {
    return j.contains(name) ? integer(j, name) : fallback;
}
int priority(const Json& j) {
    const auto value = optionalInteger(j, "priority", 0);
    if (value < -1'000'000 || value > 1'000'000) throw std::invalid_argument("Priority out of range");
    return static_cast<int>(value);
}
void boundedArray(const Json& j, const char* key, std::size_t limit) {
    if (j.contains(key) && (!j.at(key).is_array() || j.at(key).size() > limit))
        throw std::invalid_argument(std::string("Invalid or oversized array: ") + key);
}
Json nullable(std::optional<Tick> v) { return v ? Json(*v) : Json(nullptr); }
std::string csv(const std::string& text) {
    std::string result = "\"";
    // Prevent spreadsheet formula execution when opening exported scenario labels.
    if (!text.empty() && (text.front() == '=' || text.front() == '+' || text.front() == '-' || text.front() == '@')) result += '\'';
    for (char c : text) { if (c == '"') result += '"'; result += c; }
    return result + '"';
}
std::string xml(const std::string& text) {
    std::string out;
    for (char c : text) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out += c;
        }
    }
    return out;
}
void write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot open output: " + path.string());
    out << text;
    if (!out) throw std::runtime_error("Cannot finish output: " + path.string());
}
}
Scenario scenarioFromJson(const Json& j) {
    if (!j.is_object() || integer(j, "schema_version") != 1)
        throw std::invalid_argument("Expected scenario schema_version 1 object");
    Scenario s;
    s.name = j.at("name").get<std::string>();
    s.description = j.value("description", "");
    s.policy = parsePolicy(j.at("scheduler").get<std::string>());
    s.horizon = integer(j, "horizon");
    s.quantum = optionalInteger(j, "quantum", 2);
    s.inheritance = j.value("priority_inheritance", false);
    s.nestedInterrupts = j.value("nested_interrupts", true);
    boundedArray(j, "tasks", 10000);
    boundedArray(j, "resources", 10000);
    boundedArray(j, "interrupts", 100000);
    boundedArray(j, "masks", 100000);
    s.resources = j.value("resources", std::vector<std::string>{});
    for (const auto& value : j.at("tasks")) {
        Task task;
        task.id = value.at("id").get<std::string>();
        task.name = value.value("name", task.id);
        task.execution = integer(value, "execution");
        task.period = optionalInteger(value, "period", 0);
        task.deadline = integer(value, "deadline");
        task.release = optionalInteger(value, "release", 0);
        task.priority = priority(value);
        boundedArray(value, "actions", 10000);
        for (const auto& a : value.value("actions", Json::array())) {
            const auto type = a.at("kind").get<std::string>();
            ActionKind kind;
            if (type == "lock") kind = ActionKind::Lock;
            else if (type == "unlock") kind = ActionKind::Unlock;
            else if (type == "heartbeat") kind = ActionKind::Heartbeat;
            else throw std::invalid_argument("Unknown action kind: " + type);
            task.actions.push_back({integer(a, "offset"), kind, a.value("resource", "")});
        }
        s.tasks.push_back(std::move(task));
    }
    for (const auto& irq : j.value("interrupts", Json::array()))
        s.interrupts.push_back({irq.at("id").get<std::string>(), integer(irq, "arrival"),
                                integer(irq, "duration"), priority(irq)});
    for (const auto& mask : j.value("masks", Json::array()))
        s.masks.push_back({integer(mask, "start"), integer(mask, "end")});
    if (j.contains("watchdog")) {
        const auto& w = j.at("watchdog");
        s.watchdog.enabled = w.value("enabled", false);
        s.watchdog.timeout = optionalInteger(w, "timeout", 10);
        s.watchdog.terminateJobs = w.value("terminate_jobs", false);
        boundedArray(w, "heartbeats", 100000);
        for (const auto& h : w.value("heartbeats", Json::array())) {
            Json wrapped{{"heartbeat", h}};
            s.watchdog.heartbeats.push_back(integer(wrapped, "heartbeat"));
        }
    }
    validate(s);
    return s;
}
Json scenarioToJson(const Scenario& s) {
    Json tasks = Json::array(), interrupts = Json::array(), masks = Json::array();
    for (const auto& t : s.tasks) {
        Json actions = Json::array();
        for (const auto& a : t.actions)
            actions.push_back({{"offset", a.offset}, {"kind", a.kind == ActionKind::Lock ? "lock" : a.kind == ActionKind::Unlock ? "unlock" : "heartbeat"}, {"resource", a.resource}});
        tasks.push_back({{"id", t.id}, {"name", t.name}, {"execution", t.execution},
                         {"period", t.period}, {"deadline", t.deadline}, {"release", t.release},
                         {"priority", t.priority}, {"actions", actions}});
    }
    for (const auto& i : s.interrupts)
        interrupts.push_back({{"id", i.id}, {"arrival", i.arrival}, {"duration", i.duration}, {"priority", i.priority}});
    for (const auto& m : s.masks) masks.push_back({{"start", m.start}, {"end", m.end}});
    return {{"schema_version", 1}, {"name", s.name}, {"description", s.description},
            {"scheduler", toString(s.policy)}, {"horizon", s.horizon}, {"quantum", s.quantum},
            {"priority_inheritance", s.inheritance}, {"nested_interrupts", s.nestedInterrupts},
            {"tasks", tasks}, {"resources", s.resources}, {"interrupts", interrupts}, {"masks", masks},
            {"watchdog", {{"enabled", s.watchdog.enabled}, {"timeout", s.watchdog.timeout},
                          {"heartbeats", s.watchdog.heartbeats}, {"terminate_jobs", s.watchdog.terminateJobs}}}};
}
Scenario loadScenario(const std::filesystem::path& path) {
    if (std::filesystem::file_size(path) > 8 * 1024 * 1024)
        throw std::invalid_argument("Scenario exceeds 8 MiB input limit");
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot read scenario: " + path.string());
    return scenarioFromJson(Json::parse(input));
}
void saveScenario(const Scenario& scenario, const std::filesystem::path& path) {
    validate(scenario);
    write(path, scenarioToJson(scenario).dump(2) + '\n');
}
Json snapshotToJson(const Snapshot& s, const Scenario& scenario) {
    Json jobs = Json::array(), timeline = Json::array(), events = Json::array(),
         resources = Json::array(), interrupts = Json::array(), deadlocks = Json::array();
    for (const auto& j : s.jobs)
        jobs.push_back({{"id", j.id}, {"task", scenario.tasks.at(j.task).id}, {"release", j.release},
            {"deadline", j.deadline}, {"remaining", j.remaining}, {"executed", j.executed},
            {"state", toString(j.state)}, {"missed", j.missed}, {"base_priority", j.basePriority},
            {"effective_priority", j.effectivePriority}, {"first_dispatch", nullable(j.firstDispatch)},
            {"completion", nullable(j.completion)}, {"waiting", j.readyTime}, {"blocking", j.blockingTime},
            {"waiting_for", j.waitingFor ? Json(*j.waitingFor) : Json(nullptr)}});
    for (const auto& i : s.timeline)
        timeline.push_back({{"begin", i.begin}, {"end", i.end}, {"kind", i.kind}, {"label", i.label},
                            {"job", i.job ? Json(*i.job) : Json(nullptr)}});
    for (const auto& e : s.events)
        events.push_back({{"time", e.time}, {"kind", e.kind}, {"detail", e.detail},
                         {"job", e.job ? Json(*e.job) : Json(nullptr)}});
    for (const auto& r : s.resources)
        resources.push_back({{"id", r.id}, {"owner", r.owner ? Json(*r.owner) : Json(nullptr)}, {"waiting", r.waiting}});
    for (const auto& i : s.interrupts)
        interrupts.push_back({{"id", i.id}, {"arrival", i.arrival}, {"duration", i.duration},
                             {"remaining", i.remaining}, {"priority", i.priority}, {"state", i.state},
                             {"first_service", nullable(i.firstService)}});
    for (const auto& d : s.deadlocks)
        deadlocks.push_back({{"time", d.time}, {"cycle", d.cycle}, {"resources", d.resources}});
    const auto& m = s.metrics;
    Json metrics{{"job_time", m.jobTime}, {"interrupt_time", m.interruptTime}, {"idle_time", m.idleTime},
        {"completed", m.completed}, {"deadline_misses", m.deadlineMisses}, {"evaluated_deadlines", m.evaluatedDeadlines},
        {"context_switches", m.contextSwitches}, {"preemptions", m.preemptions}, {"scheduler_decisions", m.decisions},
        {"watchdog_timeouts", m.watchdogTimeouts}, {"cpu_utilization", m.utilization}, {"throughput_per_tick", m.throughput},
        {"average_waiting", m.averageWaiting}, {"average_turnaround", m.averageTurnaround},
        {"average_response", m.averageResponse}, {"maximum_response", m.maximumResponse},
        {"worst_completed_response", m.maximumTurnaround},
        {"total_blocking", m.totalBlocking}, {"average_interrupt_latency", m.averageInterruptLatency}, {"miss_ratio", m.missRatio}};
    const auto a = analyze(scenario, scenario.policy);
    return {{"schema_version", 1}, {"scenario", scenario.name}, {"scheduler", toString(scenario.policy)},
            {"time", s.time}, {"finished", s.finished}, {"jobs", jobs}, {"timeline", timeline},
            {"events", events}, {"resources", resources}, {"interrupts", interrupts}, {"deadlocks", deadlocks},
            {"metrics", metrics}, {"analysis", {{"utilization", a.utilization}, {"rms_bound", a.rmsBound},
                {"applicable", a.applicable}, {"result", a.result}, {"assumptions", a.assumptions}}}};
}
void exportReport(const Snapshot& snapshot, const Scenario& scenario, const std::filesystem::path& directory) {
    if (directory.empty()) throw std::invalid_argument("Empty export directory");
    std::filesystem::create_directories(directory);
    // Never overwrite existing reports. Each export uses a fresh directory.
    for (const auto* name : {"summary.json", "scenario.json", "timeline.csv", "events.csv", "jobs.csv", "timeline.svg"})
        if (std::filesystem::exists(directory / name)) throw std::invalid_argument("Report exists; choose a new directory");
    write(directory / "summary.json", snapshotToJson(snapshot, scenario).dump(2) + '\n');
    saveScenario(scenario, directory / "scenario.json");
    std::ostringstream timeline, events, jobs, svg;
    timeline << "begin,end,kind,label,job\n";
    for (const auto& i : snapshot.timeline)
        timeline << i.begin << ',' << i.end << ',' << csv(i.kind) << ',' << csv(i.label) << ','
                 << (i.job ? std::to_string(*i.job) : "") << '\n';
    events << "time,kind,detail,job\n";
    for (const auto& e : snapshot.events)
        events << e.time << ',' << csv(e.kind) << ',' << csv(e.detail) << ','
               << (e.job ? std::to_string(*e.job) : "") << '\n';
    jobs << "job,task,release,deadline,executed,remaining,state,ready_wait,blocking,first_dispatch,completion,missed\n";
    for (const auto& j : snapshot.jobs)
        jobs << j.id << ',' << csv(scenario.tasks[j.task].id) << ',' << j.release << ',' << j.deadline << ','
             << j.executed << ',' << j.remaining << ',' << toString(j.state) << ',' << j.readyTime << ',' << j.blockingTime << ','
             << (j.firstDispatch ? std::to_string(*j.firstDispatch) : "") << ','
             << (j.completion ? std::to_string(*j.completion) : "") << ',' << j.missed << '\n';
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1200\" height=\"160\" viewBox=\"0 0 1200 160\">"
        << "<rect width=\"1200\" height=\"160\" fill=\"#101827\"/><text x=\"20\" y=\"28\" fill=\"white\">"
        << xml(scenario.name) << " / " << toString(scenario.policy) << " / ticks</text>";
    const double scale = 1160.0 / static_cast<double>(std::max<Tick>(1, snapshot.time));
    for (const auto& i : snapshot.timeline)
        svg << "<rect x=\"" << 20 + static_cast<double>(i.begin) * scale << "\" y=\"50\" width=\""
            << static_cast<double>(i.end - i.begin) * scale << "\" height=\"50\" fill=\""
            << (i.kind == "interrupt" ? "#b697f8" : i.kind == "idle" ? "#344155" : "#50ded2")
            << "\" stroke=\"#101827\"><title>" << xml(i.label) << ' ' << i.begin << "–" << i.end << "</title></rect>";
    svg << "<text x=\"20\" y=\"130\" fill=\"white\">0</text><text x=\"1120\" y=\"130\" fill=\"white\">"
        << snapshot.time << "</text></svg>";
    write(directory / "timeline.csv", timeline.str());
    write(directory / "events.csv", events.str());
    write(directory / "jobs.csv", jobs.str());
    write(directory / "timeline.svg", svg.str());
}
Json compare(const Scenario& scenario, std::stop_token stop) {
    Json reports = Json::array();
    for (auto policy : {Policy::Fcfs, Policy::Sjf, Policy::Srtf, Policy::RoundRobin,
                         Policy::FixedPriority, Policy::Rms, Policy::Edf}) {
        if (stop.stop_requested()) break;
        auto workload = scenario;
        workload.policy = policy;
        // Preserve ALL workload fields; do not quietly disable PI or change tasks.
        try {
            SimulationEngine engine(workload);
            engine.run(stop);
            if (stop.stop_requested()) break;
            reports.push_back(snapshotToJson(engine.snapshot(), workload));
        } catch (const std::invalid_argument& e) {
            reports.push_back({{"scheduler", toString(policy)}, {"unsupported", e.what()}});
        }
    }
    return reports;
}
}
