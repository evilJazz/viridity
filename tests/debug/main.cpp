#include <QApplication>
#include <QThread>

#include <Viridity/ViridityWebServer>

#include <QtDeclarative>
#include "kclplugin.h"

#include "KCL/debug.h"

#include "viriditydeclarative.h"
#include "viriditydatabridge.h"
#include "graphicsscenedisplaysessionmanager.h"
#include "declarativescenesizehandler.h"
#include "handlers/filerequesthandler.h"
#include "handlers/fileuploadhandler.h"

#include "tools/graphicsscenedisplayrecorder.h"

class MySceneDisplaySessionManager : public MultiGraphicsSceneDisplaySessionManager
{
public:
    MySceneDisplaySessionManager(ViriditySession *session, QObject *parent = 0) : MultiGraphicsSceneDisplaySessionManager(session, parent) {}
    virtual ~MySceneDisplaySessionManager() {}

    QDeclarativeEngine *engine;

protected slots:
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id, const QStringList &params)
    {
        GraphicsSceneDisplay *display = MultiGraphicsSceneDisplaySessionManager::createDisplayInstance(id, params);

        EncoderSettings es;
        es.patchEncodingFormat = EncoderSettings::EncodingFormat_Auto;
        es.inlineMaxBytes = 0;
        es.useMultithreading = false;

        display->setEncoderSettings(es);

        ComparerSettings cs;
        cs.useMultithreading = false;

        display->setComparerSettings(cs);

        if (display)
        {
            GraphicsSceneDisplayRecorder *recorder = new GraphicsSceneDisplayRecorder(display);
            //recorder->setFullFrameFilename("/home/darkstar/Desktop/full_dump_" + id + ".fgsd");
            recorder->setDiffFrameFilename("/home/darkstar/Desktop/diff_dump_" + id + ".dgsd");
        }

        return display;
    }

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

        // Install message handler for resize() commands on the item...
        new DeclarativeSceneSizeHandler(session(), id, item, item);

        QGraphicsScene *scene = new QGraphicsScene(engine);
        scene->addItem(item);

        return scene;
    }

    virtual void tearDownScene(const QString &id, QGraphicsScene *scene)
    {
        // RUNS IN MAIN THREAD!
        delete scene;
    }
};

class MySessionManager : public ViriditySessionManager
{
protected:
    void setLogic(ViriditySession *session)
    {
        // RUNS IN MAIN THREAD! session also currently in main thread, later moved to worker thread by web server!

        DGUARDMETHODTIMED;
        QDeclarativeEngine *engine = new QDeclarativeEngine(); // Will hang here if you have QML debugging enabled and debugger was previously attached...

        KCLPlugin *kcl = new KCLPlugin;
        kcl->initializeEngine(engine, "KCL");
        kcl->registerTypes("KCL");

        engine->rootContext()->setContextProperty("currentSession", session);

        ViridityDeclarative::registerTypes();

        QDeclarativeComponent component(engine, QUrl("qrc:/qml/logic.qml"));

        if (component.status() != QDeclarativeComponent::Ready)
            qFatal("Component is not ready: %s", component.errorString().toUtf8().constData());

        QObject *instance = component.create();

        if (!instance)
            qFatal("Could not create instance of component.");

        QObject::connect(instance, SIGNAL(destroyed()), engine, SLOT(deleteLater()));

        // Add handlers...
        FileUploadHandler *fileUploadHandler = new FileUploadHandler(session, session);
        engine->rootContext()->setContextProperty("fileUploadHandler", fileUploadHandler);

        MySceneDisplaySessionManager *displaySessionManager = new MySceneDisplaySessionManager(session, session);
        displaySessionManager->engine = engine;

        session->setLogic(instance);
        QObject::connect(session, SIGNAL(destroyed()), instance, SLOT(deleteLater()));
    }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    FileRequestHandler::publishFileGlobally("/", ":/index.html", "text/html; charset=utf8");
    FileRequestHandler::publishFileGlobally("/index.html", ":/index.html", "text/html; charset=utf8");
    FileRequestHandler::publishFileGlobally("/detail1.html", ":/detail1.html", "text/html; charset=utf8");
    FileRequestHandler::publishFileGlobally("/images/backtile.png", ":/images/backtile.png", "image/png");

    FileRequestHandler::publishFileGlobally("/Viridity.js", ":/Client/Viridity.js", "application/javascript; charset=utf8");
    FileRequestHandler::publishFileGlobally("/DataBridge.js", ":/Client/DataBridge.js", "application/javascript; charset=utf8");

    FileRequestHandler::publishFileGlobally("/DisplayRenderer.js", ":/Client/DisplayRenderer.js", "application/javascript; charset=utf8");
    FileRequestHandler::publishFileGlobally("/jquery.mousewheel.js", ":/Client/jquery.mousewheel.js", "application/javascript; charset=utf8");

    MySessionManager sessionManager;

    ViridityWebServer server(&a, &sessionManager);
    if (server.listen(QHostAddress::Any, dataPort, QThread::idealThreadCount()))
        qDebug("Server is now listening on 127.0.0.1 port %d", dataPort);
    else
        qFatal("Could not setup server on 127.0.0.1 %d.", dataPort);

    return a.exec();
}
