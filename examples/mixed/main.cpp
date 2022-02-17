#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "kclplugin.h"
#include <Viridity/Declarative>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // Initialize static KCL plugin...
    KCLPlugin *kcl = new KCLPlugin();
    kcl->setParent(&app);
    kcl->initializeEngine(&engine, "KCL");
    kcl->registerTypes("KCL");

    // Initialize Viridity...
    ViridityDeclarative::registerTypes();

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
