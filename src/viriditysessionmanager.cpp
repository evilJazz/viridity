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

#include "viriditysessionmanager.h"

#include <QStringList>

#include <QUuid>
#include <QCryptographicHash>
#include <QEvent>
#include <QEventLoop>

#include <QThread>

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

QString createUniqueID()
{
    QString uuid = QUuid::createUuid().toString();
    return QString(QCryptographicHash::hash(uuid.toUtf8(), QCryptographicHash::Sha1).toHex());
}


/* ViridityMessageHandler */

QList<QByteArray> ViridityMessageHandler::takePendingMessages(bool returnBinary)
{
    return QList<QByteArray>();
}

QString ViridityMessageHandler::takeTargetFromMessage(QByteArray &message)
{
    QByteArray bm = message.left(message.indexOf("("));
    int indexOfMarker = bm.indexOf('>');

    if (indexOfMarker > -1)
    {
        message.remove(0, indexOfMarker + 1);
        return bm.left(indexOfMarker);
    }
    else
        return QString::null;
}

void ViridityMessageHandler::splitMessage(const QByteArray &message, QString &command, QStringList &params)
{
    QString rawMsg = message;

    int paramStartIndex = rawMsg.indexOf("(");
    int paramStopIndex = rawMsg.indexOf(")");

    command = rawMsg.mid(0, paramStartIndex);
    QString rawParams = rawMsg.mid(paramStartIndex + 1, paramStopIndex - paramStartIndex - 1);

    params = rawParams.split(",", QString::KeepEmptyParts);
}


/* ViriditySession */

ViriditySession::ViriditySession(AbstractViriditySessionManager *sessionManager, const QString &id) :
    QObject(), // No parent since we move this object into different thread later on...
    sessionManager_(sessionManager),
    id_(id),
    dispatchMutex_(QMutex::Recursive),
    updateCheckInterval_(10),
    detachCheckInterval_(1000),
    interactionCheckInterval_(sessionManager->sessionTimeout()),
    attached_(false),
    logic_(NULL),
    useCount_(0),
    testingState_(0)
{
    DGUARDMETHODTIMED;

    created_.start();
    lastUsed_.start();

    updateCheckTimer_ = new QTimer(this);
    DOP(updateCheckTimer_->setObjectName("ViriditySessionUpdateCheckTimer"));
    connect(updateCheckTimer_, SIGNAL(timeout()), this, SLOT(updateCheckTimerTimeout()));
    updateCheckTimer_->setSingleShot(false);
    updateCheckTimer_->start(updateCheckInterval_);

    detachDeferTimer_ = new QTimer(this);
    DOP(detachDeferTimer_->setObjectName("ViriditySessionDetachDeferTimer"));
    connect(detachDeferTimer_, SIGNAL(timeout()), this, SLOT(detachDeferTimerTimeout()));
    detachDeferTimer_->setSingleShot(true);
    detachDeferTimer_->setInterval(detachCheckInterval_);

    interactionCheckTimer_ = new QTimer(this);
    DOP(interactionCheckTimer_->setObjectName("ViriditySessionInteractionCheckTimer"));
    connect(interactionCheckTimer_, SIGNAL(timeout()), this, SLOT(interactionCheckTimerTimeout()));
    interactionCheckTimer_->setSingleShot(false);
    interactionCheckTimer_->setInterval(interactionCheckInterval_);
}

ViriditySession::~ViriditySession()
{
    DGUARDMETHODTIMED;
    updateCheckTimer_->stop();
    detachDeferTimer_->stop();
    interactionCheckTimer_->stop();

    delete updateCheckTimer_;
    delete detachDeferTimer_;
    delete interactionCheckTimer_;

    updateCheckTimer_ = NULL;
    detachDeferTimer_ = NULL;
    interactionCheckTimer_ = NULL;
}

bool ViriditySession::sendMessageToHandlers(const QByteArray &message)
{
    DGUARDMETHODTIMED;

    resetLastUsed();

    QByteArray modMsg = message;
    QString targetId = ViridityMessageHandler::takeTargetFromMessage(modMsg);

    bool result = false;

    foreach (ViridityMessageHandler *handler, messageHandlers_)
    {
        if (handler->canHandleMessage(modMsg, id_, targetId))
            result = handler->handleMessage(modMsg, id_, targetId);
    }

    return result;
}

void ViriditySession::sendMessageToClient(const QByteArray &message, const QString &targetId)
{
    QMutexLocker l(&dispatchMutex_);

    if (targetId.isEmpty())
        messages_ += message;
    else
        messages_ += targetId.toUtf8() + ">" + message;

    triggerUpdateCheckTimer();
}

