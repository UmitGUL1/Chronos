#pragma once
#include "chronos/engine.hpp"
#include "chronos/io.hpp"
#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

class Controller final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap report READ report NOTIFY reportChanged)
    Q_PROPERTY(QString scenarioText READ scenarioText WRITE setScenarioText NOTIFY scenarioTextChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool running READ running NOTIFY reportChanged)
public:
    explicit Controller(QObject* parent = nullptr);
    ~Controller() override;
    QVariantMap report() const { return report_; }
    QString scenarioText() const { return scenarioText_; }
    QString error() const { return error_; }
    bool running() const { return running_; }
    void setScenarioText(const QString& text);
    Q_INVOKABLE void applyScenario();
    Q_INVOKABLE void changePolicy(const QString& name);
    Q_INVOKABLE void start();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void step();
    Q_INVOKABLE void tick();
    Q_INVOKABLE void setSpeed(double speed);
    Q_INVOKABLE void load(const QUrl& path);
    Q_INVOKABLE void save(const QUrl& path);
    Q_INVOKABLE void exportTo(const QUrl& directory);
    Q_INVOKABLE void comparePolicies();
    Q_INVOKABLE void upsertTask(const QVariantMap& values, int index);
    Q_INVOKABLE void removeTask(int index);
    Q_INVOKABLE QVariantList scenarios() const;
signals:
    void reportChanged();
    void scenarioTextChanged();
    void errorChanged();
    void comparisonReady(QString json);
private:
    struct Worker {
        chronos::Scenario scenario;
        std::unique_ptr<chronos::SimulationEngine> engine;
        bool running{};
        double speed{1};
        std::stop_token stop;
    };
    using Command = std::function<void(Worker&)>;
    QVariantMap report_;
    QString scenarioText_;
    QString error_;
    bool running_{};
    std::mutex mutex_;
    std::condition_variable_any condition_;
    std::deque<Command> commands_;
    std::atomic<bool> snapshotPending_{};
    std::atomic<bool> refreshRequested_{};
    std::jthread thread_;
    void submit(Command command);
    void publish(Worker& worker);
    void failure(const std::string& text);
    void applyJson(const chronos::Json& json);
    void loop(std::stop_token token, chronos::Scenario initial);
};
