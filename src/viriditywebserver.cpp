#include "viriditywebserver.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

#include <QByteArray>
#include <QBuffer>
#include <QFile>

#include "handlers/inputposthandler.h"
#include "handlers/websockethandler.h"
#include "handlers/ssehandler.h"
#include "handlers/longpollinghandler.h"
#include "handlers/patchrequesthandler.h"
#include "handlers/filerequesthandler.h"
#include "handlers/sessionroutingrequesthandler.h"

/* ViridityConnection */

class ViridityConnection : public QObject
{
    Q_OBJECT
public:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    explicit ViridityConnection(ViridityWebServer *parent, qintptr socketDescriptor);
#else
    explicit ViridityConnection(ViridityWebServer *parent, int socketDescriptor);
#endif

    virtual ~ViridityConnection();

    ViridityWebServer *server() { return server_; }

public slots:
    void setupConnection();

private slots:
    void onRequestReady();
    void onUpgrade(const QByteArray &);

private:
    WebSocketHandler *webSocketHandler_;
    SSEHandler *sseHandler_;
    LongPollingHandler *longPollingHandler_;

    ViridityWebServer *server_;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    qintptr socketDescriptor_;
#else
    int socketDescriptor_;
#endif
};

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
ViridityConnection::ViridityConnection(ViridityWebServer *parent, qintptr socketDescriptor) :
#else
ViridityConnection::ViridityConnection(ViridityWebServer *parent, int socketDescriptor) :
#endif
    QObject(),
    webSocketHandler_(NULL),
    sseHandler_(NULL),
    longPollingHandler_(NULL),
    server_(parent),
    socketDescriptor_(socketDescriptor)
{
    DGUARDMETHODTIMED;
}

ViridityConnection::~ViridityConnection()
{
    DGUARDMETHODTIMED;
}

void ViridityConnection::setupConnection()
{
    DGUARDMETHODTIMED;

    // Open socket
    QTcpSocket *socket = new QTcpSocket(this);

    // Attach incomming connection to socket
    if (!socket->setSocketDescriptor(socketDescriptor_))
    {
        delete socket;
        this->deleteLater();
        // TODO: Cleanup!
        return;
    }

    // Hand-off incoming connection to Tufao to parse request...
    ViridityHttpServerRequest *request = new ViridityHttpServerRequest(socket, this);

    DPRINTF("New connection from %s.", socket->peerAddress().toString().toLatin1().constData());

    connect(request, SIGNAL(ready()), this, SLOT(onRequestReady()));
    connect(request, SIGNAL(upgrade(QByteArray)), this, SLOT(onUpgrade(QByteArray)));

    connect(socket, SIGNAL(disconnected()), request, SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));

    connect(socket, SIGNAL(disconnected()), this, SLOT(deleteLater()));
}

void ViridityConnection::onRequestReady()
{
    //DGUARDMETHODTIMED;

    ViridityHttpServerRequest *request = qobject_cast<ViridityHttpServerRequest *>(sender());

    DPRINTF("Request for %s of connection from %s ready.", request->url().constData(), ViridityConnection::getPeerAddressFromRequest(request).constData());

    QAbstractSocket *socket = request->socket();
    ViridityHttpServerResponse *response = new ViridityHttpServerResponse(socket, request->responseOptions(), this);

    connect(socket, SIGNAL(disconnected()), response, SLOT(deleteLater()));
    connect(response, SIGNAL(finished()), response, SLOT(deleteLater()));

    if (request->headers().contains("Expect", "100-continue"))
        response->writeContinue();

    if (server_->doesHandleRequest(request))
    {
        server_->handleRequest(request, response);
    }
    else if (SSEHandler::staticDoesHandleRequest(server_, request))
    {
        if (!sseHandler_)
            sseHandler_ = new SSEHandler(server_, this);

        sseHandler_->handleRequest(request, response);
    }
    else if (LongPollingHandler::staticDoesHandleRequest(server_, request))
    {
        if (!longPollingHandler_)
            longPollingHandler_ = new LongPollingHandler(server_, this);

        longPollingHandler_->handleRequest(request, response);
    }
    else
    {
        response->writeHead(404);
        response->end("Not found");
    }
}