void ViriditySession::incrementUseCount()
{
    DGUARDMETHODTIMED;

    // Note: Has to run in the same thread this session is running.

    if (interactionCheckTimer_)
    {
        bool useCountWasZero = useCount_ == 0;

        ++useCount_;

        resetLastUsed();

        if (useCountWasZero && !attached_)
        {
            attached_ = true;
            emit attached();
        }
    }
}

void ViriditySession::decrementUseCount()
{
    DGUARDMETHODTIMED;

    // Note: Has to run in the same thread this session is running.

    if (interactionCheckTimer_ && detachDeferTimer_)
    {
        --useCount_;

        //resetLastUsed();

        // Use a deferred disconnect to debounce
        // communication channel handler, especially LongPolling.
        detachDeferTimer_->start();
    }
}

void ViriditySession::resetLastUsed()
{
    DGUARDMETHODTIMED;
    lastUsed_.restart();
    interactionCheckTimer_->start();
}

bool ViriditySession::dispatchMessageToHandlers(const QByteArray &message)
{
    DGUARDMETHODTIMED;

    bool result = false;

    QMetaObject::invokeMethod(
        this, "sendMessageToHandlers",
        this->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(bool, result),
        Q_ARG(const QByteArray &, message)
    );

    return result;
}

void ViriditySession::dispatchMessageToClient(const QByteArray &message, const QString &targetId)
{
    DGUARDMETHODTIMED;

    QMetaObject::invokeMethod(
        this, "sendMessageToClient",
        Qt::QueuedConnection,
        Q_ARG(const QByteArray &, message),
        Q_ARG(const QString &, targetId)
    );
}

bool ViriditySession::pendingMessagesAvailable() const
{
    QMutexLocker l(&dispatchMutex_);
    return messages_.count() > 0 || messageHandlersRequestingMessageDispatch_.count() > 0;
}

QList<QByteArray> ViriditySession::takePendingMessages(bool returnBinary)
{
    QMutexLocker l(&dispatchMutex_);

    QList<QByteArray> messages = messages_;

    foreach (ViridityMessageHandler *handler, messageHandlersRequestingMessageDispatch_)
    {
        messages += handler->takePendingMessages(returnBinary);
    }

    messages_.clear();
    messageHandlersRequestingMessageDispatch_.clear();

    return messages;
}

void ViriditySession::handlerIsReadyForDispatch(ViridityMessageHandler *handler)
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&dispatchMutex_);

    if (!messageHandlersRequestingMessageDispatch_.contains(handler))
        messageHandlersRequestingMessageDispatch_.append(handler);

    triggerUpdateCheckTimer();
}

QString ViriditySession::parseIdFromUrl(const QByteArray &url)
{
    QString id = url.mid(1, 10);
    return id;
}

bool ViriditySession::doesHandleRequest(ViridityHttpServerRequest *request)
{
    bool result = false;

    foreach (ViridityRequestHandler *handler, requestHandlers_)
    {
        result = handler->doesHandleRequest(request);
        if (result) break;
    }

    return result;
}

void ViriditySession::handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    foreach (ViridityRequestHandler *handler, requestHandlers_)
        if (handler->doesHandleRequest(request))
            handler->handleRequest(request, response);
}

bool ViriditySession::event(QEvent *ev)
{
    if (ev->type() == QEvent::ThreadChange)
    {
        // Make sure to stop all timers before switching to new thread.
        // This fixes a potential bug in Qt where a timer started in one thread
        // is not stopped when moving to the new thread and causes double triggers.
        // Since it is no longer under control (can't be stopped with stop()),
        // it will emit the timeout() signal long after this object is killed,
        // leading to a crash.
        // TODO: Create reproducable example project and create QTBUG report.
        interactionCheckTimer_->stop();
        updateCheckTimer_->stop();
        detachDeferTimer_->stop();
    }

    return QObject::event(ev);
}

bool ViriditySession::testForInteractivity(const int timeout, bool cleanup)
{
    DGUARDMETHODTIMED;

    DPRINTF("useCount: %d", useCount());

    if (useCount() > 0)
    {
        ++testingState_;

        QElapsedTimer testTime;
        testTime.start();

        QTime comparisonLastUsed = lastUsed_;

        emit interactionDormant();

        QEventLoop el;
        bool timedOut;
        bool noInteraction;
        do
        {
            el.processEvents(QEventLoop::AllEvents, 100);
            timedOut = testTime.elapsed() > timeout;
            noInteraction = lastUsed_ == comparisonLastUsed;
            DPRINTF("compareLastUsed: %s, lastUsed: %s",
                comparisonLastUsed.toString("hh:mm:ss.zzz").toUtf8().constData(),
                lastUsed_.toString("hh:mm:ss.zzz").toUtf8().constData()
            );
        }
        while (!timedOut && noInteraction);

        DPRINTF("timedOut: %s, noInteraction: %s", timedOut ? "true" : "false", noInteraction ? "true" : "false");

        if (cleanup && timedOut && noInteraction)
            requestReleaseAndWait(timeout);

        --testingState_;
        bool result = useCount() > 0;
        return result;
    }
    else
        return false;
}

