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

#ifndef VIRIDITYSESSIONMANAGER_H
#define VIRIDITYSESSIONMANAGER_H

#include <QObject>
#include <QMutex>
#include <QElapsedTimer>
#include <QTimer>
#include <QHash>
#include <QSet>
#include <QVariant>

#include "viridityrequesthandler.h"

class ViridityWebServer;
class AbstractViriditySessionManager;
class ViridityMessageHandler;

/*!
    \addtogroup virid
    @{
*/

/*!
 * The ViriditySession class describes a session context with a remote client.
 *
 * \note Instances are never created directly but are always created by the AbstractViriditySessionManager class internally.
 * Implement AbstractViriditySessionManager::initSession in your session manager class to customize the session via a call to
 * ViriditySession::setLogic.
 */

class ViriditySession : public QObject, public ViridityRequestHandler
{
    Q_OBJECT
    /*! Specifies the identifier of the current session instance. */
    Q_PROPERTY(QString id READ id CONSTANT)

    /*! Specifies the initial peer address.
     * \sa AbstractViriditySessionManager::getNewSession()
     */
    Q_PROPERTY(QByteArray initialPeerAddress READ initialPeerAddress CONSTANT)
public:
    explicit ViriditySession(AbstractViriditySessionManager *sessionManager, const QString &id);
    virtual ~ViriditySession();

    /*!
     * Register a message handler in the context of the current session.
     * \param handler An instance pointer to a class instance implementing the ViridityMessageHandler interface.
     * \sa ViridityMessageHandler
     */
    void registerMessageHandler(ViridityMessageHandler *handler);

    /*!
     * Unregister a message handler in the context of the current session.
     * \param handler An instance pointer to a class instance implementing the ViridityMessageHandler interface.
     * \sa ViridityMessageHandler
     */
    void unregisterMessageHandler(ViridityMessageHandler *handler);

    /*!
     * Register a request handler in the context of the current session.
     * \param handler An instance pointer to a class instance implementing the ViridityRequestHandler interface.
     * \sa ViridityRequestHandler
     */
    void registerRequestHandler(ViridityRequestHandler *handler);

    /*!
     * Unregister a request handler in the context of the current session.
     * \param handler An instance pointer to a class instance implementing the ViridityRequestHandler interface.
     * \sa ViridityRequestHandler
     */
    void unregisterRequestHandler(ViridityRequestHandler *handler);

    /*!
     * Used to dispatch a message to all message handlers in this session.
     * \param message The raw message as QByteArray.
     * \returns Either true if the message was delivered to a message handler or false if no message handler was able to handle the message.
     * \sa ViridityMessageHandler
     */
    Q_INVOKABLE bool dispatchMessageToHandlers(const QByteArray &message);

    /*!
     * Used to dispatch a message to the session's remote client in a push-fashion. Actual delivery might be deferred until the client is ready to accept the message.
     * \note This method does not wait for the message to be delivered and instead returns immediately.
     * \param message The raw message as QByteArray.
     * \param targetId Either the identifier of the target on the remote side or QString::null to broadcast to all targets.
     */
    Q_INVOKABLE void dispatchMessageToClient(const QByteArray &message, const QString &targetId = QString::null);

    /*!
     * Determines if this session has pending messages available that have not yet been posted to the remote side.
     * \note For internal use by the request handlers that provide the communication channel for Viridity, i.e. WebSocketHandler, SSEHandler, LongPollingHandler.
     */
    bool pendingMessagesAvailable() const;

    /*!
     * Returns a list of pending messages for dispatching to the remote side.
     * \param returnBinary Determines that we wish to receive binary data instead of 7-bit-safe data. Depends on the capability of the request handler that provides the communication channel.
     * \note For internal use by the request handlers that provide the communication channel for Viridity, i.e. WebSocketHandler, SSEHandler, LongPollingHandler.
     */
    QList<QByteArray> takePendingMessages(bool returnBinary = false);

    /*!
     * Marks the message handler in this session as having pending messages that are ready for dispatching in a pull-fashion
     * via ViriditySession::takePendingMessages and ViriditySession::pendingMessagesAvailable
     * \note For internal use by the request handlers that provide the communication channel for Viridity, i.e. WebSocketHandler, SSEHandler, LongPollingHandler.
     */
    void handlerIsReadyForDispatch(ViridityMessageHandler *handler);

