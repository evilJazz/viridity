#include "viridityqmlwebserver.h"

#ifdef VIRIDITY_USE_QTQUICK1
    #include <QDeclarativeComponent>
    typedef QDeclarativeEngine DeclarativeEngine;
    typedef QDeclarativeContext DeclarativeContext;
    typedef QDeclarativeComponent DeclarativeComponent;
#else
    #include <QQmlComponent>
    typedef QQmlEngine DeclarativeEngine;
    typedef QQmlContext DeclarativeContext;
    typedef QQmlComponent DeclarativeComponent;
#endif

#include "handlers/filerequesthandler.h"
#include "viriditydeclarative.h"

#include "KCL/debug.h"

/* ViridityQmlWebServer */

ViridityQmlWebServer::ViridityQmlWebServer(QObject *parent)
    : QObject(parent),
      enabled_(false),
      declarativeConstructionRunning_(false)
{

}

void ViridityQmlWebServer::setEnabled(bool enabled)
{
    if (this->enabled() != enabled)
    {
        enabled_ = enabled;
        updateState();
    }
}

QString ViridityQmlWebServer::bindAddress() const
{
    return bindToAddress_.toString();
}

QHostAddress ViridityQmlWebServer::bindHostAddress() const
{
    return bindToAddress_;
}

void ViridityQmlWebServer::setBindAddress(const QString &address)
{
    if (bindToAddress_.toString() != address)
    {
        bindToAddress_.setAddress(address);
        emit bindAddressChanged();
    }
}

void ViridityQmlWebServer::setBindAddress(const QHostAddress &address)
{
    if (bindToAddress_ != address)
    {
        bindToAddress_ = address;
        emit bindAddressChanged();
    }
}

void ViridityQmlWebServer::setPort(int port)
{
    if (port_ != port)
    {
        port_ = port;
        emit portChanged();
    }
}

bool ViridityQmlWebServer::listen()
{
    DGUARDMETHODTIMED;

    if (webServer_.isNull())
    {
        // returns NULL when this ViridityQmlWebServer instance is running from a declarative context
        // ViridityQmlSessionManager will create a new internal QML engine in this case.
        DeclarativeContext *rootContext = DeclarativeEngine::contextForObject(this);
        if (rootContext)
            context_ = new DeclarativeContext(rootContext, this);
        else
            cleanUpContext();

        if (globalLogic_ || sessionLogic_)
            sessionManager_ = new ViridityQmlSessionManager(globalLogic_.data(), sessionLogic_.data(), this, context_.data());
        else
            sessionManager_ = new ViridityQmlSessionManager(globalLogicSource_, sessionLogicSource_, this,  context_.data());

        webServer_ = new ViridityWebServer(this, sessionManager_.data());

        DPRINTF("Initializing web server...");
        ViridityDeclarative::registerTypes();
        FileRequestHandler::publishViridityFiles();
        emit initialized();

        DPRINTF("Starting up global logic...");
        sessionManager_->startUpGlobalLogic();

        DPRINTF("Starting web server...");
        enabled_ = webServer_->listen(bindToAddress_, port_);

        if (!enabled_)
            cleanUp();

        emit enabledChanged();
        emit opened();

        return enabled_;
    }

    return false;
}

bool ViridityQmlWebServer::close()
{
    if (!webServer_.isNull())
    {
        emit closingDown();

        if (webServer_->close())
        {
            cleanUp();
            enabled_ = false;
            emit closed();
            emit enabledChanged();

            return true;
        }
    }

    return false;
}

void ViridityQmlWebServer::cleanUpContext()
{
    if (!context_.isNull())
    {
        delete context_;
        context_ = NULL;
    }
}

void ViridityQmlWebServer::cleanUp()
{
    if (!webServer_.isNull())
    {
        delete webServer_;
        webServer_ = NULL;
    }

    if (!sessionManager_.isNull())
    {
        delete sessionManager_;
        sessionManager_ = NULL;
    }
}

void ViridityQmlWebServer::setGlobalLogic(DeclarativeComponent *globalLogic)
{
    if (!webServer_.isNull())
        close();

    if (globalLogic_ != globalLogic)
    {
        globalLogic_ = globalLogic;
        emit globalLogicChanged();
    }
}

void ViridityQmlWebServer::setSessionLogic(DeclarativeComponent *sessionLogic)
{
    if (!webServer_.isNull())
        close();

    if (sessionLogic_ != sessionLogic)
    {
        sessionLogic_ = sessionLogic;
        emit sessionLogicChanged();
    }
}

DeclarativeEngine *ViridityQmlWebServer::engine()
{
    if (sessionManager_)
        return sessionManager_->engine();
    else
        return NULL;
}

DeclarativeContext *ViridityQmlWebServer::context()
{
    if (sessionManager_)
        return sessionManager_->context();
    else
        return NULL;
}

void ViridityQmlWebServer::setGlobalLogicSource(const QUrl &globalLogicSource)
{
    if (!webServer_.isNull())
        close();

    if (globalLogicSource_ != globalLogicSource)
    {
        globalLogicSource_ = globalLogicSource;
        emit globalLogicSourceChanged();
    }
}

void ViridityQmlWebServer::setSessionLogicSource(const QUrl &sessionLogicSource)
{
    if (!webServer_.isNull())
        close();

    if (sessionLogicSource_ != sessionLogicSource)
    {
        sessionLogicSource_ = sessionLogicSource;
        emit sessionLogicSourceChanged();
    }
}

void ViridityQmlWebServer::classBegin()
{
    declarativeConstructionRunning_ = true;
}

void ViridityQmlWebServer::componentComplete()
{
    declarativeConstructionRunning_ = false;
    updateState();
}

void ViridityQmlWebServer::updateState()
{
    if (declarativeConstructionRunning_)
        return;

    if (!enabled() && !webServer_.isNull())
        close();

    if (enabled() && webServer_.isNull())
        listen();
}