bool ViriditySession::requestReleaseAndWait(const int timeout)
{
    DGUARDMETHODTIMED;
    if (useCount() > 0)
    {
        ++testingState_;

        QElapsedTimer testTime;
        testTime.start();

        emit releaseRequired();

        QEventLoop el;
        while (testTime.elapsed() < timeout && useCount() != 0)
            el.processEvents(QEventLoop::AllEvents, 100);

        --testingState_;
        bool result = useCount() == 0;
        return result;
    }
    else
        return true;
}

QVariant ViriditySession::stats() const
{
    QVariantMap result;
    result.insert("id", id());
    result.insert("initialPeerAddress", initialPeerAddress());
    result.insert("lastUsed", lastUsed());
    result.insert("age", created_.elapsed());
    result.insert("useCount", useCount());
    result.insert("canBeDisposed", canBeDisposed());
    result.insert("isAttached", isAttached());
    result.insert("livingInThread", thread()->objectName());
    return result;
}

void ViriditySession::updateCheckTimerTimeout()
{
    DGUARDMETHODTIMED;
    updateCheckTimer_->stop();

    if (pendingMessagesAvailable())
        emit newPendingMessagesAvailable();
}

void ViriditySession::detachDeferTimerTimeout()
{
    if (useCount_ == 0)
    {
        attached_ = false;
        emit detached();
    }
}

void ViriditySession::interactionCheckTimerTimeout()
{
    emit interactionDormant();
}

void ViriditySession::triggerUpdateCheckTimer()
{
    if (!updateCheckTimer_->isActive())
        updateCheckTimer_->start(updateCheckInterval_);
}

void ViriditySession::registerMessageHandler(ViridityMessageHandler *handler)
{
    if (messageHandlers_.indexOf(handler) == -1)
        messageHandlers_.append(handler);
}

void ViriditySession::unregisterMessageHandler(ViridityMessageHandler *handler)
{
    messageHandlers_.removeAll(handler);
}

void ViriditySession::registerRequestHandler(ViridityRequestHandler *handler)
{
    if (requestHandlers_.indexOf(handler) == -1)
        requestHandlers_.append(handler);
}

void ViriditySession::unregisterRequestHandler(ViridityRequestHandler *handler)
{
    requestHandlers_.removeAll(handler);
}


/* AbstractViriditySessionManager */

AbstractViriditySessionManager::AbstractViriditySessionManager(QObject *parent) :
    QObject(parent),
    server_(NULL),
    sessionMutex_(QMutex::Recursive),
    sessionTimeout_(10000)
{
    DGUARDMETHODTIMED;
    cleanupTimer_ = new QTimer(this);
    DOP(cleanupTimer_->setObjectName("AbstractViriditySessionManagerCleanupTimer"));
    connect(cleanupTimer_, SIGNAL(timeout()), this, SLOT(killExpiredSessions()));
    cleanupTimer_->start(5000);
}

AbstractViriditySessionManager::~AbstractViriditySessionManager()
{
    DGUARDMETHODTIMED;
    cleanupTimer_->stop();
}

ViriditySession *AbstractViriditySessionManager::getNewSession(const QByteArray &initialPeerAddress)
{
    DGUARDMETHODTIMED;

    // Create new session instance in thread of session manager...
    ViriditySession *session = NULL;

    QMetaObject::invokeMethod(
        this, "createSession",
        this->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(ViriditySession *, session),
        Q_ARG(const QString &, createUniqueID().left(10)),
        Q_ARG(const QByteArray &, initialPeerAddress)
    );

    if (session)
    {
        QMutexLocker l(&sessionMutex_);
        sessions_.insert(session->id(), session);
        QMetaObject::invokeMethod(session, "incrementUseCount");

        return session;
    }
    else
        return NULL;
}

void AbstractViriditySessionManager::removeSession(ViriditySession *session)
{
    DGUARDMETHODTIMED;

    emit session->deinitializing();

    QMutexLocker l(&sessionMutex_);
    sessions_.remove(session->id());
    emit sessionRemoved(session);
    QMetaObject::invokeMethod(session, "deleteLater");
}

ViriditySession *AbstractViriditySessionManager::createNewSessionInstance(const QString &id)
{
    ViriditySession *session = new ViriditySession(this, id);
    return session;
}

void AbstractViriditySessionManager::registerHandlers(ViriditySession *session)
{
}

ViriditySession *AbstractViriditySessionManager::getSession(const QString &id)
{
    QMutexLocker l(&sessionMutex_);
    if (sessions_.contains(id))
    {
        if (!sessionInKillGracePeriod_.contains(id))
            sessionInKillGracePeriod_.insert(id);

        return sessions_[id];
    }

    return NULL;
}