    /*! Returns the session manager of this session. */
    AbstractViriditySessionManager *sessionManager() { return sessionManager_; }

    /*! Returns the identifier of the current session instance. */
    const QString id() const { return id_; }

    /*!
     * Returns how many active users this session has - essentially the reference count on this session used internally.
     *
     * \sa AbstractViriditySessionManager::getSession, AbstractViriditySessionManager::acquireSession, AbstractViriditySessionManager::releaseSession
     * \sa ViriditySession::lastUsed
     */
    int useCount() const { return useCount_; }

    /*!
     * Returns whether this session is attached to a remote client.
     *
     * \sa ViriditySession::attached, ViriditySession::detached
     */
    bool isAttached() const { return attached_; }

    /*!
     * Returns how many milliseconds ago this session was last used or interacted with - essentially the period used to determine if a session is disposable.
     *
     * \sa AbstractViriditySessionManager::getSession, AbstractViriditySessionManager::acquireSession, AbstractViriditySessionManager::releaseSession
     * \sa ViriditySession::useCount, ViriditySession::interactionCheckInterval
     */
    qint64 lastUsed() const { return lastUsed_.elapsed(); }

    /*!
     * Determines whether this session can be disposed.
     */
    bool canBeDisposed() const { useCount_ == 0 && testingState_ == 0; }

    /*!
     * Determines after how many milliseconds a session is deemed disposable because no interaction happened.
     *
     * \sa ViriditySession::lastUsed
     */
    int interactionCheckInterval() const { return interactionCheckInterval_; }

    /*!
     * Tests if a handler holding this session is still interactive. It triggers the interactionDormant() signal once and
     * waits for any input to arrive in this session for up to the period defined by timeout and returns whether this session
     * is still interactive or not, i.e. whether a remote client is still reacting to our outgoing messages.
     *
     * \sa ViriditySession::lastUsed, ViriditySession::interactionDormant
     */
    bool testForInteractivity(const int timeout = 5000, bool cleanup = false);

    /*!
     * Requests a release of this session from all currently attached remote clients.
     */
    bool requestReleaseAndWait(const int timeout = 5000);

    /*!
     * Returns debug statistics and details.
     */
    QVariant stats() const;

    /*! Associated a logic object as an opaque pointer with this session.
     * \note The session instance does not take ownership of this logic object.
     * Connect the destroyed() signal of the session to the deleteLater() method.
     */
    void setLogic(QObject *logic) { logic_ = logic; }

    /*! Returns the logic object associated with this session as an opaque pointer. */
    QObject *logic() const { return logic_; }

    /*! Specifies the initial peer address.
     * \sa AbstractViriditySessionManager::getNewSession()
     */
    QByteArray initialPeerAddress() const { return initialPeerAddress_; }

    /*! Static helper method that parses the session ID from an input URL. */
    static QString parseIdFromUrl(const QByteArray &url);

    // ViridityRequestHandler
    virtual bool doesHandleRequest(ViridityHttpServerRequest *request);
    virtual void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);

    virtual bool event(QEvent *);

signals:
    /*!
     * Signal is emitted when there are pending messages available,
     * either pushed directly via ViriditySession::dispatchMessageToClient or
     * marked as ready by ViriditySession::handlerIsReadyForDispatch and later pulled via ViriditySession::takePendingMessages
     * \note For internal use by the request handlers that provide the communication channel for Viridity, i.e. WebSocketHandler, SSEHandler, LongPollingHandler.
     */
    void newPendingMessagesAvailable();

    /*!
     * Signal is emitted when the session was initialized and is ready for action.
     * This signal is emitted after the initial ViriditySession::attached()
     */
    void initialized();

    /*!
     * Signal is emitted when the session is attached to a remote client.
     * \note For long polling connections, i.e. connections handled by LongPollingHandler,
     * this signal can bounce due to the nature of the connection, especially with long-running or blocking operations on the client-side.
     *
     * \sa ViriditySession::isAttached
     */
    void attached();

    /*!
     * Signal is emitted when the session has not received any message from a client for some time.
     * Currently this interval is defined by AbstractViriditySessionManager::sessionTimeout()
     *
     * \sa ViriditySession::interactionCheckInterval, ViriditySession::lastUsed
     */
    void interactionDormant();

    /*!
     * Signal is emitted when the session requests all users to release this session.
     */
    void releaseRequired();

    /*!
     * Signal is emitted when the session is detached from a remote client.
     * \note For long polling connections, i.e. connections handled by LongPollingHandler,
     * this signal can bounce due to the nature of the connection, especially with long-running or blocking operations on the client-side.
     *
     * \sa ViriditySession::isAttached
     */
    void detached();

    /*!
     * Signal is emitted before tearing down the session, when no client is attached,
     * ViriditySession::useCount() is zero and AbstractViriditySessionManager::killExpiredSessions()
     * deems it is time to clean up this instance.
     */
    void deinitializing();

