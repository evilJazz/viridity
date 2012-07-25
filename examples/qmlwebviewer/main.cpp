#include <QtGui/QApplication>
#include <QtDeclarative>
#include <QDeclarativeEngine>
#include <Viridity/GraphicsSceneMultiThreadedWebServer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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

    GraphicsSceneMultiThreadedWebServer server(&a, &scene);
    server.listen(QHostAddress::Any, 8080, QThread::idealThreadCount() * 2);

    qDebug("Server is now listening on 127.0.0.1 Port 8080");

    return a.exec();
}