void ViridityConnection::onUpgrade(const QByteArray &head)
{
    ViridityHttpServerRequest *request = qobject_cast<ViridityHttpServerRequest *>(sender());

    DPRINTF("Request for %s of connection from %s wants upgrade.", request->url().constData(), ViridityConnection::getPeerAddressFromRequest(request).constData());

    if (!webSocketHandler_)
        webSocketHandler_ = new WebSocketHandler(server_, this);

    webSocketHandler_->handleUpgrade(request, head);
}



/* ViridityWebServer */

ViridityWebServer::ViridityWebServer(QObject *parent, ViriditySessionManager *sessionManager) :
    QTcpServer(parent),
    sessionManager_(sessionManager),
    incomingConnectionCount_(0)
{
    sessionManager_->setServer(this);
    connect(sessionManager_, SIGNAL(newSessionCreated(ViriditySession*)), this, SLOT(newSessionCreated(ViriditySession*)));

    fileRequestHandler_ = new FileRequestHandler(this, this);
    sessionRoutingRequestHandler_ = new SessionRoutingRequestHandler(this, this);

    registerRequestHandler(fileRequestHandler_);
    registerRequestHandler(sessionRoutingRequestHandler_);
}

ViridityWebServer::~ViridityWebServer()
{
    // the thread can't have a parent then...
    foreach (QThread* t, connectionThreads_)
        t->quit();

    foreach (QThread* t, connectionThreads_)
    {
        t->wait();
        delete t;
    }

    // the thread can't have a parent then...
    foreach (QThread* t, sessionThreads_)
        t->quit();

    foreach (QThread* t, sessionThreads_)
    {
        t->wait();
        delete t;
    }
}

bool ViridityWebServer::listen(const QHostAddress &address, quint16 port, int threadsNumber)
{
    connectionThreads_.reserve(threadsNumber);

    for (int i = 0; i < threadsNumber; ++i)
    {
        connectionThreads_.append(new QThread(this));
        connectionThreads_.at(i)->start();
    }

    for (int i = 0; i < threadsNumber; ++i)
    {
        sessionThreads_.append(new QThread(this));
        sessionThreads_.at(i)->start();
    }

    return QTcpServer::listen(address, port);
}

ViriditySessionManager *ViridityWebServer::sessionManager()
{
    return sessionManager_;
}

void ViridityWebServer::newSessionCreated(ViriditySession *session)
{
    DGUARDMETHODTIMED;

    int threadIndex = sessionManager_->sessionCount() % sessionThreads_.count();
    QThread *workerThread = sessionThreads_.at(threadIndex);
    session->moveToThread(workerThread); // Move session to thread's event loop

    DPRINTF("New worker thread %p for session id %s", workerThread, session->id().toLatin1().constData());
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void ViridityWebServer::incomingConnection(qintptr handle)
#else
void ViridityWebServer::incomingConnection(int handle)
#endif
{
    DGUARDMETHODTIMED;

    ++incomingConnectionCount_;
    int threadIndex = incomingConnectionCount_ % connectionThreads_.count();

    ViridityConnection *connection = new ViridityConnection(this, handle);
    connection->moveToThread(connectionThreads_.at(threadIndex)); // Move connection to thread's event loop

    QMetaObject::invokeMethod(connection, "setupConnection"); // Dispatch setupConnection call to thread's event loop
}

void ViridityWebServer::registerRequestHandler(ViridityRequestHandler *handler)
{
    if (requestHandlers_.indexOf(handler) == -1)
        requestHandlers_.append(handler);
}

void ViridityWebServer::unregisterRequestHandler(ViridityRequestHandler *handler)
{
    requestHandlers_.removeAll(handler);
}

bool ViridityWebServer::doesHandleRequest(ViridityHttpServerRequest *request)
{
    bool result = false;

    foreach (ViridityRequestHandler *handler, requestHandlers_)
    {
        result = handler->doesHandleRequest(request);
        if (result) break;
    }

    return result;
}

void ViridityWebServer::handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    foreach (ViridityRequestHandler *handler, requestHandlers_)
        if (handler->doesHandleRequest(request))
            handler->handleRequest(request, response);
}

#include "viriditywebserver.moc"
