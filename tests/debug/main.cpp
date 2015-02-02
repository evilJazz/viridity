#include <QApplication>
#include <QThread>

#include <Viridity/ViridityWebServer>

#include <QtDeclarative>
#include "kclplugin.h"

#include "viriditydatabridge.h"
#include "graphicsscenedisplaysessionmanager.h"

class MySceneDisplaySessionManager : public MultiGraphicsSceneDisplaySessionManager
{
public:
    MySceneDisplaySessionManager(ViriditySession *session, QObject *parent = 0) : MultiGraphicsSceneDisplaySessionManager(session, parent) {}
    virtual ~MySceneDisplaySessionManager() {}

    QDeclarativeEngine *engine;

protected slots:
    virtual QGraphicsScene *getScene(const QString &id, const QStringList &params)
    {
        QDeclarativeComponent component(engine, QUrl("qrc:/qml/" + params.at(0) + ".qml"));

        if (component.status() != QDeclarativeComponent::Ready)
            qFatal("Component is not ready: %s", component.errorString().toUtf8().constData());

        QObject *instance = component.create();

        if (!instance)
            qFatal("Could not create instance of component.");

        QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(instance);

        QGraphicsScene *scene = new QGraphicsScene(engine);
        scene->addItem(item);

        return scene;
    }
};

void createLogic(ViriditySession *session)
{
    QDeclarativeEngine *engine = new QDeclarativeEngine();

    KCLPlugin *kcl = new KCLPlugin;
    kcl->initializeEngine(engine, "KCL");
    kcl->registerTypes("KCL");

    engine->rootContext()->setContextProperty("currentSession", session);

    qmlRegisterType<ViridityDataBridge>("Viridity", 1, 0, "NativeViridityDataBridge");
    qmlRegisterUncreatableType<ViriditySession>("Viridity", 1, 0, "ViriditySession", "Can't create a session out of thin air.");

    QDeclarativeComponent component(engine, QUrl("qrc:/qml/test.qml"));

    if (component.status() != QDeclarativeComponent::Ready)
        qFatal("Component is not ready: %s", component.errorString().toUtf8().constData());

    QObject *instance = component.create();

    if (!instance)
        qFatal("Could not create instance of component.");

    /*
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(instance);

    session->scene = new QGraphicsScene(engine);
    session->scene->addItem(item);

    QObject::connect(session->scene, SIGNAL(destroyed()), engine, SLOT(deleteLater()));
    */

    MySceneDisplaySessionManager *displaySessionManager = new MySceneDisplaySessionManager(session, session);
    displaySessionManager->engine = engine;
    session->registerHandler(displaySessionManager);

    session->logic = instance;
    QObject::connect(session->logic, SIGNAL(destroyed()), engine, SLOT(deleteLater()));
}

class MySingleSessionManager : public SingleLogicSessionManager
{
    void setLogic(ViriditySession *session) { createLogic(session); }
};

class MyMultiSessionManager : public MultiLogicSessionManager
{
    void setLogic(ViriditySession *session) { createLogic(session); }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    //MySingleSessionManager sessionManager;
    MyMultiSessionManager sessionManager;

    ViridityWebServer server(&a, &sessionManager);
    if (server.listen(QHostAddress::Any, dataPort, QThread::idealThreadCount()))
        qDebug("Server is now listening on 127.0.0.1 port %d", dataPort);
    else
        qFatal("Could not setup server on 127.0.0.1 %d.", dataPort);

    return a.exec();
}