private slots:
    void updateCheckTimerTimeout();
    void detachDeferTimerTimeout();
    void interactionCheckTimerTimeout();

    void incrementUseCount();
    void decrementUseCount();
    void resetLastUsed();

private:
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

    QTimer *detachDeferTimer_;
    int detachCheckInterval_;
    bool attached_;

    QTimer *interactionCheckTimer_;
    int interactionCheckInterval_;

    QObject *logic_;
    QTime created_;
    QTime lastUsed_;
    int useCount_;
    int testingState_;

    QByteArray initialPeerAddress_;

    Q_INVOKABLE bool sendMessageToHandlers(const QByteArray &message);
    Q_INVOKABLE void sendMessageToClient(const QByteArray &message, const QString &targetId);

    void setInitialPeerAddress(const QByteArray &address) { initialPeerAddress_ = address; }
};

/*!
 * The ViridityMessageHandler class defines the interface for message handlers on the Viridity communication channel.
 * It provides methods for handling incoming messages, i.e. ViridityMessageHandler::canHandleMessage and ViridityMessageHandler::handleMessage similar to ViridityRequestHandler.
 * It also provides means of producing messages via ViridityMessageHandler::takePendingMessages and ViriditySession::handlerIsReadyForDispatch.
 */

class ViridityMessageHandler
{
public:
    virtual ~ViridityMessageHandler() {}

    /*!
     * Determines whether this class can handle (or wishes to handle) the message.
     * \param message The messsage received from the client/browser via the session.
     * \param sessionId The identifier of the session this message originates from.
     * \param targetId The identifier of the target this message is destined for.
     * \return Return true if this class can handle the message, false otherwise.
     */
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId) = 0;

    /*!
     * Called to handle the message.
     * \param message The messsage received from the client/browser via the session.
     * \param sessionId The identifier of the session this message originates from.
     * \param targetId The identifier of the target this message is destined for.
     * \return Return true if the message was handled, false otherwise.
     */
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId) = 0;

    /*!
     * Override this method to produce outgoing messages after signaling you have pending messages via ViriditySession::handlerIsReadyForDispatch.
     * \param returnBinary Determines that we wish to receive binary data instead of 7-bit-safe data. Depends on the capability of the request handler that provides the communication channel.
     * \return List of message lines of QByteArray.
     * \sa ViriditySession::takePendingMessages, ViriditySession::handlerIsReadyForDispatch
     */
    virtual QList<QByteArray> takePendingMessages(bool returnBinary = false);

    /*! Static helper method to take the target identifier from the raw input message and return it as QString. */
    static QString takeTargetFromMessage(QByteArray &message);

    /*! Static helper to split the message into command and parameter list.
     * \param message The input message.
     * \param command The parsed command as QString.
     * \param params The parsed parameters of the message as QStringList.
     */
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
 * Sessions in Viridity are always associated with a logic object assigned in the pure virtual method AbstractViriditySessionManager::initSession which
 * needs to be implemented by a custom session manager class. Logic objects can be individual to a session or can be shared by different sessions.
 * Logic objects can be any QObject derived class and provide a generic way to attach your custom business logic to one or more sessions as opaque pointer.
 * Messages can be dispatched to sessions sharing the same logic object via the AbstractViriditySessionManager::dispatchMessageToClientMatchingLogic method.
 */

class AbstractViriditySessionManager : public QObject
{
    Q_OBJECT
public:
    /*!
     * Constructs a AbstractViriditySessionManager instance with the specified parent.
     *
     * \param parent The parent this instance is a child to.
     */
    AbstractViriditySessionManager(QObject *parent = 0);
    virtual ~AbstractViriditySessionManager();

