#ifndef VIRIDITYWEBSERVER_H
#define VIRIDITYWEBSERVER_H

#include "viridity_global.h"

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QThread>

#include "viridityrequesthandler.h"
#include "viriditysessionmanager.h"

/*!
 * \defgroup virid Viridity Base
 * The base classes provide support for bi-directional communication between the server and the client, for instance a web browser.
 */


/*!
    The ViridityWebServer class provides the basic multi-threaded & session-aware Viridity HTTP server.
    \ingroup virid
    \sa ViridityRequestHandler, ViriditySessionManager
*/

class ViridityWebServer : public QTcpServer, private ViridityRequestHandler
{
    Q_OBJECT
public:
    /*!
     * Constructs a Viridity web server instance with a specified session manager used for managing sessions.
     * Does not take ownership of the session manager.
     *
     * \param parent The parent this instance is a child to.
     * \param sessionManager Specifies the session manager to use for handling session.
     */
    explicit ViridityWebServer(QObject *parent, AbstractViriditySessionManager *sessionManager);

    /*! Destroys the web server instance. */
    virtual ~ViridityWebServer();

    /*!
     * Starts the HTTP server.
     * \param address Defines the IP address to bind to.
     * \param port Defines the port number to listen on.
     * \param threadsNumber Defines the number of threads to use for handling incoming connections.
     * \return Returns true if the server was started successfully, otherwise false if the server was
     * not able to bind the port or the address.
     */
    bool listen(const QHostAddress &address, quint16 port, int threadsNumber = QThread::idealThreadCount());

    /*!
     * The current session manager associated with this web server instance.
     * \return The session manager set during construction of the web server instance.
     */
    AbstractViriditySessionManager *sessionManager();

    /*!
     * Register a request handler.
     * \param handler An instance pointer to a class instance implementing the ViridityRequestHandler interface.
     * \sa ViridityRequestHandler
     */
    void registerRequestHandler(ViridityRequestHandler *handler);

    /*!
     * Unregister a request handler.
     * \param handler An instance pointer to a class instance implementing the ViridityRequestHandler interface.
     * \sa ViridityRequestHandler
     */
    void unregisterRequestHandler(ViridityRequestHandler *handler);

private slots:
    void handleNewSessionCreated(ViriditySession *session);

private:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    virtual void incomingConnection(qintptr handle);
#else
    virtual void incomingConnection(int handle);
#endif

    friend class ViridityConnection;
    virtual bool doesHandleRequest(ViridityHttpServerRequest *request);
    virtual void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);

private:
    AbstractViriditySessionManager *sessionManager_;

    QList<QThread *> connectionThreads_;
    int incomingConnectionCount_;

    QList<QThread *> sessionThreads_;

    QList<ViridityRequestHandler *> requestHandlers_;
    ViridityRequestHandler *fileRequestHandler_;
    ViridityRequestHandler *sessionRoutingRequestHandler_;
};

#endif // VIRIDITYWEBSERVER_H
