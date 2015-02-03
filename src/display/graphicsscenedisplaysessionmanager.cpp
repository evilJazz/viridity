#include "graphicsscenedisplaysessionmanager.h"

#include <QCoreApplication>

#include <QUuid>
#include <QCryptographicHash>

//#undef DEBUG
#include "KCL/debug.h"

class MainThreadGateway : public QObject
{
    Q_OBJECT
public:
    MainThreadGateway(QObject *parent = 0) : QObject(parent) {}
    virtual ~MainThreadGateway() {}

    Q_INVOKABLE GraphicsSceneDisplay *createDisplayInstance(const QString &id, const QStringList &params)
    {
        DGUARDMETHODTIMED;
        GraphicsSceneDisplay *display = manager->createDisplayInstance(id, params);
        if (display)
            display->moveToThread(manager->thread());

        return display;
    }

    GraphicsSceneDisplaySessionManager *manager;
};

/* GraphicsSceneDisplaySessionManager */

QList<GraphicsSceneDisplaySessionManager *> GraphicsSceneDisplaySessionManager::activeSessionManagers_;

GraphicsSceneDisplaySessionManager::GraphicsSceneDisplaySessionManager(ViriditySession *session, QObject *parent) :
    QObject(parent),
    session_(session),
    displayMutex_(QMutex::Recursive)
{
    DGUARDMETHODTIMED;
    connect(&cleanupTimer_, SIGNAL(timeout()), this, SLOT(killObsoleteDisplays()));
    cleanupTimer_.start(10000);

    activeSessionManagers_.append(this);

    mainThreadGateway_ = new MainThreadGateway();
    mainThreadGateway_->manager = this;
    mainThreadGateway_->moveToThread(qApp->thread());
}

GraphicsSceneDisplaySessionManager::~GraphicsSceneDisplaySessionManager()
{
    activeSessionManagers_.removeAll(this);
    DGUARDMETHODTIMED;
}

GraphicsSceneDisplay *GraphicsSceneDisplaySessionManager::getNewDisplay(const QString &id, const QStringList &params)
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&displayMutex_);

    // Create new display instance in thread of session manager...
    GraphicsSceneDisplay *display = NULL;
    metaObject()->invokeMethod(
        mainThreadGateway_, "createDisplayInstance",
        mainThreadGateway_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(GraphicsSceneDisplay *, display),
        Q_ARG(const QString &, id),
        Q_ARG(const QStringList &, params)
    );

    if (display)
    {
        displays_.insert(display->id(), display);

        DisplayResource res;
        res.display = display;
        res.lastUsed.restart();
        res.useCount = 1;

        displayResources_.insert(display, res);

        emit newDisplayCreated(display);
    }

    return display;
}

void GraphicsSceneDisplaySessionManager::removeDisplay(GraphicsSceneDisplay *display)
{
    QMutexLocker l(&displayMutex_);
    displays_.remove(display->id());
    displayResources_.remove(display);

    metaObject()->invokeMethod(display, "deleteLater");
}

bool GraphicsSceneDisplaySessionManager::canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    QMutexLocker l(&displayMutex_);
    return
        (targetId.isEmpty() && message.startsWith("newDisplay")) ||
        (displays_.contains(targetId) && static_cast<ViridityMessageHandler *>(displays_.value(targetId))->canHandleMessage(message, sessionId, targetId));
}

bool GraphicsSceneDisplaySessionManager::handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&displayMutex_);

    if (targetId.isEmpty())
    {
        QString command;
        QStringList params;

        ViridityMessageHandler::splitMessage(message, command, params);

        if (command.startsWith("newDisplay"))
        {
            QStringList modParams = params;
            QString id = modParams.takeFirst();

            GraphicsSceneDisplay *display = getNewDisplay(id, modParams);

            if (display)
            {
                connect(display, SIGNAL(updateAvailable()), this, SLOT(handleDisplayUpdateAvailable()));
                session_->dispatchMessageToClient("done()", targetId);
                return true;
            }
        }
    }
    else
    {
        GraphicsSceneDisplay *display = acquireDisplay(targetId);
        if (display)
        {
            bool result = static_cast<ViridityMessageHandler *>(display)->handleMessage(message, sessionId, targetId);
            releaseDisplay(display);
            return result;
        }
    }

    return false;
}

void GraphicsSceneDisplaySessionManager::handleDisplayUpdateAvailable()
{
    DGUARDMETHODTIMED;
    GraphicsSceneDisplay *display = qobject_cast<GraphicsSceneDisplay *>(sender());
    if (display)
        session_->handlerIsReadyForDispatch(display);
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
        DisplayResource &res = displayResources_[display];
        ++res.useCount;
        res.lastUsed.restart();
    }

    return display;
}

void GraphicsSceneDisplaySessionManager::releaseDisplay(GraphicsSceneDisplay *display)
{
    QMutexLocker l(&displayMutex_);

    if (display)
    {
        DisplayResource &res = displayResources_[display];
        --res.useCount;
        res.lastUsed.restart();
    }
}

QList<GraphicsSceneDisplaySessionManager *> GraphicsSceneDisplaySessionManager::activeSessionManagers()
{
    return activeSessionManagers_;
}

void GraphicsSceneDisplaySessionManager::killObsoleteDisplays()
{
    QMutexLocker l(&displayMutex_);

    foreach (const DisplayResource &res, displayResources_.values())
        if (res.useCount == 0 && res.lastUsed.elapsed() > 5000)
            removeDisplay(res.display);
}


/* SingleGraphicsSceneDisplaySessionManager */

SingleGraphicsSceneDisplaySessionManager::SingleGraphicsSceneDisplaySessionManager(ViriditySession *session, QObject *parent, QGraphicsScene *scene) :
    GraphicsSceneDisplaySessionManager(session, parent),
    scene_(scene)
{
    commandInterpreter_ = new GraphicsSceneWebControlCommandInterpreter(this);
    commandInterpreter_->setTargetGraphicsScene(scene_);
}

GraphicsSceneDisplay *SingleGraphicsSceneDisplaySessionManager::createDisplayInstance(const QString &id, const QStringList &params)
{
    DGUARDMETHODTIMED;
    return new GraphicsSceneDisplay(id, scene_, commandInterpreter_);
}

/* MultiGraphicsSceneDisplaySessionManager */

MultiGraphicsSceneDisplaySessionManager::MultiGraphicsSceneDisplaySessionManager(ViriditySession *session, QObject *parent) :
    GraphicsSceneDisplaySessionManager(session, parent)
{

}

GraphicsSceneDisplay *MultiGraphicsSceneDisplaySessionManager::createDisplayInstance(const QString &id, const QStringList &params)
{
    QGraphicsScene *scene = getScene(id, params);

    GraphicsSceneWebControlCommandInterpreter *ci = new GraphicsSceneWebControlCommandInterpreter(scene);
    ci->setTargetGraphicsScene(scene);

    GraphicsSceneDisplay *display = new GraphicsSceneDisplay(id, scene, ci);
    connect(display, SIGNAL(destroyed()), scene, SLOT(deleteLater()));

    return display;
}

#include "graphicsscenedisplaysessionmanager.moc" // for MainThreadGateway
