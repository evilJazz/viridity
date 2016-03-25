#ifndef VIRIDITYSESSIONMANAGER_H
#define VIRIDITYSESSIONMANAGER_H

#include <QObject>
#include <QMutex>
#include <QElapsedTimer>
#include <QTimer>
#include <QHash>

#include "viridityrequesthandler.h"

class ViridityWebServer;
class ViriditySessionManager;
class ViridityMessageHandler;

class ViriditySession : public QObject, public ViridityRequestHandler
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QByteArray initialPeerAddress READ initialPeerAddress CONSTANT)
public:
    explicit ViriditySession(ViriditySessionManager *sessionManager, const QString &id);
    virtual ~ViriditySession();

    void registerMessageHandler(ViridityMessageHandler *handler);
    void unregisterMessageHandler(ViridityMessageHandler *handler);

    void registerRequestHandler(ViridityRequestHandler *handler);
    void unregisterRequestHandler(ViridityRequestHandler *handler);

    Q_INVOKABLE bool dispatchMessageToHandlers(const QByteArray &message);
    Q_INVOKABLE void dispatchMessageToClient(const QByteArray &message, const QString &targetId = QString::null);

    bool pendingMessagesAvailable() const;
    QList<QByteArray> takePendingMessages(bool returnBinary = false);

    void handlerIsReadyForDispatch(ViridityMessageHandler *handler);

    ViriditySessionManager *sessionManager() { return sessionManager_; }
    const QString id() const { return id_; }

    int useCount() const { return useCount_; }

    void setLogic(QObject *logic) { logic_ = logic; }
    QObject *logic() const { return logic_; }

    void setInitialPeerAddress(const QByteArray &address) { initialPeerAddress_ = address; }
    QByteArray initialPeerAddress() const { return initialPeerAddress_; }

    static QString parseIdFromUrl(const QByteArray &url);

    // ViridityRequestHandler
    virtual bool doesHandleRequest(ViridityHttpServerRequest *request);
    virtual void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);

signals:
    void newPendingMessagesAvailable();

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
    QList<ViridityRequestHandler *> requestHandlers_;

    mutable QMutex dispatchMutex_;
    QList<QByteArray> messages_;
    QList<ViridityMessageHandler *> messageHandlersRequestingMessageDispatch_;

    QTimer *updateCheckTimer_;
    int updateCheckInterval_;
    void triggerUpdateCheckTimer();

    QObject *logic_;
    QElapsedTimer lastUsed_;
    int useCount_;

    QByteArray initialPeerAddress_;
};

class ViridityMessageHandler
{
public:
    virtual ~ViridityMessageHandler() {}
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId) = 0;
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId) = 0;

    virtual QList<QByteArray> takePendingMessages(bool returnBinary = false);

    static QString takeTargetFromMessage(QByteArray &message);
    static void splitMessage(const QByteArray &message, QString &command, QStringList &params);
};

class ViriditySessionManager : public QObject
{
    Q_OBJECT
public:
    ViriditySessionManager(QObject *parent = 0);
    virtual ~ViriditySessionManager();

    ViriditySession *getNewSession(const QByteArray &initialPeerAddress);

    ViriditySession *getSession(const QString &id);

    ViriditySession *acquireSession(const QString &id);
    void releaseSession(ViriditySession *display);

    QStringList sessionIds(QObject *logic = NULL);

    int sessionCount() const { return sessions_.count(); }

    Q_INVOKABLE bool dispatchMessageToHandlers(const QByteArray &message, const QString &sessionId = QString::null);
    Q_INVOKABLE bool dispatchMessageToClientMatchingLogic(const QByteArray &message, QObject *logic, const QString &targetId);
    Q_INVOKABLE bool dispatchMessageToClient(const QByteArray &message, const QString &sessionId = QString::null, const QString &targetId = QString::null);

    ViridityWebServer *server() const { return server_; }
    void setServer(ViridityWebServer *server) { server_ = server; }

signals:
    void newSessionCreated(ViriditySession *session);
    void sessionRemoved(ViriditySession *session);
    void noSessions();

protected:
    void removeSession(ViriditySession *session);

    virtual ViriditySession *createNewSessionInstance(const QString &id);
    virtual void setLogic(ViriditySession *session) = 0;

protected slots:
    virtual void registerHandlers(ViriditySession *session);
    virtual ViriditySession *createSession(const QString &id, const QByteArray &initialPeerAddress); // Always executed in thread of session manager

private slots:
    void killExpiredSessions();

private:
    ViridityWebServer *server_;

    QMutex sessionMutex_;
    QHash<QString, ViriditySession *> sessions_;
    QTimer *cleanupTimer_;
};

#endif // VIRIDITYSESSIONMANAGER_H
