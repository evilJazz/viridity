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

#include "KCL/debug.h"

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
    // RUNS IN MAIN THREAD! session also currently in main thread, later moved to worker thread by web server!

    QObject *gl = globalLogic();

    DeclarativeContext *context = new DeclarativeContext(engine()->rootContext());

    DeclarativeComponent component(engine(), sessionLogicUrl_);

    if (component.status() != DeclarativeComponent::Ready)
        qFatal("Component is not ready: %s", component.errorString().toUtf8().constData());

    // Make sure, the engine does not take ownership of our session...
    DeclarativeEngine::setObjectOwnership(session, DeclarativeEngine::CppOwnership);

    context->setContextProperty("globalLogic", gl);
    context->setContextProperty("currentSession", session);
    context->setContextProperty("sessionManager", this);

    QObject *sessionLogic = component.create(context);

    if (!sessionLogic)
        qFatal("Could not create instance of component.");

    sessionLogic->setParent(engine());

    connect(session, SIGNAL(destroyed()), sessionLogic, SLOT(deleteLater()));
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
