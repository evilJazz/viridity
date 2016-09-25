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

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    typedef qintptr ViriditySocketDescriptor;
#else
    typedef int ViriditySocketDescriptor;
#endif

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

    ViriditySocketDescriptor socketDescriptor() const { return socketDescriptor_; }

    ViridityWebServer *server() { return server_; }

    QVariant stats() const;

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

    QTcpSocket *socket_;
    ViridityHttpServerRequest *request_;
    ViriditySocketDescriptor socketDescriptor_;

    QTime created_;
    QTime lastUsed_;
    int reUseCount_;
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
    socket_(NULL),
    request_(NULL),
    socketDescriptor_(socketDescriptor),
    reUseCount_(0)
{
    DGUARDMETHODTIMED;
    created_.start();
}

ViridityConnection::~ViridityConnection()
{
    DGUARDMETHODTIMED;
    server_->removeConnection(this);
}

QVariant ViridityConnection::stats() const
{
    QVariantMap result;

    result.insert("request.url", request_->url());
    result.insert("request.method", request_->method());
    result.insert("socket.peerAddress", socket_->peerAddress().toString());
    result.insert("socketDescriptor", socketDescriptor());
    result.insert("age", created_.elapsed());
    result.insert("lastUsed", lastUsed_.elapsed());
    result.insert("reUseCount", reUseCount_);
    result.insert("livingInThread", thread()->objectName());

    return result;
}

void ViridityConnection::setupConnection()
{
    DGUARDMETHODTIMED;

    // Open socket
    socket_ = new QTcpSocket(this);

    // Attach incomming connection to socket
    if (!socket_->setSocketDescriptor(socketDescriptor_))
    {
        delete socket_;
        this->deleteLater();
        // TODO: Cleanup!
        return;
    }

    // Hand-off incoming connection to Tufao to parse request...
    request_ = new ViridityHttpServerRequest(socket_, this);

    DPRINTF("New connection from %s.", socket_->peerAddress().toString().toLatin1().constData());

    connect(request_, SIGNAL(ready()), this, SLOT(onRequestReady()));
    connect(request_, SIGNAL(upgrade(QByteArray)), this, SLOT(onUpgrade(QByteArray)));

    connect(socket_, SIGNAL(disconnected()), request_, SLOT(deleteLater()));
    connect(socket_, SIGNAL(disconnected()), socket_, SLOT(deleteLater()));

    connect(socket_, SIGNAL(disconnected()), this, SLOT(deleteLater()));
}

void ViridityConnection::onRequestReady()
{
    //DGUARDMETHODTIMED;

    lastUsed_.restart();
    ++reUseCount_;

    ViridityHttpServerRequest *request = qobject_cast<ViridityHttpServerRequest *>(sender());

    DPRINTF("Request for %s of connection from %s ready.", request->url().constData(), request->getPeerAddressFromRequest().constData());

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
    lastUsed_.restart();
    ++reUseCount_;

    ViridityHttpServerRequest *request = qobject_cast<ViridityHttpServerRequest *>(sender());

    DPRINTF("Request for %s of connection from %s wants upgrade.", request->url().constData(), request->getPeerAddressFromRequest().constData());

    if (!webSocketHandler_)
        webSocketHandler_ = new WebSocketHandler(server_, this);

    webSocketHandler_->handleUpgrade(request, head);
}



/* ViridityWebServer */

ViridityWebServer::ViridityWebServer(QObject *parent, AbstractViriditySessionManager *sessionManager) :
    QTcpServer(parent),
    sessionManager_(sessionManager),
    connectionMutex_(QMutex::Recursive),
    incomingConnectionCount_(0)
{
    sessionManager_->setServer(this);
    connect(sessionManager_, SIGNAL(newSessionCreated(ViriditySession*)), this, SLOT(handleNewSessionCreated(ViriditySession*)));

    fileRequestHandler_ = new FileRequestHandler(this, this);
    sessionRoutingRequestHandler_ = new SessionRoutingRequestHandler(this, this);

    registerRequestHandler(fileRequestHandler_);
    registerRequestHandler(sessionRoutingRequestHandler_);
}

ViridityWebServer::~ViridityWebServer()
{
    requestHandlers_.clear();

    foreach (QThread *t, connectionThreads_)
        t->quit();

    foreach (QThread *t, connectionThreads_)
    {
        t->wait();
        delete t;
    }

    foreach (QThread *t, sessionThreads_)
        t->quit();

    foreach (QThread *t, sessionThreads_)
    {
        t->wait();
        delete t;
    }

    connectionThreads_.clear();
    sessionThreads_.clear();
}

bool ViridityWebServer::listen(const QHostAddress &address, quint16 port, int threadsNumber)
{
    threadsNumber = qMax(1, threadsNumber / 2);

    QThread *newThread;

    connectionThreads_.reserve(threadsNumber);
    for (int i = 0; i < threadsNumber; ++i)
    {
        newThread = new QThread(this);
        newThread->setObjectName("VConnThread" + QString::number(i));
        newThread->start();
        connectionThreads_.append(newThread);
    }

    sessionThreads_.reserve(threadsNumber);
    for (int i = 0; i < threadsNumber; ++i)
    {
        newThread = new QThread(this);
        newThread->setObjectName("VSessionThread" + QString::number(i));
        newThread->start();
        sessionThreads_.append(newThread);
    }

    return QTcpServer::listen(address, port);
}

AbstractViriditySessionManager *ViridityWebServer::sessionManager()
{
    return sessionManager_;
}

void ViridityWebServer::handleNewSessionCreated(ViriditySession *session)
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
    QMutexLocker l(&connectionMutex_);
    connections_.append(connection);
    connection->moveToThread(connectionThreads_.at(threadIndex)); // Move connection to thread's event loop

    QMetaObject::invokeMethod(connection, "setupConnection"); // Dispatch setupConnection call to thread's event loop
}

void ViridityWebServer::removeConnection(ViridityConnection *connection)
{
    QMutexLocker l(&connectionMutex_);
    connections_.removeAll(connection);
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

QVariant ViridityWebServer::stats() const
{
    QVariantMap result;

    result.insert("sessionManager", sessionManager_->stats());

    QMutexLocker l(&connectionMutex_);
    QVariantList connectionArray;
    foreach (ViridityConnection *connection, connections_)
        connectionArray.append(connection->stats());

    result.insert("connections", connectionArray);

    return result;
}

#include "viriditywebserver.moc"
