#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <QQmlError>
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
