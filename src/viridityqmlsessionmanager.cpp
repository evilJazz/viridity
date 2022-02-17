#include "viridityqmlsessionmanager.h"

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

#include "viriditydeclarative.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

/* ViridityQmlSessionManager */

ViridityQmlSessionManager::ViridityQmlSessionManager(const QUrl &globalLogicUrl, const QUrl &sessionLogicUrl, QObject *parent, DeclarativeContext *context) :
    AbstractViriditySessionManager(parent),
    engine_(NULL),
    externalContext_(context),
    globalLogic_(NULL),
    globalLogicUrl_(globalLogicUrl),
    sessionLogicUrl_(sessionLogicUrl)
{
    // Make sure, the engine does not take ownership of us if we don't have any parent...
    DeclarativeEngine::setObjectOwnership(this, DeclarativeEngine::CppOwnership);
}

ViridityQmlSessionManager::ViridityQmlSessionManager(DeclarativeComponent *globalLogicComponent, DeclarativeComponent *sessionLogicComponent, QObject *parent, DeclarativeContext *context) :
    AbstractViriditySessionManager(parent),
    engine_(NULL),
    externalContext_(context),
    externalGlobalLogicComponent_(globalLogicComponent),
    externalSessionLogicComponent_(sessionLogicComponent),
    globalLogic_(NULL)
{
    // Make sure, the engine does not take ownership of us if we don't have any parent...
    DeclarativeEngine::setObjectOwnership(this, DeclarativeEngine::CppOwnership);
}

ViridityQmlSessionManager::~ViridityQmlSessionManager()
{
}

void ViridityQmlSessionManager::initSession(ViriditySession *session)
{
    if ((sessionLogicUrl_.isValid() || !externalSessionLogicComponent_.isNull()) && session)
    {
        DGUARDMETHODTIMED;

        // RUNS IN MAIN THREAD! session already is in different thread!

        QObject *gl = globalLogic();
        DeclarativeContext *rootContext = gl ? DeclarativeEngine::contextForObject(gl) : context();
        DeclarativeContext *context = new DeclarativeContext(rootContext);

        DeclarativeComponent *component = NULL;

        // Use external component?
        if (!externalSessionLogicComponent_.isNull())
        {
            component = externalSessionLogicComponent_;
        }
        else
        {
            component = new DeclarativeComponent(engine(), sessionLogicUrl_);

            if (component->status() != DeclarativeComponent::Ready)
                qFatal("Component is not ready: %s", component->errorString().toUtf8().constData());
        }

        if (!component)
            qFatal("No session logic component set.");

        ViridityQmlSessionWrapper *sessionWrapper = new ViridityQmlSessionWrapper(session);
        DeclarativeEngine::setObjectOwnership(sessionWrapper, DeclarativeEngine::JavaScriptOwnership);

        context->setContextProperty("globalLogic", gl);
        context->setContextProperty("currentSession", sessionWrapper);
        context->setContextProperty("sessionManager", this);
        context->setContextProperty("currentSessionManager", this);

        QObject *sessionLogic = component->create(context);

        if (!sessionLogic)
            qFatal("Could not create instance of session logic component.");

        sessionLogic->setParent(engine());

        // Component created here instead of externally? Clean up.
        if (externalSessionLogicComponent_.isNull())
        {
            component->deleteLater();
            component = NULL;
        }

        connect(session, SIGNAL(destroyed()), sessionLogic, SLOT(deleteLater()));
        connect(session, SIGNAL(destroyed()), sessionWrapper, SLOT(deleteLater()));
        connect(sessionLogic, SIGNAL(destroyed()), context, SLOT(deleteLater()));

        session->setLogic(sessionLogic);
    }
}

QObject *ViridityQmlSessionManager::globalLogic()
{
    if ((globalLogicUrl_.isValid() || !externalGlobalLogicComponent_.isNull()) && !globalLogic_)
    {
        DGUARDMETHODTIMED;

        DeclarativeComponent *component = NULL;

        // Use external component?
        if (!externalGlobalLogicComponent_.isNull())
        {
            component = externalGlobalLogicComponent_;
        }
        else if (globalLogicUrl_.isValid())
        {
            component = new DeclarativeComponent(engine(), globalLogicUrl_);

            if (component->status() != DeclarativeComponent::Ready)
                qFatal("Component is not ready: %s", component->errorString().toUtf8().constData());
        }

        if (!component)
            qFatal("No global logic component set.");

        context()->setContextProperty("sessionManager", this);
        context()->setContextProperty("currentSessionManager", this);

        globalLogic_ = component->create(context());
        globalLogic_->setParent(this);

        // Make sure, the engine does not take ownership of our global logic...
        DeclarativeEngine::setObjectOwnership(globalLogic_, DeclarativeEngine::CppOwnership);

        // Component created here instead of externally? Clean up.
        if (externalGlobalLogicComponent_.isNull())
        {
            component->deleteLater();
            component = NULL;
        }

        if (!globalLogic_)
            qFatal("Could not create instance of global logic component.");
    }

    return globalLogic_;
}

DeclarativeEngine *ViridityQmlSessionManager::engine()
{
    DGUARDMETHODTIMED;

    if (externalContext_)
    {
        engine_ = externalContext_->engine();
    }
    else if (!engine_)
    {
        engine_ = new DeclarativeEngine(this);
    }

    return engine_;
}

DeclarativeContext *ViridityQmlSessionManager::context()
{
    DGUARDMETHODTIMED;

    if (externalContext_)
        return externalContext_;
    else
        return engine()->rootContext();
}
