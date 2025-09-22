#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "CameraController.h"
#include <QtQml>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Ваши boost/time или другой код можно оставить
    CameraController cameraController;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("cameraController", &cameraController);

    const QUrl url(QStringLiteral("qrc:/SimplestApp/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
