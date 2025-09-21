#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <boost/date_time.hpp>
#include <QString>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Получаем текущее время через Boost
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
    QString currentTime = QString::fromStdString(boost::posix_time::to_simple_string(now));

    // Создаем QML контекстную переменную
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("boostTime", currentTime);

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