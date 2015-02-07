#include <QApplication>
#include <QThread>

#include <Viridity/ViridityWebServer>

#include <QtDeclarative>
#include "kclplugin.h"

#include "KCL/debug.h"

#include "viriditydatabridge.h"
#include "graphicsscenedisplaysessionmanager.h"
#include "declarativescenesizehandler.h"
#include "handlers/filerequesthandler.h"
#include "handlers/fileuploadhandler.h"

class MySceneDisplaySessionManager : public MultiGraphicsSceneDisplaySessionManager
{
public:
    MySceneDisplaySessionManager(ViriditySession *session, QObject *parent = 0) : MultiGraphicsSceneDisplaySessionManager(session, parent) {}
    virtual ~MySceneDisplaySessionManager() {}

    QDeclarativeEngine *engine;

protected slots:
    virtual QGraphicsScene *getScene(const QString &id, const QStringList &params)
    {
        // RUNS IN MAIN THREAD!

        DGUARDMETHODTIMED;
        QDeclarativeComponent component(engine, QUrl("qrc:/qml/" + params.at(0) + ".qml"));

        if (component.status() != QDeclarativeComponent::Ready)
            qFatal("Component is not ready: %s", component.errorString().toUtf8().constData());

        QObject *instance = component.create();

        if (!instance)
            qFatal("Could not create instance of component.");

        QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(instance);

        DeclarativeSceneSizeHandler *sizeHandler = new DeclarativeSceneSizeHandler(id, item, item);
        session()->registerMessageHandler(sizeHandler);

        QGraphicsScene *scene = new QGraphicsScene(engine);
        scene->addItem(item);

        return scene;
    }
};

void createLogic(ViriditySession *session)
{
    // RUNS IN MAIN THREAD! session also currently in main thread, later moved to worker thread by web server!

    DGUARDMETHODTIMED;
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

    FileUploadHandler *fileUploadHandler = new FileUploadHandler(session->sessionManager()->server(), session);
    session->registerRequestHandler(fileUploadHandler);
    engine->rootContext()->setContextProperty("fileUploadHandler", fileUploadHandler);

    MySceneDisplaySessionManager *displaySessionManager = new MySceneDisplaySessionManager(session, session);
    displaySessionManager->engine = engine;
    session->registerMessageHandler(displaySessionManager);

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

    FileRequestHandler::publishFileGlobally("/", ":/index.html", "text/html; charset=utf8");
    FileRequestHandler::publishFileGlobally("/index.html", ":/index.html", "text/html; charset=utf8");
    FileRequestHandler::publishFileGlobally("/testimages/backtile.png", ":/testimages/backtile.png", "image/png");

    FileRequestHandler::publishFileGlobally("/Viridity.js", ":/Client/Viridity.js", "application/javascript; charset=utf8");
    FileRequestHandler::publishFileGlobally("/DataBridge.js", ":/Client/DataBridge.js", "application/javascript; charset=utf8");
    FileRequestHandler::publishFileGlobally("/DisplayRenderer.js", ":/Client/DisplayRenderer.js", "application/javascript; charset=utf8");
    FileRequestHandler::publishFileGlobally("/jquery.mousewheel.js", ":/Client/jquery.mousewheel.js", "application/javascript; charset=utf8");

    ViridityWebServer server(&a, &sessionManager);
    if (server.listen(QHostAddress::Any, dataPort, QThread::idealThreadCount()))
        qDebug("Server is now listening on 127.0.0.1 port %d", dataPort);
    else
        qFatal("Could not setup server on 127.0.0.1 %d.", dataPort);

    return a.exec();
}
