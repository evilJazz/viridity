#include "viriditywebserver.h"

//#undef DEBUG
#include "KCL/debug.h"

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

#include <QByteArray>
#include <QBuffer>
#include <QFile>
#include <QThread>

#include <QUuid>
#include <QCryptographicHash>
#include <QMutexLocker>
#include <QUrl>

#include "handlers/inputposthandler.h"
#include "handlers/websockethandler.h"
#include "handlers/ssehandler.h"
#include "handlers/longpollinghandler.h"
#include "handlers/patchrequesthandler.h"
#include "handlers/filerequesthandler.h"

/* GraphicsSceneWebServerThread */

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
ViridityConnection::ViridityConnection(ViridityWebServer *parent, qintptr socketDescriptor) :
#else
ViridityConnection::ViridityConnection(ViridityWebServer *parent, int socketDescriptor) :
#endif
    QObject(),
    webSocketHandler_(NULL),
    sseHandler_(NULL),
    longPollingHandler_(NULL),
//    patchRequestHandler_(NULL),
    fileRequestHandler_(NULL),
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
    Tufao::HttpServerRequest *request = new Tufao::HttpServerRequest(socket, this);

    connect(request, SIGNAL(ready()), this, SLOT(onRequestReady()));
    connect(request, SIGNAL(upgrade(QByteArray)), this, SLOT(onUpgrade(QByteArray)));

    connect(socket, SIGNAL(disconnected()), request, SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));

    connect(socket, SIGNAL(disconnected()), this, SLOT(deleteLater()));

    webSocketHandler_ = new WebSocketHandler(this);
    sseHandler_ = new SSEHandler(this);
    longPollingHandler_ = new LongPollingHandler(this);
    patchRequestHandler_ = new PatchRequestHandler(this);
    fileRequestHandler_ = new FileRequestHandler(this);
    fileRequestHandler_->insertFileInformation("/", ":/Client/index.html", "text/html; charset=utf8");
    fileRequestHandler_->insertFileInformation("/index.html", ":/Client/index.html", "text/html; charset=utf8");
    fileRequestHandler_->insertFileInformation("/DataBridge.js", ":/Client/DataBridge.js", "application/javascript; charset=utf8");
    fileRequestHandler_->insertFileInformation("/DisplayRenderer.js", ":/Client/DisplayRenderer.js", "application/javascript; charset=utf8");
    fileRequestHandler_->insertFileInformation("/Viridity.js", ":/Client/Viridity.js", "application/javascript; charset=utf8");
    fileRequestHandler_->insertFileInformation("/jquery.mousewheel.js", ":/Client/jquery.mousewheel.js", "application/javascript; charset=utf8");
}

void ViridityConnection::onRequestReady()
{
    //DGUARDMETHODTIMED;

    Tufao::HttpServerRequest *request = qobject_cast<Tufao::HttpServerRequest *>(sender());

    QAbstractSocket *socket = request->socket();
    Tufao::HttpServerResponse *response = new Tufao::HttpServerResponse(socket, request->responseOptions(), this);

    connect(socket, SIGNAL(disconnected()), response, SLOT(deleteLater()));
    connect(response, SIGNAL(finished()), response, SLOT(deleteLater()));

    if (request->headers().contains("Expect", "100-continue"))
        response->writeContinue();

    if (fileRequestHandler_->doesHandleRequest(request))
    {
        fileRequestHandler_->handleRequest(request, response);
    }
    else if (sseHandler_->doesHandleRequest(request))
    {
        sseHandler_->handleRequest(request, response);
    }
    else if (longPollingHandler_->doesHandleRequest(request))
    {
        longPollingHandler_->handleRequest(request, response);
    }
    else if (patchRequestHandler_->doesHandleRequest(request))
    {
        patchRequestHandler_->handleRequest(request, response);
    }
    else
    {
        response->writeHead(404);
        response->end("Not found");
    }
}

void ViridityConnection::onUpgrade(const QByteArray &head)
{
    Tufao::HttpServerRequest *request = qobject_cast<Tufao::HttpServerRequest *>(sender());
    webSocketHandler_->handleUpgrade(request, head);
}



/* GraphicsSceneMultiThreadedWebServer */

ViridityWebServer::ViridityWebServer(QObject *parent, ViriditySessionManager *sessionManager) :
    QTcpServer(parent),
    sessionManager_(sessionManager),
    incomingConnectionCount_(0)
{
    connect(sessionManager_, SIGNAL(newSessionCreated(ViriditySession*)), this, SLOT(newSessionCreated(ViriditySession*)));
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

