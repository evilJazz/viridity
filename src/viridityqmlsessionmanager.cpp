#include "viridityqmlsessionmanager.h"

#ifdef USE_QTQUICK1
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

ViridityQmlSessionManager::ViridityQmlSessionManager(const QUrl &globalLogicUrl, const QUrl &sessionLogicUrl, QObject *parent) :
    AbstractViriditySessionManager(parent),
    engine_(NULL),
    globalLogic_(NULL),
    globalLogicUrl_(globalLogicUrl),
    sessionLogicUrl_(sessionLogicUrl)
{
    // Make sure, the engine does not take ownership of us if we don't have any parent...
    DeclarativeEngine::setObjectOwnership(this, DeclarativeEngine::CppOwnership);
}

ViridityQmlSessionManager::~ViridityQmlSessionManager()
{
}

void ViridityQmlSessionManager::initSession(ViriditySession *session)
{
    DGUARDMETHODTIMED;
    // RUNS IN MAIN THREAD! session already is in different thread!

    QObject *gl = globalLogic();

    DeclarativeContext *context = new DeclarativeContext(engine()->rootContext());

    DeclarativeComponent component(engine(), sessionLogicUrl_);

    if (component.status() != DeclarativeComponent::Ready)
        qFatal("Component is not ready: %s", component.errorString().toUtf8().constData());

    ViridityQmlSessionWrapper *sessionWrapper = new ViridityQmlSessionWrapper(session);
    DeclarativeEngine::setObjectOwnership(sessionWrapper, DeclarativeEngine::JavaScriptOwnership);

    context->setContextProperty("globalLogic", gl);
    context->setContextProperty("currentSession", sessionWrapper);
    context->setContextProperty("sessionManager", this);
    context->setContextProperty("currentSessionManager", this);

    QObject *sessionLogic = component.create(context);

    if (!sessionLogic)
        qFatal("Could not create instance of component.");

    sessionLogic->setParent(engine());

    connect(session, SIGNAL(destroyed()), sessionLogic, SLOT(deleteLater()));
    connect(session, SIGNAL(destroyed()), sessionWrapper, SLOT(deleteLater()));
    connect(sessionLogic, SIGNAL(destroyed()), context, SLOT(deleteLater()));

    session->setLogic(sessionLogic);
}

QObject *ViridityQmlSessionManager::globalLogic()
{
    if (globalLogicUrl_.isValid() && !globalLogic_)
    {
        DGUARDMETHODTIMED;
        DeclarativeComponent component(engine(), globalLogicUrl_);

        if (component.status() != DeclarativeComponent::Ready)
            qFatal("Component is not ready: %s", component.errorString().toUtf8().constData());

        engine()->rootContext()->setContextProperty("sessionManager", this);
        engine()->rootContext()->setContextProperty("currentSessionManager", this);

        globalLogic_ = component.create(engine()->rootContext());
        globalLogic_->setParent(this);

        // Make sure, the engine does not take ownership of our global logic...
        DeclarativeEngine::setObjectOwnership(globalLogic_, DeclarativeEngine::CppOwnership);

        if (!globalLogic_)
            qFatal("Could not create instance of component.");
    }

    return globalLogic_;
}

DeclarativeEngine *ViridityQmlSessionManager::engine()
{
    if (!engine_)
    {
        DGUARDMETHODTIMED;
        engine_ = new DeclarativeEngine(this);
    }

    return engine_;
}
