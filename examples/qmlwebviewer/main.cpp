#ifdef VIRIDITY_USE_QTQUICK1
#include <QApplication>
#else
#include <QGuiApplication>
#endif

#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <Viridity/ViridityWebServer>
#include "viriditydeclarative.h"
#include "viridityqmlsessionmanager.h"
#include "viriditydatabridge.h"
#include "handlers/filerequesthandler.h"

#ifdef VIRIDITY_DEBUG
#define VIRIDITY_DEBUG_REQUESTHANDLER
#endif

#ifdef VIRIDITY_DEBUG_REQUESTHANDLER
#include "handlers/debugrequesthandler.h"
#endif

#include "kclplugin.h"
#include "KCL/filesystemutils.h"
#include "KCL/settingsgroup.h"
#include "KCL/simplebase.h"

int main(int argc, char *argv[])
{
#ifdef VIRIDITY_USE_QTQUICK1
    QApplication a(argc, argv);
#else
    QGuiApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
#endif

    a.setOrganizationDomain("meteorasoftworks.com");
    a.setOrganizationName("Meteora Softworks");
    a.setApplicationName("QMLWebViewer");

    QByteArray settingsFileName = qgetenv("QMLWEBVIEWER_SETTINGS");
    if (!settingsFileName.isEmpty())
        SettingsGroup::setGlobalIniFilename(QFile::decodeName(settingsFileName));

    SettingsGroup::setGlobalCustomSettingsFormat(SimpleBase::simpleBaseFormat());

    if (a.arguments().count() == 1)
    {
        qWarning("Usage: %s <QML-file to serve> [port] [HTML-directory to serve]", QFileInfo(a.applicationFilePath()).fileName().toUtf8().constData());
        return 1;
    }

    if (a.arguments().count() == 4)
    {
        QString htmlDirName = a.arguments().at(3);

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

    FileRequestHandler::publishViridityFiles();

    ViridityDeclarative::registerTypes();

    // We want a session manager with a QML/declarative engine.
    // No global logic required here, so we leave it empty.
    ViridityQmlSessionManager sessionManager(QUrl(), QUrl("qrc:/SessionLogic.qml"));

    // Initialize static KCL plugin...
    KCLPlugin *kcl = new KCLPlugin();
    kcl->setParent(sessionManager.engine());
    kcl->initializeEngine(sessionManager.engine(), "KCL");
    kcl->registerTypes("KCL");

    // Set source QML file...
    QUrl qmlUrl = QFile(a.arguments().at(1)).exists() ? QUrl::fromLocalFile(a.arguments().at(1)) : QUrl(a.arguments().at(1));

    sessionManager.engine()->rootContext()->setContextProperty("qmlSource", qmlUrl);

    // Start up web server...
    const int dataPort = a.arguments().count() > 2 ? a.arguments().at(2).toInt() : 8080;

    ViridityWebServer server(&a, &sessionManager);

    sessionManager.startUpGlobalLogic();

#ifdef VIRIDITY_DEBUG_REQUESTHANDLER
    QSharedPointer<DebugRequestHandler> debugRequestHandler(new DebugRequestHandler(&server));
    server.registerRequestHandler(debugRequestHandler);
#endif

    if (server.listen(QHostAddress::Any, dataPort))
        qDebug("Server is now listening on port %d", dataPort);
    else
    {
        qWarning("Could not setup server on port %d.", dataPort);
        return 1;
    }

    return a.exec();
}
