#ifndef VIRIDITYSESSIONMANAGER_H
#define VIRIDITYSESSIONMANAGER_H

#include <QObject>
#include <QMutex>
#include <QElapsedTimer>
#include <QTimer>
#include <QHash>

#include "viridityrequesthandler.h"

class ViriditySessionManager;
class ViridityMessageHandler;

class ViriditySession : public QObject, public ViridityRequestHandler
{
    Q_OBJECT
public:
    explicit ViriditySession(ViriditySessionManager *sessionManager, const QString &id);
    virtual ~ViriditySession();

    void registerHandler(ViridityMessageHandler *handler);
    void registerHandlers(const QList<ViridityMessageHandler *> &handlers);
    void unregisterHandler(ViridityMessageHandler *handler);

    Q_INVOKABLE bool dispatchMessageToHandlers(const QByteArray &message);
    Q_INVOKABLE void dispatchMessageToClient(const QByteArray &message, const QString &targetId = QString::null);

    bool pendingMessagesAvailable() const;
    QList<QByteArray> takePendingMessages();

    void handlerIsReadyForDispatch(ViridityMessageHandler *handler);

    ViriditySessionManager *sessionManager() { return sessionManager_; }
    const QString id() const { return id_; }

    static QString parseIdFromUrl(const QByteArray &url);

    // ViridityRequestHandler
    virtual bool doesHandleRequest(Tufao::HttpServerRequest *request);
    virtual void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

signals:
    void newPendingMessagesAvailable();

public:
    QObject *logic;

    QElapsedTimer lastUsed;
    int useCount;

private slots:
    void updateCheckTimerTimeout();

private:
    Q_INVOKABLE bool sendMessageToHandlers(const QByteArray &message);
    Q_INVOKABLE void sendMessageToClient(const QByteArray &message, const QString &targetId);

protected:
    friend class ViriditySessionManager;
    ViriditySessionManager *sessionManager_;
    QString id_;

    QList<ViridityMessageHandler *> messageHandlers_;

    mutable QMutex dispatchMutex_;
    QList<QByteArray> messages_;
    QList<ViridityMessageHandler *> messageHandlersRequestingMessageDispatch_;

    QTimer *updateCheckTimer_;
    int updateCheckInterval_;
    void triggerUpdateCheckTimer();
};

class ViridityMessageHandler
{
public:
    virtual ~ViridityMessageHandler() {}
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId) = 0;
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId) = 0;

    virtual QList<QByteArray> takePendingMessages() {}

    static QString takeTargetFromMessage(QByteArray &message);
    static void splitMessage(const QByteArray &message, QString &command, QStringList &params);
};

class ViriditySessionManager : public QObject
{
    Q_OBJECT
public:
    ViriditySessionManager(QObject *parent = 0);
    virtual ~ViriditySessionManager();

    ViriditySession *getNewSession();

    ViriditySession *getSession(const QString &id);

    ViriditySession *acquireSession(const QString &id);
    void releaseSession(ViriditySession *display);

    QStringList sessionIds(QObject *logic = NULL);

    int sessionCount() const { return sessions_.count(); }

    Q_INVOKABLE bool dispatchMessageToHandlers(const QByteArray &message, const QString &sessionId = QString::null);
    Q_INVOKABLE bool dispatchMessageToClientMatchingLogic(const QByteArray &message, QObject *logic, const QString &targetId);
    Q_INVOKABLE bool dispatchMessageToClient(const QByteArray &message, const QString &sessionId = QString::null, const QString &targetId = QString::null);

signals:
    void newSessionCreated(ViriditySession *session);
    void sessionRemoved(ViriditySession *session);

protected:
    void removeSession(ViriditySession *session);

    virtual ViriditySession *createNewSessionInstance(const QString &id);
    virtual void setLogic(ViriditySession *session) = 0;

protected slots:
    virtual void registerHandlers(ViriditySession *session);
    virtual ViriditySession *createSession(const QString &id) = 0; // Always executed in thread of session manager

private slots:
    void killExpiredSessions();

private:
    QMutex sessionMutex_;
    QHash<QString, ViriditySession *> sessions_;
    QTimer cleanupTimer_;
};

class SingleLogicSessionManager : public ViriditySessionManager
{
    Q_OBJECT
public:
    SingleLogicSessionManager(QObject *parent = 0);
    virtual ~SingleLogicSessionManager();

protected slots:
    virtual ViriditySession *createSession(const QString &id);

private:
    ViriditySession *protoSession_;
};

class MultiLogicSessionManager : public ViriditySessionManager
{
    Q_OBJECT
public:
    MultiLogicSessionManager(QObject *parent = 0);
    virtual ~MultiLogicSessionManager() {}

protected slots:
    virtual ViriditySession *createSession(const QString &id);
};

#endif // VIRIDITYSESSIONMANAGER_H