ViriditySession *AbstractViriditySessionManager::acquireSession(const QString &id)
{
    QMutexLocker l(&sessionMutex_);

    ViriditySession *session = getSession(id);

    if (session)
        QMetaObject::invokeMethod(session, "incrementUseCount");

    return session;
}

void AbstractViriditySessionManager::releaseSession(ViriditySession *session)
{
    QMutexLocker l(&sessionMutex_);

    if (session)
        QMetaObject::invokeMethod(session, "decrementUseCount");
}

ViriditySession *AbstractViriditySessionManager::createSession(const QString &id, const QByteArray &initialPeerAddress)
{
    DGUARDMETHODTIMED;
    ViriditySession *session = createNewSessionInstance(id);

    session->setInitialPeerAddress(initialPeerAddress);
    initSession(session);
    registerHandlers(session);

    emit newSessionCreated(session);

    emit session->initialized();

    return session;
}

QStringList AbstractViriditySessionManager::sessionIds(QObject *logic)
{
    QMutexLocker l(&sessionMutex_);

    QStringList result;

    foreach (ViriditySession *session, sessions_.values())
    {
        if (!logic || session->logic() == logic)
            result << session->id();
    }

    return result;
}

bool AbstractViriditySessionManager::dispatchMessageToHandlers(const QByteArray &message, const QString &sessionId)
{
    QMutexLocker l(&sessionMutex_);

    bool result = false;

    if (sessionId.isEmpty())
    {
        foreach (ViriditySession *session, sessions_.values())
            QMetaObject::invokeMethod(
                session, "dispatchMessageToHandlers",
                session->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(bool, result),
                Q_ARG(const QByteArray &, message)
            );
    }
    else
    {
        ViriditySession *session = getSession(sessionId);
        if (session)
        {
            QMetaObject::invokeMethod(
                session, "dispatchMessageToHandlers",
                session->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(bool, result),
                Q_ARG(const QByteArray &, message)
            );
        }
    }

    return result;
}

bool AbstractViriditySessionManager::dispatchMessageToClientMatchingLogic(const QByteArray &message, QObject *logic, const QString &targetId)
{
    QMutexLocker l(&sessionMutex_);

    int dispatched = 0;

    foreach (ViriditySession *session, sessions_.values())
        if (session->logic() == logic)
        {
            QMetaObject::invokeMethod(
                session, "dispatchMessageToClient",
                Qt::QueuedConnection,
                Q_ARG(const QByteArray &, message),
                Q_ARG(const QString &, targetId)
            );
            ++dispatched;
        }

    return dispatched > 0;
}

bool AbstractViriditySessionManager::dispatchMessageToClient(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    QMutexLocker l(&sessionMutex_);

    int dispatched = 0;

    if (sessionId.isEmpty())
    {
        foreach (ViriditySession *session, sessions_.values())
        {
            QMetaObject::invokeMethod(
                session, "dispatchMessageToClient",
                Qt::QueuedConnection,
                Q_ARG(const QByteArray &, message),
                Q_ARG(const QString &, targetId)
            );
            ++dispatched;
        }
    }
    else
    {
        ViriditySession *session = getSession(sessionId);
        if (session)
        {
            QMetaObject::invokeMethod(
                session, "dispatchMessageToClient",
                Qt::QueuedConnection,
                Q_ARG(const QByteArray &, message),
                Q_ARG(const QString &, targetId)
            );
            ++dispatched;
        }
    }

    return dispatched > 0;
}

QVariant AbstractViriditySessionManager::stats() const
{
    QVariantMap result;

    QMutexLocker l(&sessionMutex_);

    result.insert("sessionCount", sessions_.count());

    QVariantList sessionArray;

    foreach (ViriditySession *session, sessions_.values())
        sessionArray.append(session->stats());

    result.insert("sessions", sessionArray);

    return result;
}

void AbstractViriditySessionManager::killExpiredSessions()
{
    DGUARDMETHODTIMED;
    if (sessionMutex_.tryLock())
    {
        cleanupTimer_->stop();

        sessionMutex_.unlock();
        QMutexLocker l(&sessionMutex_);

        foreach (ViriditySession *session, sessions_.values())
        {
            if (session->canBeDisposed() && session->lastUsed_.elapsed() > sessionTimeout_)
            {
                if (sessionInKillGracePeriod_.contains(session->id()))
                {
                    DPRINTF("Session %s is still in grace period. Only removing from list.", session->id().toUtf8().constData());
                    sessionInKillGracePeriod_.remove(session->id());
                }
                else
                {
                    DPRINTF("Removing session %s due to expiration.", session->id().toUtf8().constData());
                    removeSession(session);
                }
            }
        }

        if (sessions_.count() == 0)
            emit noSessions();

        cleanupTimer_->start();
    }
}
