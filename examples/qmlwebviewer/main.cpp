#include <QApplication>
#include <QtDeclarative>
#include <QDeclarativeEngine>

#include <Viridity/GraphicsSceneMultiThreadedWebServer>

#include "kclplugin.h"

#include "private/commandbridge.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    QDeclarativeEngine engine;

    KCLPlugin kcl;
    kcl.initializeEngine(&engine, "KCL");
    kcl.registerTypes("KCL");

    QDeclarativeContext *rootContext = engine.rootContext();
    rootContext->setContextProperty("commandBridge", &globalCommandBridge);

    QDeclarativeComponent component(&engine, QUrl("qrc:/qml/test.qml"));

    if (component.status() != QDeclarativeComponent::Ready)
        qFatal("Component is not ready.");

    QObject *instance = component.create();

    if (!instance)
        qFatal("Could not create instance of component.");

    QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(instance);

    QGraphicsScene scene;
    scene.addItem(item);

    GraphicsSceneMultiThreadedWebServer server(&a, &scene);
    server.listen(QHostAddress::Any, dataPort, QThread::idealThreadCount() * 2);

    qDebug("Server is now listening on 127.0.0.1 port %d", dataPort);

    return a.exec();
}
