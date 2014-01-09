#include <QtGui/QApplication>
#include <QtDeclarative>
#include <QDeclarativeEngine>

#ifdef USE_MULTITHREADED_WEBSERVER
#include <Viridity/GraphicsSceneMultiThreadedWebServer>
#else
#include <Viridity/GraphicsSceneSingleThreadedWebServer>
#endif

#include "private/commandbridge.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;
    const int commandPort = a.arguments().count() > 2 ? a.arguments().at(2).toInt() : 8081;

    QDeclarativeEngine engine;

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

#ifdef USE_MULTITHREADED_WEBSERVER
    GraphicsSceneMultiThreadedWebServer server(&a, &scene);
    server.listen(QHostAddress::Any, dataPort, QThread::idealThreadCount() * 2);
#else
    GraphicsSceneSingleThreadedWebServer server(&a, &scene);
    server.listen(QHostAddress::Any, dataPort);
#endif

    CommandWebServer commandServer(&a);
    commandServer.listen(QHostAddress::Any, commandPort);

    qDebug("Server is now listening on 127.0.0.1 data port %d and command port %d", dataPort, commandPort);

    return a.exec();
}
