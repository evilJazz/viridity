#include <QApplication>
#include <QtDeclarative>
#include <QDeclarativeEngine>

#include <Viridity/ViridityWebServer>

#include "kclplugin.h"

#include "commandbridge.h"

void createScene(ViriditySession *session)
{
    QDeclarativeEngine *engine = new QDeclarativeEngine();

    KCLPlugin *kcl = new KCLPlugin;
    kcl->initializeEngine(engine, "KCL");
    kcl->registerTypes("KCL");

    CommandBridge *commandBridge = new CommandBridge(session, engine);
    engine->rootContext()->setContextProperty("CommandBridge", commandBridge);

    session->commandHandlers.append(commandBridge);

    QDeclarativeComponent component(engine, QUrl("qrc:/qml/test.qml"));

    if (component.status() != QDeclarativeComponent::Ready)
        qFatal("Component is not ready.");

    QObject *instance = component.create();

    if (!instance)
        qFatal("Could not create instance of component.");

    QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(instance);

    session->scene = new QGraphicsScene(engine);
    session->scene->addItem(item);

    QObject::connect(session->scene, SIGNAL(destroyed()), engine, SLOT(deleteLater()));
}

class MySingleSessionManager : public SingleLogicSessionManager
{
    void setLogic(ViriditySession *session) { createScene(session); }
};

class MyMultiSessionManager : public MultiLogicSessionManager
{
    void setLogic(ViriditySession *session) { createScene(session); }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    //MySingleSessionManager sessionManager;
    MyMultiSessionManager sessionManager;

    ViridityWebServer server(&a, &sessionManager);
    server.listen(QHostAddress::Any, dataPort, QThread::idealThreadCount());

    qDebug("Server is now listening on 127.0.0.1 port %d", dataPort);

    return a.exec();
}
