#include "viridityqmlappcore.h"

#include <QCoreApplication>

#ifdef VIRIDITY_USE_QTQUICK1
    #include <QtDeclarative>
    #include <QDeclarativeEngine>
    #include <QDeclarativeContext>
    #include <QDeclarativeComponent>
    typedef QDeclarativeEngine DeclarativeEngine;
    typedef QDeclarativeContext DeclarativeContext;
    typedef QDeclarativeComponent DeclarativeComponent;
#else
    #include <QtQml>
    #include <QQmlEngine>
    #include <QQmlContext>
    #include <QQmlComponent>
    typedef QQmlEngine DeclarativeEngine;
    typedef QQmlContext DeclarativeContext;
    typedef QQmlComponent DeclarativeComponent;
#endif

#include "kclplugin.h"

#include "KCL/filesystemutils.h"
#include "KCL/settingsgroup.h"
#include "KCL/logging.h"
#include "KCL/debug.h"

#ifdef KCL_simplebase
#include "KCL/simplebase.h"
#endif

#include "viriditydeclarative.h"
#include "viriditydatabridge.h"

#ifdef Q_OS_LINUX
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#endif

#ifdef VIRIDITY_DEBUG
#include "handlers/debugrequesthandler.h"
#endif

#ifdef APPVERSION_EXISTS
    #include "appversion.h"
#else
    #define VIRIDITY_COMMIT "HEAD"
    #define KCL_COMMIT "HEAD"
    #define MAIN_COMMIT "HEAD"
#endif

void viridityQuitSignalHandler(int s)
{
    qApp->exit();
}


/* ViridityQmlBasicAppCore */

static ViridityQmlBasicAppCore *viridityGlobalAppCore = NULL;

ViridityQmlBasicAppCore::ViridityQmlBasicAppCore(QObject *parent) :
    QObject(parent),
    sessionManager_(NULL),
    server_(NULL)
{
    if (!viridityGlobalAppCore)
        viridityGlobalAppCore = this;
}

ViridityQmlBasicAppCore::~ViridityQmlBasicAppCore()
{
    if (viridityGlobalAppCore == this)
        viridityGlobalAppCore = NULL;
}

ViridityQmlBasicAppCore *ViridityQmlBasicAppCore::instance()
{
    return viridityGlobalAppCore;
}

void ViridityQmlBasicAppCore::installTrapsForDefaultQuittingSignals()
{
#ifdef Q_OS_LINUX
    signal(SIGTERM, viridityQuitSignalHandler);
    signal(SIGINT, viridityQuitSignalHandler);
    signal(SIGHUP, viridityQuitSignalHandler);
#endif
}

bool ViridityQmlBasicAppCore::initialize(const QUrl &globalLogicUrl, const QUrl &sessionLogicUrl)
{
    DGUARDMETHODTIMED;

    if (sessionManager_ || server_)
    {
        qDebug("Session manager and/or server already initialized.");
        return false;
    }

    sessionManager_ = new ViridityQmlSessionManager(
        globalLogicUrl,
        sessionLogicUrl,
        this
    );

    server_ = new ViridityWebServer(this, sessionManager_);

    return true;
}

bool ViridityQmlBasicAppCore::startWebServer(const QHostAddress &address, int dataPort)
{
    DGUARDMETHODTIMED;

    if (!server_)
    {
        qDebug("App core not initialized. Cannot start web server.");
        return false;
    }

    DPRINTF("Initializing web server...");
    initEngine();

    DPRINTF("Starting up global logic...");
    sessionManager_->startUpGlobalLogic();

    // Start web server...
    DPRINTF("Starting web server...");

    return server_->listen(address, dataPort);
}

void ViridityQmlBasicAppCore::stopWebServer()
{
    if (server_)
        server_->close();
}

void ViridityQmlBasicAppCore::initEngine()
{
    DGUARDMETHODTIMED;
    FileRequestHandler::publishViridityFiles();
    ViridityDeclarative::registerTypes();

    sessionManager_->engine()->setObjectOwnership(this, DeclarativeEngine::CppOwnership);
    sessionManager_->engine()->rootContext()->setContextProperty("appCore", this);
}

/* ViridityQmlExtendedAppCore */

ViridityQmlExtendedAppCore::ViridityQmlExtendedAppCore(QObject *parent) :
    ViridityQmlBasicAppCore(parent)
{
}

