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
    GraphicsSceneDisplay *display = NULL;
    metaObject()->invokeMethod(
        this, "createDisplayInstance",
        this->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(GraphicsSceneDisplay *, display),
        Q_ARG(const QString &, createUniqueID())
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

void GraphicsSceneDisplaySessionManager::killObsoleteDisplays()
{
    QMutexLocker l(&displayMutex_);

    foreach (const DisplayResource &res, displayResources_.values())
        if (res.useCount == 0 && res.lastUsed.elapsed() > 5000)
            removeDisplay(res.display);
}


/* SingleGraphicsSceneDisplaySessionManager */

SingleGraphicsSceneDisplaySessionManager::SingleGraphicsSceneDisplaySessionManager(QObject *parent, QGraphicsScene *scene) :
    GraphicsSceneDisplaySessionManager(parent),
    scene_(scene)
{
    commandInterpreter_ = new GraphicsSceneWebControlCommandInterpreter(this);
    commandInterpreter_->setTargetGraphicsScene(scene_);
}

GraphicsSceneDisplay *SingleGraphicsSceneDisplaySessionManager::createDisplayInstance(const QString &id)
{
    DGUARDMETHODTIMED;
    return new GraphicsSceneDisplay(id, scene_, commandInterpreter_);
}

/* MultiGraphicsSceneDisplaySessionManager */

MultiGraphicsSceneDisplaySessionManager::MultiGraphicsSceneDisplaySessionManager(QObject *parent) :
    GraphicsSceneDisplaySessionManager(parent)
{

}

GraphicsSceneDisplay *MultiGraphicsSceneDisplaySessionManager::createDisplayInstance(const QString &id)
{
    QGraphicsScene *scene = getScene(id);

    GraphicsSceneWebControlCommandInterpreter *ci = new GraphicsSceneWebControlCommandInterpreter(scene);
    ci->setTargetGraphicsScene(scene);

    GraphicsSceneDisplay *display = new GraphicsSceneDisplay(id, scene, ci);
    connect(display, SIGNAL(destroyed()), scene, SLOT(deleteLater()));

    return display;
}
