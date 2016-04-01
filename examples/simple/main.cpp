#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>

#include "viriditywebserver.h"
#include "viriditydeclarative.h"
#include "viriditydatabridge.h"

#include "handlers/filerequesthandler.h"

class MySessionManager : public AbstractViriditySessionManager
{
public:
    QString qmlFileName;

protected:
    void initSession(ViriditySession *session)
    {
        // RUNS IN MAIN THREAD! session also currently in main thread, later moved to worker thread by web server!

        // Keep the engine in the main thread, do not parent to session as it is moved to a separate thread later on.
        QQmlEngine *engine = new QQmlEngine();
        engine->addImportPath(":/");

        engine->rootContext()->setContextProperty("currentSession", session);

        QQmlComponent component(engine, QUrl("qrc:/simple.qml"));

        if (component.status() != QQmlComponent::Ready)
            qFatal("Component is not ready: %s", component.errorString().toUtf8().constData());

        QObject *instance = component.create();

        if (!instance)
            qFatal("Could not create instance of component.");

        QObject::connect(session, SIGNAL(destroyed()), instance, SLOT(deleteLater()));
        QObject::connect(instance, SIGNAL(destroyed()), engine, SLOT(deleteLater()));

        session->setLogic(instance);
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ViridityDeclarative::registerTypes();

    FileRequestHandler::publishViridityFiles();

    FileRequestHandler::publishFileGlobally("/", ":/index.html");
    FileRequestHandler::publishFileGlobally("/index.html", ":/index.html");

    MySessionManager sessionManager;
    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    ViridityWebServer server(&a, &sessionManager);
    if (server.listen(QHostAddress::Any, dataPort))
        qDebug("Server is now listening on 127.0.0.1 port %d", dataPort);
    else
        qFatal("Could not setup server on 127.0.0.1 %d.", dataPort);

    return a.exec();
}
