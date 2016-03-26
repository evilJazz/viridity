#ifndef VIRIDITYSESSIONMANAGER_H
#define VIRIDITYSESSIONMANAGER_H

#include <QObject>
#include <QMutex>
#include <QElapsedTimer>
#include <QTimer>
#include <QHash>

#include "viridityrequesthandler.h"

class ViridityWebServer;
class AbstractViriditySessionManager;
class ViridityMessageHandler;

/*!
    \addtogroup virid
    @{
*/

class ViriditySession : public QObject, public ViridityRequestHandler
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QByteArray initialPeerAddress READ initialPeerAddress CONSTANT)
public:
    explicit ViriditySession(AbstractViriditySessionManager *sessionManager, const QString &id);
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

    AbstractViriditySessionManager *sessionManager() { return sessionManager_; }
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
    friend class AbstractViriditySessionManager;
    AbstractViriditySessionManager *sessionManager_;
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

/*!
 * The AbstractViriditySessionManager class provides the basic session functionality in Viridity.
 * AbstractViriditySessionManager takes care of the whole session life-cycle. It provides means of creating new sessions and re-attaching to existing sessions by a known ID.
 * Existing sessions no longer in use will be automatically disposed of after a pre-defined time.
 *
 * The management methods AbstractViriditySessionManager::getNewSession, AbstractViriditySessionManager::getSession, AbstractViriditySessionManager::acquireSession,
 * AbstractViriditySessionManager::releaseSession are used internally by the request handlers that provide the communication channel for Viridity,
 * i.e. WebSocketHandler, SSEHandler, LongPollingHandler.
 *
 * Sessions in Viridity are always associated with a logic object assigned via the pure virtual method setLogic which
 * needs to be implemented by a custom session manager class. Logic objects can be individual to a session or can be shared by different sessions.
 * Logic objects can be any QObject derived class and provide a generic way to attach your custom business logic to one or more sessions as opaque pointer.
 * Messages can be dispatched to sessions sharing the same logic object via the AbstractViriditySessionManager::dispatchMessageToClientMatchingLogic() method.
 */

class AbstractViriditySessionManager : public QObject
{
    Q_OBJECT
public:
    AbstractViriditySessionManager(QObject *parent = 0);
    virtual ~AbstractViriditySessionManager();

    /*!
     * Creates a new session and sets the initial peer address that requested a new session.
     * Internally calls AbstractViriditySessionManager::createSession() in the same thread context where this session manager instance was created, in most cases the application's main thread.
     * \param initialPeerAddress Specifies the IP address of the peer triggering the initial creation of this session.
     * \return The new session instance.
     */
    ViriditySession *getNewSession(const QByteArray &initialPeerAddress);

    /*!
     * Attempts to return the session instance identified by ID.
     * \param id The known identifier string.
     * \return Returns the session instance if it exists. NULL if no matching session was found.
     */
    ViriditySession *getSession(const QString &id);

    /*!
     * Acquires the resources of the session instance identified by ID. Acquiring the resource will prevent the session from expiring and being disposed of.
     * The mechanism employed is similar to manual reference counting.
     * \param id The known identifier string.
     * \return Returns the session instance if it exists. NULL if no matching session was found.
     */
    ViriditySession *acquireSession(const QString &id);

    /*!
     * Releases the resources of the session instance identified by ID. When released by all callers having previously acquired this resource, this session is no
     * longer protected from expriring and being disposed of. The mechanism employed is similar to manual reference counting.
     * \param id The known identifier string.
     * \return Returns the session instance if it exists. NULL if no matching session was found.
     */
    void releaseSession(ViriditySession *display);

    /*!
     * Returns a list of session identifier strings this session manager currently manages.
     * \param logic Filter sessions by a shared logic object or NULL to return all known session IDs.
     * \return A list of session identifier strings.
     */
    QStringList sessionIds(QObject *logic = NULL);

    /*! Returns the count of sessions currently alive and kicking in this session manager. */
    int sessionCount() const { return sessions_.count(); }

    Q_INVOKABLE bool dispatchMessageToHandlers(const QByteArray &message, const QString &sessionId = QString::null);    
    Q_INVOKABLE bool dispatchMessageToClientMatchingLogic(const QByteArray &message, QObject *logic, const QString &targetId);
    Q_INVOKABLE bool dispatchMessageToClient(const QByteArray &message, const QString &sessionId = QString::null, const QString &targetId = QString::null);

    /*! Returns the ViridityWebServer instance this session manager is associated to. */
    ViridityWebServer *server() const { return server_; }

signals:
    /*!
     * Emitted when a new session instance was successfully created.
     * \param session The new fully initialized session instance.
     */
    void newSessionCreated(ViriditySession *session);

    /*!
     * Emitted when a session instance has expired and was disposed.
     * \param session The session removed from this manager which is still alive and will be deleted later.
     */
    void sessionRemoved(ViriditySession *session);

    /*! Emitted when there are no remaining sessions in this manager. */
    void noSessions();

protected:
    /*!
     * This method needs to be implemented in a custom session manager to finish the setup of a new session and associate it with a logic object.
     * To associate your logic object with the session call the ViriditySession::setLogic() method. The session instance will not take ownership of your logic object.
     *
     * This method is always run in the same thread context in which the session manager was created. In most cases this is the application's main thread.
     * The session instance is later moved to a different thread. Be careful not to parent your logic to the session as it will be moved to this new thread as well.
     * \param session The new session instance.
     */
    virtual void setLogic(ViriditySession *session) = 0; // Always executed in thread of session manager

    /*!
     * Override this method to register any request or message handlers with the session.
     * This method is called after AbstractViriditySessionManager::setLogic() was called.
     *
     * This method is always run in the same thread context in which the session manager was created. In most cases this is the application's main thread.
     * \param session The new session instance.
     */
    virtual void registerHandlers(ViriditySession *session); // Always executed in thread of session manager

private slots:
    void killExpiredSessions();

private:
    ViridityWebServer *server_;

    QMutex sessionMutex_;
    QHash<QString, ViriditySession *> sessions_;
    QTimer *cleanupTimer_;

    Q_INVOKABLE ViriditySession *createSession(const QString &id, const QByteArray &initialPeerAddress); // Always executed in thread of session manager
    ViriditySession *createNewSessionInstance(const QString &id); // Always executed in thread of session manager
    void removeSession(ViriditySession *session);

    friend class ViridityWebServer;
    void setServer(ViridityWebServer *server) { server_ = server; }
};

/*! @} */

#endif // VIRIDITYSESSIONMANAGER_H
