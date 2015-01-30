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
    id_(id)
{

}

ViriditySession::~ViriditySession()
{

}

bool ViriditySession::dispatchMessageToHandlers(const QByteArray &message)
{
    DGUARDMETHODTIMED;

    foreach (ViridityMessageHandler *handler, messageHandlers_)
    {
        if (handler->canHandleMessage(message, id_))
            return handler->handleMessage(message, id_);
    }

    return false;
}

bool ViriditySession::dispatchMessagesToClient(const QByteArray &message)
{

}

bool ViriditySession::pendingMessagesAvailable() const
{

}

QList<QByteArray> ViriditySession::getPendingMessages()
{

}

void ViriditySession::registerHandler(ViridityMessageHandler *handler)
{
    if (messageHandlers_.indexOf(handler) == -1)
        messageHandlers_.append(handler);
}

void ViriditySession::registerHandlers(const QList<ViridityMessageHandler *> &handlers)
{
    foreach (ViridityMessageHandler *handler, handlers)
        registerHandler(handler);
}

void ViriditySession::unregisterHandler(ViridityMessageHandler *handler)
{
    messageHandlers_.removeAll(handler);
}


/* ViriditySessionManager */

ViriditySessionManager::ViriditySessionManager(QObject *parent) :
    QObject(parent),
    sessionMutex_(QMutex::Recursive)
{
    DGUARDMETHODTIMED;
    connect(&cleanupTimer_, SIGNAL(timeout()), this, SLOT(killExpiredSessions()));
    cleanupTimer_.start(10000);
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
        Q_ARG(const QString &, createUniqueID())
    );

    if (session)
    {
        sessions_.insert(session->id(), session);

        session->lastUsed.restart();
        session->useCount = 1;

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
        ++session->useCount;
        session->lastUsed.restart();
    }

    return session;
}

void ViriditySessionManager::releaseSession(ViriditySession *session)
{
    QMutexLocker l(&sessionMutex_);

    if (session)
    {
        --session->useCount;
        session->lastUsed.restart();
    }
}

QStringList ViriditySessionManager::sessionIds(QObject *logic)
{
    QMutexLocker l(&sessionMutex_);

    QStringList result;

    foreach (ViriditySession *session, sessions_.values())
    {
        if (!logic || session->logic == logic)
            result << session->id_;
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
                Q_ARG(const QByteArray &, message),
                Q_ARG(const QString &, session->id())
            );
    }
    else
    {
        ViriditySession *session = getSession(sessionId);
        QMetaObject::invokeMethod(
            session, "dispatchMessageToHandlers",
            session->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result),
            Q_ARG(const QByteArray &, message),
            Q_ARG(const QString &, session->id())
        );
    }

    return result;
}

bool ViriditySessionManager::dispatchMessagesToClientMatchingLogic(const QByteArray &message, QObject *logic)
{
    QMutexLocker l(&sessionMutex_);

    bool result = false;

    foreach (ViriditySession *session, sessions_.values())
        if (session->logic == logic)
            QMetaObject::invokeMethod(
                session, "dispatchMessageToClient",
                session->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(bool, result),
                Q_ARG(const QByteArray &, message),
                Q_ARG(const QString &, session->id())
            );

    return result;
}

bool ViriditySessionManager::dispatchMessagesToClient(const QByteArray &message, const QString &sessionId)
{
    QMutexLocker l(&sessionMutex_);

    bool result = false;

    if (sessionId.isEmpty())
    {
        foreach (ViriditySession *session, sessions_.values())
            QMetaObject::invokeMethod(
                session, "dispatchMessageToClient",
                session->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(bool, result),
                Q_ARG(const QByteArray &, message),
                Q_ARG(const QString &, session->id())
            );
    }
    else
    {
        ViriditySession *session = getSession(sessionId);
        QMetaObject::invokeMethod(
            session, "dispatchMessageToClient",
            session->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result),
            Q_ARG(const QByteArray &, message),
            Q_ARG(const QString &, session->id())
        );
    }

    return result;
}

void ViriditySessionManager::killExpiredSessions()
{
    QMutexLocker l(&sessionMutex_);

    foreach (ViriditySession *session, sessions_.values())
        if (session->useCount == 0 && session->lastUsed.elapsed() > 5000)
            removeSession(session);
}


/* SingleGraphicsSceneDisplaySessionManager */

SingleLogicSessionManager::SingleLogicSessionManager(QObject *parent) :
    ViriditySessionManager(parent),
    protoSession_(NULL)
{
}

SingleLogicSessionManager::~SingleLogicSessionManager()
{
    if (protoSession_)
        protoSession_->deleteLater();
}

ViriditySession *SingleLogicSessionManager::createSession(const QString &id)
{
    DGUARDMETHODTIMED;

    if (!protoSession_)
    {
        protoSession_ = createNewSessionInstance(id);
        setLogic(protoSession_);
        registerHandlers(protoSession_);
    }

    ViriditySession *session = createNewSessionInstance(id);
    session->logic = protoSession_->logic;
    registerHandlers(session);

    return session;
}

/* MultiGraphicsSceneDisplaySessionManager */

MultiLogicSessionManager::MultiLogicSessionManager(QObject *parent) :
    ViriditySessionManager(parent)
{

}

ViriditySession *MultiLogicSessionManager::createSession(const QString &id)
{
    ViriditySession *session = createNewSessionInstance(id);
    setLogic(session);

    connect(session, SIGNAL(destroyed()), session->logic, SLOT(deleteLater()));

    registerHandlers(session);

    return session;
}