    /*!
     * Creates a new session and sets the initial peer address that requested a new session.
     * Internally calls AbstractViriditySessionManager::initSession and AbstractViriditySessionManager::registerHandlers in the same thread context where
     * this session manager instance was created, in most cases the application's main thread.
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

    /*! Used to dispatch a message to a specific session and its message handlers.
     * \param message The raw message as QByteArray.
     * \param sessionId Either the identifier of a specific session or QString::null to address all sessions known to this session manager.
     * \returns Either true if the message was delivered to a message handler or false if no message handler handled the message or if the specified session was not found.
     * \sa ViridityMessageHandler, ViriditySession::dispatchMessageToHandlers
     */
    Q_INVOKABLE bool dispatchMessageToHandlers(const QByteArray &message, const QString &sessionId = QString::null);

    /*! Used to dispatch a message to all remote clients whose sessions share the same logic.
     * \param message The raw message as QByteArray.
     * \param logic The opaque logic pointer used by one or more session.
     * \param targetId Either the identifier of the target on the remote side or QString::null to broadcast to all targets.
     * \returns True if the message was dispatched to at least one session's remote client, false if no matching session was found.
     * \note This method does not wait for the message to be delivered and instead returns immediately.
     * \sa ViriditySession::dispatchMessageToClient
     */
    Q_INVOKABLE bool dispatchMessageToClientMatchingLogic(const QByteArray &message, QObject *logic, const QString &targetId);

    /*! Used to dispatch a message to the remote client(s) of a specific session or all sessions, optionally specifying a target within the session(s).
     * \param message The raw message as QByteArray.
     * \param sessionId Either the identifier of a specific session or QString::null to address all sessions known to this session manager.
     * \param targetId Either the identifier of the target on the remote side or QString::null to broadcast to all targets.
     * \returns True if the message was dispatched to at least one session's remote client, false if no matching session was found.
     * \note This method does not wait for the message to be delivered and instead returns immediately.
     * \sa ViriditySession::dispatchMessageToClient
     */
    Q_INVOKABLE bool dispatchMessageToClient(const QByteArray &message, const QString &sessionId = QString::null, const QString &targetId = QString::null);

    /*! Returns the ViridityWebServer instance this session manager is associated to. */
    ViridityWebServer *server() const { return server_; }

    /*!
     * Returns how many milliseconds to wait for activity until a session is deemed disposable
     */
    int sessionTimeout() const { return sessionTimeout_; }

    /*!
     * Returns debug statistics and details.
     */
    virtual QVariant stats() const;

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
     * This method needs to be implemented in a custom session manager to finish the setup of a new session and associate it with you custom logic object.
     * To associate your logic object with the session call the ViriditySession::setLogic method. The session instance will not take ownership of your logic object.
     *
     * \note This method is always run in the same thread context in which the session manager was created. In most cases this is the application's main thread.
     * \warning The session instance is later moved to a different thread. Be careful not to parent your logic to the session as it will be moved to this new thread as well - except this is what you want.
     * \param session The new session instance.
     */
    virtual void initSession(ViriditySession *session) = 0; // Always executed in thread of session manager

    /*!
     * Override this method to register any request or message handlers with the session.
     * This method is called after AbstractViriditySessionManager::initSession was called.
     *
     * \note This method is always run in the same thread context in which the session manager was created. In most cases this is the application's main thread.
     * \param session The new session instance.
     */
    virtual void registerHandlers(ViriditySession *session); // Always executed in thread of session manager

protected slots:
    /*!
     * Cleans up ViriditySession instances known to this session manager that are expired, i.e.
     * ViriditySession::useCount() is zero and the deadline is reached.
     */
    void killExpiredSessions();

private:
    ViridityWebServer *server_;

    mutable QMutex sessionMutex_;
    QHash<QString, ViriditySession *> sessions_;
    QSet<QString> sessionInKillGracePeriod_;
    QTimer *cleanupTimer_;

    int sessionTimeout_;

    Q_INVOKABLE ViriditySession *createSession(const QString &id, const QByteArray &initialPeerAddress); // Always executed in thread of session manager
    ViriditySession *createNewSessionInstance(const QString &id); // Always executed in thread of session manager
    void removeSession(ViriditySession *session);

    friend class ViridityWebServer;
    void setServer(ViridityWebServer *server) { server_ = server; }
};

/*! @} */

#endif // VIRIDITYSESSIONMANAGER_H
