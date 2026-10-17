#include "controller.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMetaObject>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace {
chronos::Scenario initialScenario() {
    chronos::Scenario s;
    s.name = "Fixed-priority preemption";
    s.policy = chronos::Policy::FixedPriority;
    s.horizon = 12;
    s.tasks = {{"A","A",5,0,10,0,1,{}},{"B","B",2,0,5,2,3,{}}};
    return s;
}
std::filesystem::path localPath(const QUrl& url) {
    if (!url.isLocalFile()) throw std::invalid_argument("Select a local file or directory");
    return std::filesystem::path(url.toLocalFile().toStdWString());
}
}
Controller::Controller(QObject* parent) : QObject(parent) {
    const auto s = initialScenario();
    scenarioText_ = QString::fromStdString(chronos::scenarioToJson(s).dump(2));
    thread_ = std::jthread([this, s](std::stop_token token) { loop(token, s); });
}
Controller::~Controller() {
    thread_.request_stop(); condition_.notify_all();
    if (thread_.joinable()) thread_.join();
}
void Controller::setScenarioText(const QString& text) {
    if (scenarioText_ != text) { scenarioText_ = text; emit scenarioTextChanged(); }
}
void Controller::failure(const std::string& text) {
    QMetaObject::invokeMethod(this, [this, text] { error_ = QString::fromStdString(text); emit errorChanged(); }, Qt::QueuedConnection);
}
void Controller::submit(Command command) {
    { std::lock_guard lock(mutex_);
      if (commands_.size() >= 64) { failure("Command queue full; wait for the current operation"); return; }
      commands_.push_back(std::move(command)); }
    condition_.notify_all();
}
void Controller::publish(Worker& w) {
    // Only one immutable UI snapshot may be queued at once.
    if (snapshotPending_.exchange(true)) { refreshRequested_.store(true); return; }
    refreshRequested_.store(false);
    try {
        auto data = chronos::snapshotToJson(w.engine->snapshot(), w.scenario);
        data["event_count"] = data["events"].size();
        data["interval_count"] = data["timeline"].size();
        if (data["events"].size() > 2000) data["events"].erase(data["events"].begin(), data["events"].end() - 2000);
        if (data["timeline"].size() > 5000) data["timeline"].erase(data["timeline"].begin(), data["timeline"].end() - 5000);
        auto packet = QJsonDocument::fromJson(QByteArray::fromStdString(data.dump())).toVariant().toMap();
        const bool running = w.running;
        QMetaObject::invokeMethod(this, [this, packet = std::move(packet), running]() mutable {
            report_ = std::move(packet); running_ = running; snapshotPending_.store(false); condition_.notify_all(); emit reportChanged();
        }, Qt::QueuedConnection);
    } catch (...) { snapshotPending_.store(false); throw; }
}
void Controller::loop(std::stop_token token, chronos::Scenario initial) {
    Worker w{initial, std::make_unique<chronos::SimulationEngine>(initial), false, 1, token};
    publish(w);
    while (!token.stop_requested()) {
        std::deque<Command> commands;
        { std::unique_lock lock(mutex_);
          if (commands_.empty()) {
              const auto ready = [this] { return !commands_.empty() || (refreshRequested_.load() && !snapshotPending_.load()); };
              if (w.running) condition_.wait_for(lock, token, std::chrono::milliseconds(static_cast<int>(std::max(1.0, 150.0/w.speed))), ready);
              else condition_.wait(lock, token, ready);
          }
          commands.swap(commands_); }
        if (token.stop_requested()) break;
        try {
            for (auto& command : commands) { if (token.stop_requested()) break; command(w); }
            if (w.running && !w.engine->step()) w.running = false;
            if (w.engine->finished()) w.running = false;
            publish(w);
        } catch (const std::exception& e) {
            w.running = false; failure(e.what());
            try { publish(w); } catch (...) {}
        }
    }
}
void Controller::applyJson(const chronos::Json& json) {
    try {
        const auto s = chronos::scenarioFromJson(json);
        setScenarioText(QString::fromStdString(chronos::scenarioToJson(s).dump(2)));
        error_.clear(); emit errorChanged();
        submit([s](Worker& w) { auto engine = std::make_unique<chronos::SimulationEngine>(s); w.running=false; w.scenario=s; w.engine=std::move(engine); });
    } catch (const std::exception& e) { failure(e.what()); }
}
void Controller::applyScenario() {
    try { applyJson(chronos::Json::parse(scenarioText_.toStdString())); }
    catch (const std::exception& e) { failure(e.what()); }
}
void Controller::changePolicy(const QString& name) {
    try { auto j=chronos::Json::parse(scenarioText_.toStdString()); j["scheduler"]=name.toStdString(); applyJson(j); }
    catch (const std::exception& e) { failure(e.what()); }
}
void Controller::start() { submit([](Worker& w) { w.running=true; }); }
void Controller::pause() { submit([](Worker& w) { w.running=false; }); }
void Controller::reset() { submit([](Worker& w) { w.running=false; w.engine=std::make_unique<chronos::SimulationEngine>(w.scenario); }); }
void Controller::step() { submit([](Worker& w) { w.running=false; w.engine->step(); }); }
void Controller::tick() { submit([](Worker& w) { w.running=false; w.engine->tick(); }); }
void Controller::setSpeed(double speed) {
    if (std::isfinite(speed)) submit([speed](Worker& w) { w.speed=std::clamp(speed,0.25,32.0); });
}
void Controller::load(const QUrl& url) {
    try { applyJson(chronos::scenarioToJson(chronos::loadScenario(localPath(url)))); }
    catch (const std::exception& e) { failure(e.what()); }
}
void Controller::save(const QUrl& url) {
    try { chronos::saveScenario(chronos::scenarioFromJson(chronos::Json::parse(scenarioText_.toStdString())),localPath(url)); }
    catch (const std::exception& e) { failure(e.what()); }
}
void Controller::exportTo(const QUrl& url) {
    try { const auto path=localPath(url); submit([path](Worker& w) { chronos::exportReport(w.engine->snapshot(),w.scenario,path); }); }
    catch (const std::exception& e) { failure(e.what()); }
}
void Controller::comparePolicies() {
    try {
        const auto s=chronos::scenarioFromJson(chronos::Json::parse(scenarioText_.toStdString()));
        submit([this,s](Worker& w) {
            w.running=false;
            auto json=QString::fromStdString(chronos::compare(s,w.stop).dump(2));
            QMetaObject::invokeMethod(this,[this,json] { emit comparisonReady(json); },Qt::QueuedConnection);
        });
    } catch (const std::exception& e) { failure(e.what()); }
}
void Controller::upsertTask(const QVariantMap& values, int index) {
    try {
        auto j=chronos::Json::parse(scenarioText_.toStdString());
        const auto task=chronos::Json::parse(QJsonDocument::fromVariant(values).toJson().toStdString());
        if(index>=0) { auto& old=j.at("tasks").at(static_cast<std::size_t>(index)); for(auto it=task.begin();it!=task.end();++it)old[it.key()]=it.value(); }
        else j.at("tasks").push_back(task);
        applyJson(j);
    } catch (const std::exception& e) { failure(e.what()); }
}
void Controller::removeTask(int index) {
    try {
        auto j=chronos::Json::parse(scenarioText_.toStdString()); auto& tasks=j.at("tasks");
        if(index<0 || static_cast<std::size_t>(index)>=tasks.size())throw std::invalid_argument("Select a task");
        tasks.erase(tasks.begin()+index); applyJson(j);
    } catch (const std::exception& e) { failure(e.what()); }
}
QVariantList Controller::scenarios() const {
    QVariantList result;
    const QStringList paths{QDir::currentPath()+"/scenarios",QCoreApplication::applicationDirPath()+"/../share/chronos/scenarios"};
    for(const auto& path:paths) {
        QDir dir(path); if(!dir.exists())continue;
        for(const auto& name:dir.entryList({"*.json"},QDir::Files,QDir::Name))result.append(QVariantMap{{"name",name},{"url",QUrl::fromLocalFile(dir.absoluteFilePath(name))}});
        break;
    }
    return result;
}
