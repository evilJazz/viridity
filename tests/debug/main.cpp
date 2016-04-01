#include <QApplication>
#include <QThread>

#include <Viridity/ViridityWebServer>

#include <QtDeclarative>
#include "kclplugin.h"

#include "KCL/debug.h"

#include "viriditydeclarative.h"
#include "viriditydatabridge.h"
#include "graphicsscenedisplaymanager.h"
#include "declarativescenesizehandler.h"
#include "display/adapters/qtquick1adapter.h"
#include "handlers/filerequesthandler.h"
#include "handlers/fileuploadhandler.h"

#include "tools/graphicsscenedisplayrecorder.h"

class MySceneDisplaySessionManager : public AbstractMultiGraphicsSceneDisplayManager
{
public:
    MySceneDisplaySessionManager(ViriditySession *session, QObject *parent = 0) : AbstractMultiGraphicsSceneDisplayManager(session, parent) {}
    virtual ~MySceneDisplaySessionManager() {}

    QDeclarativeEngine *engine;

protected slots:
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id, const QStringList &params)
    {
        GraphicsSceneDisplay *display = AbstractMultiGraphicsSceneDisplayManager::createDisplayInstance(id, params);

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

    virtual AbstractGraphicsSceneAdapter *getAdapter(const QString &id, const QStringList &params)
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

        QGraphicsScene *scene = new QGraphicsScene(engine);
        scene->addItem(item);

        QtQuick1Adapter *adapter = new QtQuick1Adapter(item);

        // Install message handler for resize() commands on the item...
        new DeclarativeSceneSizeHandler(session(), id, adapter, true);

        return adapter;
    }

    virtual void tearDownAdapter(const QString &id, AbstractGraphicsSceneAdapter *adapter)
    {
        // RUNS IN MAIN THREAD!
        delete adapter;
    }
};

class MySessionManager : public AbstractViriditySessionManager
{
protected:
    void initSession(ViriditySession *session)
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

    FileRequestHandler::publishViridityFiles();

    MySessionManager sessionManager;

    ViridityWebServer server(&a, &sessionManager);
    if (server.listen(QHostAddress::Any, dataPort, QThread::idealThreadCount()))
        qDebug("Server is now listening on 127.0.0.1 port %d", dataPort);
    else
        qFatal("Could not setup server on 127.0.0.1 %d.", dataPort);

    return a.exec();
}
