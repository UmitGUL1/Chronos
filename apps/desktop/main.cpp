#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <QQmlError>
#include <QQuickWindow>
#include <QDir>
#include <QImage>
#include <memory>
#include "controller.hpp"

int main(int argc, char* argv[]) {
    QGuiApplication application(argc, argv);
    QQuickStyle::setStyle("Basic");
    Controller controller;
    bool qmlWarnings = false;
    QQmlApplicationEngine engine;
    const bool smoke = application.arguments().contains("--smoke");
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &application,
                     [&qmlWarnings](const QList<QQmlError>&) { qmlWarnings = true; });
    engine.rootContext()->setContextProperty("controller", &controller);
    engine.loadFromModule("ChronOS", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    const auto arguments = application.arguments();
    const int captureIndex = arguments.indexOf("--screenshots");
    if (captureIndex >= 0) {
        if (captureIndex + 1 >= arguments.size()) return 2;
        const QString directory = arguments.at(captureIndex + 1);
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        if (!window || !QDir().mkpath(directory)) return 2;
        // Capture actual rendered QML and engine reports, without fabricated data.
        auto stage = std::make_shared<int>(0);
        auto* capture = new QTimer(&application);
        capture->setInterval(500);
        QObject::connect(capture, &QTimer::timeout, &application, [&, window, directory, stage] {
            if (!controller.error().isEmpty() || qmlWarnings) { application.exit(1); return; }
            if (!controller.report().value("finished").toBool()) return;
            if (*stage == 0) controller.comparePolicies();
            const QStringList names{"dashboard.png", "scheduler-lab.png", "analytics.png"};
            const int pages[]{0, 1, 6};
            if (*stage % 2 == 0) {
                window->setProperty("page", pages[*stage / 2]);
                window->update();
            } else {
                const auto image = window->grabWindow();
                if (image.isNull() || !image.save(QDir(directory).filePath(names[*stage / 2]))) {
                    application.exit(1); return;
                }
            }
            if (++*stage == 6) application.exit(0);
        });
        QTimer::singleShot(100, &application, [&] { controller.setSpeed(16); controller.start(); });
        QTimer::singleShot(30000, &application, [&] { application.exit(1); });
        capture->start();
    }
    if (smoke) {
        QTimer::singleShot(100, &controller, &Controller::tick);
        QTimer::singleShot(250, &controller, &Controller::step);
        QTimer::singleShot(400, &controller, &Controller::start);
        QTimer::singleShot(1200, &controller, &Controller::pause);
        QTimer::singleShot(1800, &application, [&] {
            application.exit(qmlWarnings || controller.report().isEmpty() || !controller.error().isEmpty() ? 1 : 0);
        });
    }
    return application.exec();
}