ViridityQmlExtendedAppCore::~ViridityQmlExtendedAppCore()
{
}

bool ViridityQmlExtendedAppCore::initialize(const QUrl &globalLogicUrl, const QUrl &sessionLogicUrl, const QString &dataLocation)
{
    DGUARDMETHODTIMED;
    if (!ViridityQmlBasicAppCore::initialize(globalLogicUrl, sessionLogicUrl))
        return false;

#ifdef VIRIDITY_DEBUG
    debugRequestHandler_ = QSharedPointer<DebugRequestHandler>(new DebugRequestHandler(server_));
#endif

    rewriteRequestHandler_ = QSharedPointer<RewriteRequestHandler>(new RewriteRequestHandler(server_));

    // Print version info...
    version_ = QString("%1: %2, Viridity: %3, KCL: %4").arg(qApp->applicationName(), MAIN_COMMIT, VIRIDITY_COMMIT, KCL_COMMIT);
    DPRINTF("Version: %s", version_.toUtf8().constData());

    // Initialize settings...
    DPRINTF("Initializing settings...");
    dataLocation_ = dataLocation;
    settingsFileName_ = FileSystemUtils::pathAppend(dataLocation_, "settings.ini");

    if (!FileSystemUtils::fileExists(settingsFileName_))
        settingsFileName_ = FileSystemUtils::pathAppend(qApp->applicationDirPath(), "settings.ini");

    SettingsGroup::setGlobalIniFilename(settingsFileName_);
    settings_ = SettingsGroup::settingsInstance();

    // Read settings...
    DPRINTF("Loading settings from %s...", settingsFileName_.toUtf8().constData());

#ifdef KCL_logging
    debugLogFileName_ = settings_->value("logging/debugLogFileName", FileSystemUtils::pathAppend(qApp->applicationDirPath(), "debug.log")).toString();

/*
    // Enable debug logger...
    if (!debugLogFileName_.isEmpty())
    {
        DPRINTF("Enabling debug logging to file %s...", debugLogFileName_.toUtf8().constData());
        globalLogging.enableLogFile(debugLogFileName_);
        globalLogging.registerHandler();
        setDiagnosticOutputEnabled(true);
        setTimestampsEnabled(true);
    }
    else
        DPRINTF("No debug logging to file enabled...");
*/
#endif

#ifdef KCL_simplebase
    QString databaseType = settings_->value("database/type", "sqlite").toString().toLower();

    // Initialize data storage...
    DPRINTF("Setting up database...");
    if (databaseType.toLower() == "sqlite")
    {
        FileSystemUtils::forceDirectory(dataLocation_);
        QString sqliteFileName = FileSystemUtils::pathAppend(dataLocation_, "sbase.sqlite");

        DPRINTF("Configuring for SQLite backend in file %s...", sqliteFileName.toUtf8().constData());
        SimpleBase::setGlobalDatabaseFilename(sqliteFileName);
    }
    else if (databaseType.toLower() == "mysql")
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName(settings_->value("database/hostname").toString());
        db.setDatabaseName(settings_->value("database/database").toString());
        db.setUserName(settings_->value("database/username").toString());
        db.setPassword(settings_->value("database/password").toString());
        bool ok = db.open();

        if (!ok)
            qFatal("Could not open connection to MySQL database...");

        // SimpleBase will pick up the new default database...
    }
    else
    {
        qDebug("No database configured. Please specify in settings.ini.");
        return false;
    }

    SimpleBase sb;
    sb.loadDatabase();
#endif

    return true;
}

QSharedPointer<RewriteRequestHandler> ViridityQmlExtendedAppCore::rewriteRequestHandler()
{
    return rewriteRequestHandler_;
}

void ViridityQmlExtendedAppCore::initEngine()
{
    DGUARDMETHODTIMED;
    ViridityQmlBasicAppCore::initEngine();

    // Initialize static KCL plugin...
    KCLPlugin *kcl = new KCLPlugin();
    kcl->setParent(sessionManager_->engine());
    kcl->initializeEngine(sessionManager_->engine(), "KCL");
    kcl->registerTypes("KCL");

    server_->registerRequestHandler(rewriteRequestHandler_);

#ifdef VIRIDITY_DEBUG
    server_->registerRequestHandler(debugRequestHandler_);
#endif
}
