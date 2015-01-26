#include <QApplication>
#include <QtDeclarative>
#include <QDeclarativeEngine>

#include <Viridity/GraphicsSceneMultiThreadedWebServer>

#include "kclplugin.h"

#include "private/commandbridge.h"

QGraphicsScene *createScene()
{
    QDeclarativeEngine *engine = new QDeclarativeEngine();

    KCLPlugin *kcl = new KCLPlugin;
    kcl->initializeEngine(engine, "KCL");
    kcl->registerTypes("KCL");

    QDeclarativeContext *rootContext = engine->rootContext();
    rootContext->setContextProperty("CommandBridge", &globalCommandBridge);

    QDeclarativeComponent component(engine, QUrl("qrc:/qml/test.qml"));

    if (component.status() != QDeclarativeComponent::Ready)
        qFatal("Component is not ready.");

    QObject *instance = component.create();

    if (!instance)
        qFatal("Could not create instance of component.");

    QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(instance);

    QGraphicsScene *scene = new QGraphicsScene(engine);
    scene->addItem(item);

    QObject::connect(scene, SIGNAL(destroyed()), engine, SLOT(deleteLater()));

    return scene;
}

class MySessionManager : public MultiGraphicsSceneDisplaySessionManager
{
protected:
    QGraphicsScene *getScene(const QString &id)
    {
        return createScene();
    }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    /*
    QGraphicsScene *scene = createScene();
    SingleGraphicsSceneDisplaySessionManager sessionManager(&a, scene);
    */

    MySessionManager sessionManager;

    GraphicsSceneMultiThreadedWebServer server(&a, &sessionManager);
    server.listen(QHostAddress::Any, dataPort, QThread::idealThreadCount());

    qDebug("Server is now listening on 127.0.0.1 port %d", dataPort);

    return a.exec();
}
