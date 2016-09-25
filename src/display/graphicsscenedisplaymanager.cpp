/****************************************************************************
**
** Copyright (C) 2012-2016 Andre Beckedorf, Meteora Softworks
** Contact: info@meteorasoftworks.com
**
** This file is part of Viridity
**
** $VIRIDITY_BEGIN_LICENSE:COMMERCIAL_AGPL$
**
** This library is licensed under either a separately available commercial
** license or the GNU Affero General Public License Version 3.0,
** published 19 November 2007.
**
** See https://www.gnu.org/licenses/agpl-3.0.html or LICENSE-agpl-3.0.txt for
** details.
**
** If you wish to use and distribute the Viridity library in your commercial
** product without making your sourcecode available to the public, please
** contact us for a commercial license at info@meteorasoftworks.com
**
** $VIRIDITY_END_LICENSE$
**
****************************************************************************/

#include "graphicsscenedisplaymanager.h"

#include <QCoreApplication>

#include <QUuid>
#include <QCryptographicHash>

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

class MainThreadGateway : public QObject
{
    Q_OBJECT
public:
    MainThreadGateway(QObject *parent = 0) : QObject(parent) { DGUARDMETHODTIMED; }
    virtual ~MainThreadGateway() { DGUARDMETHODTIMED; }

    Q_INVOKABLE GraphicsSceneDisplay *createDisplayInstance(const QString &id, const QStringList &params)
    {
        DGUARDMETHODTIMED;
        GraphicsSceneDisplay *display = manager->createDisplayInstance(id, params);
        if (display)
            display->moveToThread(manager->thread());

        return display;
    }

    Q_INVOKABLE void tearDownDisplayInstance(GraphicsSceneDisplay *display)
    {
        DGUARDMETHODTIMED;
        manager->tearDownDisplayInstance(display);
    }

    AbstractGraphicsSceneDisplayManager *manager;
};

/* AbstractGraphicsSceneDisplayManager */

QList<AbstractGraphicsSceneDisplayManager *> AbstractGraphicsSceneDisplayManager::activeDisplayManagers_;

AbstractGraphicsSceneDisplayManager::AbstractGraphicsSceneDisplayManager(ViriditySession *session, QObject *parent) :
    QObject(parent),
    session_(session),
    displayMutex_(QMutex::Recursive)
{
    DGUARDMETHODTIMED;
    cleanupTimer_ = new QTimer(this);
    DOP(cleanupTimer_->setObjectName("AbstractGraphicsSceneDisplayManagerCleanupTimer"));
    connect(cleanupTimer_, SIGNAL(timeout()), this, SLOT(killObsoleteDisplays()));
    cleanupTimer_->start(10000);

    activeDisplayManagers_.append(this);

    mainThreadGateway_ = new MainThreadGateway();
    mainThreadGateway_->manager = this;
    mainThreadGateway_->moveToThread(qApp->thread());

    patchRequestHandler_ = new PatchRequestHandler(session->sessionManager()->server(), this);
    session->registerRequestHandler(patchRequestHandler_);
    session->registerMessageHandler(this);

    connect(session, SIGNAL(destroyed()), this, SLOT(handleSessionDestroyed()), Qt::DirectConnection);
}

AbstractGraphicsSceneDisplayManager::~AbstractGraphicsSceneDisplayManager()
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&displayMutex_);

    cleanupTimer_->stop();

    if (session_)
    {
        session_->unregisterMessageHandler(this);
        session_->unregisterRequestHandler(patchRequestHandler_);
    }

    activeDisplayManagers_.removeAll(this);

    killAllDisplays();

    QMetaObject::invokeMethod(mainThreadGateway_, "deleteLater");
}

void AbstractGraphicsSceneDisplayManager::handleSessionDestroyed()
{
    session_ = NULL;
}

GraphicsSceneDisplay *AbstractGraphicsSceneDisplayManager::getNewDisplay(const QString &id, const QStringList &params)
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

void AbstractGraphicsSceneDisplayManager::removeDisplay(GraphicsSceneDisplay *display)
{
    QMutexLocker l(&displayMutex_);
    displays_.remove(display->id());
    displayResources_.remove(display);

    metaObject()->invokeMethod(
        mainThreadGateway_, "tearDownDisplayInstance",
        mainThreadGateway_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_ARG(GraphicsSceneDisplay *, display)
    );

    delete display;
}

void AbstractGraphicsSceneDisplayManager::tearDownDisplayInstance(GraphicsSceneDisplay *display)
{
    DGUARDMETHODTIMED;
}

bool AbstractGraphicsSceneDisplayManager::canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    QMutexLocker l(&displayMutex_);
    return
        message.startsWith("newDisplay") ||
        (displays_.contains(targetId) && static_cast<ViridityMessageHandler *>(displays_.value(targetId))->canHandleMessage(message, sessionId, targetId));
}

