#include <QtGui/QApplication>
#include <QtDeclarative>
#include <QDeclarativeEngine>

#ifdef USE_MULTITHREADED_WEBSERVER
#include <Viridity/GraphicsSceneMultiThreadedWebServer>
#else
#include <Viridity/GraphicsSceneSingleThreadedWebServer>
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    int port = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    QDeclarativeEngine engine;
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
    server.listen(QHostAddress::Any, port, QThread::idealThreadCount() * 2);
#else
    GraphicsSceneSingleThreadedWebServer server(&a, &scene);
    server.listen(QHostAddress::Any, port);
#endif

    qDebug("Server is now listening on 127.0.0.1 Port %d", port);

    return a.exec();
}
