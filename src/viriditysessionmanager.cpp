#include "viriditysessionmanager.h"

#include <QStringList>

#include <QUuid>
#include <QCryptographicHash>

#include <QThread>

//#undef DEBUG
#include "KCL/debug.h"

QString createUniqueID()
{
    QString uuid = QUuid::createUuid().toString();
    return QString(QCryptographicHash::hash(uuid.toUtf8(), QCryptographicHash::Sha1).toHex());
}


/* ViridityMessageHandler */

QList<QByteArray> ViridityMessageHandler::takePendingMessages()
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

ViriditySession::ViriditySession(ViriditySessionManager *sessionManager, const QString &id) :
    QObject(), // No parent since we move this object into different thread later on...
    sessionManager_(sessionManager),
    id_(id),
    dispatchMutex_(QMutex::Recursive),
    updateCheckInterval_(10),
    logic_(NULL),
    useCount_(0)
{
    DGUARDMETHODTIMED;
    updateCheckTimer_ = new QTimer(this);
    connect(updateCheckTimer_, SIGNAL(timeout()), this, SLOT(updateCheckTimerTimeout()));
    updateCheckTimer_->setSingleShot(false);
    updateCheckTimer_->start(updateCheckInterval_);
}

ViriditySession::~ViriditySession()
{
    DGUARDMETHODTIMED;
}

bool ViriditySession::sendMessageToHandlers(const QByteArray &message)
{
    DGUARDMETHODTIMED;

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

QList<QByteArray> ViriditySession::takePendingMessages()
{
    QMutexLocker l(&dispatchMutex_);

    QList<QByteArray> messages = messages_;

    foreach (ViridityMessageHandler *handler, messageHandlersRequestingMessageDispatch_)
    {
        messages += handler->takePendingMessages();
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

bool ViriditySession::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    bool result = false;

    foreach (ViridityRequestHandler *handler, requestHandlers_)
    {
        result = handler->doesHandleRequest(request);
        if (result) break;
    }

    return result;
}

void ViriditySession::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    foreach (ViridityRequestHandler *handler, requestHandlers_)
        if (handler->doesHandleRequest(request))
            handler->handleRequest(request, response);
}

void ViriditySession::updateCheckTimerTimeout()
{
    DGUARDMETHODTIMED;
    updateCheckTimer_->stop();

    if (pendingMessagesAvailable())
        emit newPendingMessagesAvailable();
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


/* ViriditySessionManager */

ViriditySessionManager::ViriditySessionManager(QObject *parent) :
    QObject(parent),
    server_(NULL),
    sessionMutex_(QMutex::Recursive)
{
    DGUARDMETHODTIMED;
    connect(&cleanupTimer_, SIGNAL(timeout()), this, SLOT(killExpiredSessions()));
    cleanupTimer_.start(5000);
}

ViriditySessionManager::~ViriditySessionManager()
{
    DGUARDMETHODTIMED;
}

ViriditySession *ViriditySessionManager::getNewSession()
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&sessionMutex_);

    // Create new session instance in thread of session manager...
    ViriditySession *session = NULL;

    QMetaObject::invokeMethod(
        this, "createSession",
        this->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(ViriditySession *, session),
        Q_ARG(const QString &, createUniqueID().left(10))
    );

    if (session)
    {
        sessions_.insert(session->id(), session);

        session->lastUsed_.restart();
        session->useCount_ = 1;

        emit newSessionCreated(session);

        return session;
    }
    else
        return NULL;
}

void ViriditySessionManager::removeSession(ViriditySession *session)
{
    QMutexLocker l(&sessionMutex_);
    sessions_.remove(session->id());
    QMetaObject::invokeMethod(session, "deleteLater");
}

ViriditySession *ViriditySessionManager::createNewSessionInstance(const QString &id)
{
    ViriditySession *session = new ViriditySession(this, id);
    return session;
}

void ViriditySessionManager::registerHandlers(ViriditySession *session)
{
}

ViriditySession *ViriditySessionManager::getSession(const QString &id)
{
    QMutexLocker l(&sessionMutex_);
    if (sessions_.contains(id))
        return sessions_[id];

    return NULL;
}

ViriditySession *ViriditySessionManager::acquireSession(const QString &id)
{
    QMutexLocker l(&sessionMutex_);

    ViriditySession *session = getSession(id);

    if (session)
    {
        ++session->useCount_;
        session->lastUsed_.restart();
    }

    return session;
}

void ViriditySessionManager::releaseSession(ViriditySession *session)
{
    QMutexLocker l(&sessionMutex_);

    if (session)
    {
        --session->useCount_;
        session->lastUsed_.restart();
    }
}

ViriditySession *ViriditySessionManager::createSession(const QString &id)
{
    DGUARDMETHODTIMED;
    ViriditySession *session = createNewSessionInstance(id);
    setLogic(session);

    connect(session, SIGNAL(destroyed()), session->logic(), SLOT(deleteLater()));

    registerHandlers(session);

    return session;
}

QStringList ViriditySessionManager::sessionIds(QObject *logic)
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

bool ViriditySessionManager::dispatchMessageToHandlers(const QByteArray &message, const QString &sessionId)
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

bool ViriditySessionManager::dispatchMessageToClientMatchingLogic(const QByteArray &message, QObject *logic, const QString &targetId)
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

bool ViriditySessionManager::dispatchMessageToClient(const QByteArray &message, const QString &sessionId, const QString &targetId)
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

void ViriditySessionManager::killExpiredSessions()
{
    if (sessionMutex_.tryLock())
    {
        sessionMutex_.unlock();
        QMutexLocker l(&sessionMutex_);

        foreach (ViriditySession *session, sessions_.values())
            if (session->useCount() == 0 && session->lastUsed_.elapsed() > 600000)
                removeSession(session);
    }
}