bool AbstractGraphicsSceneDisplayManager::handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&displayMutex_);

    GraphicsSceneDisplay *display = getDisplay(targetId);

    if (!display)
    {
        QString command;
        QStringList params;

        ViridityMessageHandler::splitMessage(message, command, params);

        if (command.startsWith("newDisplay"))
        {
            QStringList modParams = params;
            GraphicsSceneDisplay *display = getNewDisplay(targetId, modParams);

            if (display)
            {
                connect(display, SIGNAL(updateAvailable()), this, SLOT(handleDisplayUpdateAvailable()));
                session_->dispatchMessageToClient("done()", targetId);
                releaseDisplay(display);
                return true;
            }
        }
    }
    else
    {
        display = acquireDisplay(targetId);
        if (display)
        {
            // Are we reattaching?
            if (message.startsWith("newDisplay"))
            {
                DPRINTF("Reattaching to session %s, newDisplay received...", sessionId.toLatin1().constData());

                // Ignore all params and send full update...
                display->requestFullUpdate(true);
                releaseDisplay(display);
                return true;
            }
            else
            {
                bool result = static_cast<ViridityMessageHandler *>(display)->handleMessage(message, sessionId, targetId);
                releaseDisplay(display);
                return result;
            }
        }
    }

    return false;
}

void AbstractGraphicsSceneDisplayManager::handleDisplayUpdateAvailable()
{
    DGUARDMETHODTIMED;
    GraphicsSceneDisplay *display = qobject_cast<GraphicsSceneDisplay *>(sender());
    if (display)
        session_->handlerIsReadyForDispatch(display);
}

GraphicsSceneDisplay *AbstractGraphicsSceneDisplayManager::getDisplay(const QString &id)
{
    QMutexLocker l(&displayMutex_);
    if (displays_.contains(id))
        return displays_[id];

    return NULL;
}

GraphicsSceneDisplay *AbstractGraphicsSceneDisplayManager::acquireDisplay(const QString &id)
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

void AbstractGraphicsSceneDisplayManager::releaseDisplay(GraphicsSceneDisplay *display)
{
    QMutexLocker l(&displayMutex_);

    if (display)
    {
        DisplayResource &res = displayResources_[display];
        --res.useCount;
        res.lastUsed.restart();
    }
}

QList<AbstractGraphicsSceneDisplayManager *> AbstractGraphicsSceneDisplayManager::activeDisplayManagers()
{
    return activeDisplayManagers_;
}

void AbstractGraphicsSceneDisplayManager::killObsoleteDisplays()
{
    QMutexLocker l(&displayMutex_);

    foreach (const DisplayResource &res, displayResources_.values())
    {
        if (res.useCount == 0 && res.lastUsed.elapsed() > 120000)
        {
            removeDisplay(res.display);
        }
    }
}

void AbstractGraphicsSceneDisplayManager::killAllDisplays()
{
    QMutexLocker l(&displayMutex_);

    foreach (const DisplayResource &res, displayResources_.values())
        removeDisplay(res.display);
}

/* SingleGraphicsSceneDisplayManager */

SingleGraphicsSceneDisplayManager::SingleGraphicsSceneDisplayManager(ViriditySession *session, QObject *parent, AbstractGraphicsSceneAdapter *adapter) :
    AbstractGraphicsSceneDisplayManager(session, parent),
    adapter_(adapter)
{
    DGUARDMETHODTIMED;
    commandInterpreter_ = new GraphicsSceneDisplayCommandInterpreter(this);
    commandInterpreter_->setTargetGraphicsSceneAdapter(adapter_);
}

SingleGraphicsSceneDisplayManager::~SingleGraphicsSceneDisplayManager()
{
    DGUARDMETHODTIMED;
}

GraphicsSceneDisplay *SingleGraphicsSceneDisplayManager::createDisplayInstance(const QString &id, const QStringList &params)
{
    DGUARDMETHODTIMED;
    GraphicsSceneDisplay *display = new GraphicsSceneDisplay(id, adapter_, commandInterpreter_);
    display->setEncoderSettings(es_);
    display->setComparerSettings(cs_);
    return display;
}

/* MultiGraphicsSceneDisplayManager */

AbstractMultiGraphicsSceneDisplayManager::AbstractMultiGraphicsSceneDisplayManager(ViriditySession *session, QObject *parent) :
    AbstractGraphicsSceneDisplayManager(session, parent)
{
    DGUARDMETHODTIMED;
}

AbstractMultiGraphicsSceneDisplayManager::~AbstractMultiGraphicsSceneDisplayManager()
{
    DGUARDMETHODTIMED;
}

GraphicsSceneDisplay *AbstractMultiGraphicsSceneDisplayManager::createDisplayInstance(const QString &id, const QStringList &params)
{
    DGUARDMETHODTIMED;
    AbstractGraphicsSceneAdapter *adapter = getAdapter(id, params);

    if (adapter)
    {
        GraphicsSceneDisplayCommandInterpreter *ci = new GraphicsSceneDisplayCommandInterpreter(adapter);
        ci->setTargetGraphicsSceneAdapter(adapter);

        GraphicsSceneDisplay *display = new GraphicsSceneDisplay(id, adapter, ci);
        return display;
    }
    else
        return NULL;
}

void AbstractMultiGraphicsSceneDisplayManager::tearDownDisplayInstance(GraphicsSceneDisplay *display)
{
    DGUARDMETHODTIMED;
    tearDownAdapter(display->id(), display->adapter());
}

void AbstractMultiGraphicsSceneDisplayManager::tearDownAdapter(const QString &id, AbstractGraphicsSceneAdapter *adapter)
{
    DGUARDMETHODTIMED;
    delete adapter;
}

#include "graphicsscenedisplaymanager.moc" // for MainThreadGateway
