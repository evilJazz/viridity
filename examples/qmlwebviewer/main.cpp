#ifdef USE_QTQUICK1
#include <QApplication>
#include <QtDeclarative>
#include <QDeclarativeEngine>
#include "display/adapters/qtquick1adapter.h"
#else
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QQuickItem>
#include "display/adapters/qtquick2adapter.h"
#endif

#include "display/declarativescenesizehandler.h"

#include <QFileInfo>
#include <QDir>

#include <Viridity/ViridityWebServer>

#include "viriditydeclarative.h"
#include "viriditydatabridge.h"
#include "graphicsscenedisplaysessionmanager.h"
#include "handlers/filerequesthandler.h"

#include "kclplugin.h"

#include "KCL/filesystemutils.h"
#include "KCL/debug.h"

class MySessionManager : public ViriditySessionManager
{
public:
    QString qmlFileName;

protected:
    void setLogic(ViriditySession *session)
    {
        DGUARDMETHODTIMED;
        // RUNS IN MAIN THREAD! session also currently in main thread, later moved to worker thread by web server!

        // Keep the engine in the main thread, do not parent to session as it is moved to a separate thread later on.
#ifdef USE_QTQUICK1
        QDeclarativeEngine *engine = new QDeclarativeEngine();
#else
        QQmlEngine *engine = new QQmlEngine();
#endif

        // Initialize addons...
        KCLPlugin *kcl = new KCLPlugin;
        kcl->initializeEngine(engine, "KCL");
        kcl->registerTypes("KCL");

        engine->rootContext()->setContextProperty("currentSession", session);

#ifdef USE_QTQUICK1
        QDeclarativeComponent component(engine, QUrl::fromLocalFile(qmlFileName));

        if (component.status() != QDeclarativeComponent::Ready)
            qFatal("Component is not ready: %s", component.errorString().toUtf8().constData());
#else
        QQmlComponent component(engine, QUrl::fromLocalFile(qmlFileName));

        if (component.status() != QQmlComponent::Ready)
            qFatal("Component is not ready: %s", component.errorString().toUtf8().constData());
#endif

        QObject *instance = component.create();

        if (!instance)
            qFatal("Could not create instance of component.");

#ifdef USE_QTQUICK1
        QDeclarativeItem *rootItem = qobject_cast<QDeclarativeItem *>(instance);

        QGraphicsScene *scene = new QGraphicsScene(engine);
        scene->addItem(rootItem);

        GraphicsSceneAdapter *adapter = new QtQuick1Adapter(rootItem);
#else
        GraphicsSceneAdapter *adapter = 0;
        QQuickItem *rootItem = qobject_cast<QQuickItem *>(instance);
        if (!rootItem)
        {
            QQuickWindow *window = qobject_cast<QQuickWindow *>(instance);
            if (!window)
                qFatal("Could not cast instance to usable type.");

            adapter = new QtQuick2Adapter(window);
        }
        else
            adapter = new QtQuick2Adapter(rootItem);
#endif

        // Install message handler for resize() commands on the item...
        new DeclarativeSceneSizeHandler(session, "main", adapter, true, rootItem);

        SingleGraphicsSceneDisplaySessionManager *displaySessionManager = new SingleGraphicsSceneDisplaySessionManager(session, session, adapter);

        // Take care of our created instances because they are not parented and live in the main thread.
        QObject::connect(displaySessionManager, SIGNAL(destroyed()), adapter, SLOT(deleteLater()));
        QObject::connect(session, SIGNAL(destroyed()), instance, SLOT(deleteLater()));
        QObject::connect(instance, SIGNAL(destroyed()), engine, SLOT(deleteLater()));

        EncoderSettings &es = displaySessionManager->encoderSettings();
        es.patchEncodingFormat = EncoderSettings::EncodingFormat_Auto;
        es.alphaChannelEnabled = true;
        es.inlineMaxBytes = 0;
        es.compressionLevel = 1;
        es.jpegQuality = 50;
        es.useMultithreading = true;

        ComparerSettings &cs = displaySessionManager->comparerSettings();
        cs.tileWidth = 64;
        cs.useMultithreading = true;
        cs.analyzeMoves = true;

        session->setLogic(rootItem);
    }
};

int main(int argc, char *argv[])
{
#ifdef USE_QTQUICK1
    QApplication a(argc, argv);
#else
    QGuiApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
#endif

    ViridityDeclarative::registerTypes();

    if (a.arguments().count() == 1)
        qFatal("Usage: %s <QML-file to serve> [port] [HTML-directory to serve]", QFileInfo(a.applicationFilePath()).fileName().toUtf8().constData());

    if (a.arguments().length() == 2)
    {
        QString htmlDirName = a.arguments().at(1);

        if (QDir(htmlDirName).exists())
        {
            FileRequestHandler::publishDirectoryGlobally("/", htmlDirName);

            QString indexFileName = FileSystemUtils::pathAppend(htmlDirName, "index.html");
            if (QFileInfo(indexFileName).exists())
                FileRequestHandler::publishFileGlobally("/", indexFileName);
        }
    }
    else
    {
        FileRequestHandler::publishFileGlobally("/", ":/index.html");
        FileRequestHandler::publishFileGlobally("/index.html", ":/index.html");
    }

    FileRequestHandler::publishDirectoryGlobally("/", ":/Client");

    MySessionManager sessionManager;
    sessionManager.qmlFileName = a.arguments().at(1);
    const int dataPort = a.arguments().count() > 2 ? a.arguments().at(2).toInt() : 8080;

    ViridityWebServer server(&a, &sessionManager);
    if (server.listen(QHostAddress::Any, dataPort, QThread::idealThreadCount()))
        qDebug("Server is now listening on 127.0.0.1 port %d", dataPort);
    else
        qFatal("Could not setup server on 127.0.0.1 %d.", dataPort);

    return a.exec();
}
