#ifndef GRAPHICSSCENEDISPLAYSESSIONMANAGER_H
#define GRAPHICSSCENEDISPLAYSESSIONMANAGER_H

#include <QObject>
#include <QMutex>
#include <QElapsedTimer>
#include <QTimer>
#include <QHash>

class ViriditySessionManager;
class ViridityMessageHandler;

class ViriditySession : public QObject
{
    Q_OBJECT
public:
    explicit ViriditySession(ViriditySessionManager *sessionManager, const QString &id);
    virtual ~ViriditySession();

    void registerHandler(ViridityMessageHandler *handler);
    void registerHandlers(const QList<ViridityMessageHandler *> &handlers);
    void unregisterHandler(ViridityMessageHandler *handler);

    Q_INVOKABLE bool dispatchMessageToHandlers(const QByteArray &message);
    Q_INVOKABLE bool dispatchMessagesToClient(const QByteArray &message);

    bool pendingMessagesAvailable() const;
    QList<QByteArray> getPendingMessages();

    ViriditySessionManager *sessionManager() { return sessionManager_; }
    const QString id() const { return id_; }

signals:
    void newPendingMessagesAvailable();

public:
    QObject *logic;

    QElapsedTimer lastUsed;
    int useCount;

protected:
    friend class ViriditySessionManager;
    ViriditySessionManager *sessionManager_;
    QString id_;

    QList<ViridityMessageHandler *> messageHandlers_;
};

class ViridityMessageHandler
{
public:
    virtual ~ViridityMessageHandler() {}
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId) = 0;
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId) = 0;

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
    Q_INVOKABLE bool dispatchMessagesToClientMatchingLogic(const QByteArray &message, QObject *logic);
    Q_INVOKABLE bool dispatchMessagesToClient(const QByteArray &message, const QString &sessionId = QString::null);

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

#endif // GRAPHICSSCENEDISPLAYSESSIONMANAGER_H
