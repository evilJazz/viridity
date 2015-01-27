#include "graphicsscenedisplaysessionmanager.h"

#include <QUuid>
#include <QCryptographicHash>

//#undef DEBUG
#include "KCL/debug.h"

QString createUniqueID()
{
    QString uuid = QUuid::createUuid().toString();
    return QString(QCryptographicHash::hash(uuid.toUtf8(), QCryptographicHash::Sha1).toHex());
}

GraphicsSceneDisplaySessionManager::GraphicsSceneDisplaySessionManager(QObject *parent) :
    QObject(parent),
    displayMutex_(QMutex::Recursive)
{
    DGUARDMETHODTIMED;
    connect(&cleanupTimer_, SIGNAL(timeout()), this, SLOT(killObsoleteDisplays()));
    cleanupTimer_.start(10000);
}

GraphicsSceneDisplaySessionManager::~GraphicsSceneDisplaySessionManager()
{
    DGUARDMETHODTIMED;
}

GraphicsSceneDisplay *GraphicsSceneDisplaySessionManager::getNewDisplay()
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&displayMutex_);

    // Create new display instance in thread of session manager...
    GraphicsSceneDisplaySession *resources = NULL;

    QMetaObject::invokeMethod(
        this, "createSession",
        this->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(GraphicsSceneDisplaySession *, resources),
        Q_ARG(const QString &, createUniqueID())
    );

    if (resources && resources->display)
    {
        displays_.insert(resources->display->id(), resources->display);

        resources->lastUsed.restart();
        resources->useCount = 1;

        sessions_.insert(resources->display, resources);

        emit newDisplayCreated(resources->display);

        return resources->display;
    }
    else
        return NULL;
}

void GraphicsSceneDisplaySessionManager::removeDisplay(GraphicsSceneDisplay *display)
{
    QMutexLocker l(&displayMutex_);
    displays_.remove(display->id());

    GraphicsSceneDisplaySession *resources = sessions_.take(display);
    delete resources;

    QMetaObject::invokeMethod(display, "deleteLater");
}

GraphicsSceneDisplaySession *GraphicsSceneDisplaySessionManager::createNewSessionInstance()
{
    return new GraphicsSceneDisplaySession;
}

void GraphicsSceneDisplaySessionManager::registerHandlers(GraphicsSceneDisplaySession *session)
{
    session->commandInterpreter->registerHandlers(session->commandHandlers);
}

GraphicsSceneDisplay *GraphicsSceneDisplaySessionManager::getDisplay(const QString &id)
{
    QMutexLocker l(&displayMutex_);
    if (displays_.contains(id))
        return displays_[id];

    return NULL;
}

GraphicsSceneDisplay *GraphicsSceneDisplaySessionManager::acquireDisplay(const QString &id)
{
    QMutexLocker l(&displayMutex_);

    GraphicsSceneDisplay *display = getDisplay(id);

    if (display)
    {
        GraphicsSceneDisplaySession *res = sessions_[display];
        ++res->useCount;
        res->lastUsed.restart();
    }

    return display;
}

void GraphicsSceneDisplaySessionManager::releaseDisplay(GraphicsSceneDisplay *display)
{
    QMutexLocker l(&displayMutex_);

    if (display)
    {
        GraphicsSceneDisplaySession *res = sessions_[display];
        --res->useCount;
        res->lastUsed.restart();
    }
}

QStringList GraphicsSceneDisplaySessionManager::displaysIds(QGraphicsScene *scene)
{
    QMutexLocker l(&displayMutex_);

    QStringList result;

    foreach (GraphicsSceneDisplaySession *session, sessions_.values())
    {
        if (!scene || session->scene == scene)
            result << session->id;
    }

    return result;
}

void GraphicsSceneDisplaySessionManager::killObsoleteDisplays()
{
    QMutexLocker l(&displayMutex_);

    foreach (GraphicsSceneDisplaySession *res, sessions_.values())
        if (res->useCount == 0 && res->lastUsed.elapsed() > 5000)
            removeDisplay(res->display);
}


/* SingleGraphicsSceneDisplaySessionManager */

SingleGraphicsSceneDisplaySessionManager::SingleGraphicsSceneDisplaySessionManager(QObject *parent) :
    GraphicsSceneDisplaySessionManager(parent),
    protoSession_(NULL)
{
}

SingleGraphicsSceneDisplaySessionManager::~SingleGraphicsSceneDisplaySessionManager()
{
    if (protoSession_)
        delete protoSession_;
}

GraphicsSceneDisplaySession *SingleGraphicsSceneDisplaySessionManager::createSession(const QString &id)
{
    DGUARDMETHODTIMED;

    if (!protoSession_)
    {
        protoSession_ = createNewSessionInstance();
        protoSession_->id = id;
        protoSession_->sessionManager = this;

        setScene(protoSession_);
        protoSession_->scene->setParent(this);

        protoSession_->commandInterpreter = new GraphicsSceneWebControlCommandInterpreter(protoSession_->scene);
        protoSession_->commandInterpreter->setTargetGraphicsScene(protoSession_->scene);

        registerHandlers(protoSession_);
    }

    GraphicsSceneDisplaySession *session = createNewSessionInstance();
    *session = *protoSession_;

    session->id = id;
    session->display = new GraphicsSceneDisplay(id, protoSession_->scene, protoSession_->commandInterpreter);

    return session;
}

/* MultiGraphicsSceneDisplaySessionManager */

MultiGraphicsSceneDisplaySessionManager::MultiGraphicsSceneDisplaySessionManager(QObject *parent) :
    GraphicsSceneDisplaySessionManager(parent)
{

}

GraphicsSceneDisplaySession *MultiGraphicsSceneDisplaySessionManager::createSession(const QString &id)
{
    GraphicsSceneDisplaySession *session = createNewSessionInstance();
    session->id = id;
    session->sessionManager = this;

    setScene(session);

    session->commandInterpreter = new GraphicsSceneWebControlCommandInterpreter(session->scene);
    session->commandInterpreter->setTargetGraphicsScene(session->scene);

    session->display = new GraphicsSceneDisplay(id, session->scene, session->commandInterpreter);
    connect(session->display, SIGNAL(destroyed()), session->scene, SLOT(deleteLater()));

    registerHandlers(session);

    return session;
}
