#include "graphicsscenewebcontrol.h"

#undef DEBUG
#include "KCL/debug.h"

#include "private/commandbridge.h"

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

#include "graphicsscenedisplay.h"

#include "handlers/graphicssceneinputposthandler.h"
#include "handlers/commandposthandler.h"
#include "handlers/websockethandler.h"
#include "handlers/longpollinghandler.h"
#include "handlers/patchrequesthandler.h"
#include "handlers/filerequesthandler.h"

/* GraphicsSceneWebServerThread */

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
GraphicsSceneWebServerConnection::GraphicsSceneWebServerConnection(GraphicsSceneMultiThreadedWebServer *parent, qintptr socketDescriptor) :
#else
GraphicsSceneWebServerConnection::GraphicsSceneWebServerConnection(GraphicsSceneMultiThreadedWebServer *parent, int socketDescriptor) :
#endif
    QObject(),
    webSocketHandler_(NULL),
    longPollingHandler_(NULL),
    patchRequestHandler_(NULL),
    fileRequestHandler_(NULL),
    server_(parent),
    socketDescriptor_(socketDescriptor)
{
    DGUARDMETHODTIMED;
}

GraphicsSceneWebServerConnection::~GraphicsSceneWebServerConnection()
{
    DGUARDMETHODTIMED;
}

void GraphicsSceneWebServerConnection::setupConnection()
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
    longPollingHandler_ = new LongPollingHandler(this);
    patchRequestHandler_ = new PatchRequestHandler(this);
    fileRequestHandler_ = new FileRequestHandler(this);
    fileRequestHandler_->insertFileInformation("/", ":/webcontrol/index.html", "text/html; charset=utf8");
    fileRequestHandler_->insertFileInformation("/index.html", ":/webcontrol/index.html", "text/html; charset=utf8");
    fileRequestHandler_->insertFileInformation("/displayRenderer.js", ":/webcontrol/displayRenderer.js", "application/javascript; charset=utf8");
    fileRequestHandler_->insertFileInformation("/jquery.mousewheel.js", ":/webcontrol/jquery.mousewheel.js", "application/javascript; charset=utf8");
}

void GraphicsSceneWebServerConnection::onRequestReady()
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

void GraphicsSceneWebServerConnection::onUpgrade(const QByteArray &head)
{
    Tufao::HttpServerRequest *request = qobject_cast<Tufao::HttpServerRequest *>(sender());
    webSocketHandler_->handleUpgrade(request, head);
}



/* GraphicsSceneMultiThreadedWebServer */

GraphicsSceneMultiThreadedWebServer::GraphicsSceneMultiThreadedWebServer(QObject *parent, QGraphicsScene *scene) :
    QTcpServer(parent),
    scene_(scene),
    displayMutex_(QMutex::Recursive),
    incomingConnectionCount_(0)
{
    commandInterpreter_.setTargetGraphicsScene(scene_);
}

GraphicsSceneMultiThreadedWebServer::~GraphicsSceneMultiThreadedWebServer()
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
    foreach (QThread* t, displayThreads_)
        t->quit();

    foreach (QThread* t, displayThreads_)
    {
        t->wait();
        delete t;
    }
}

void GraphicsSceneMultiThreadedWebServer::listen(const QHostAddress &address, quint16 port, int threadsNumber)
{
    connectionThreads_.reserve(threadsNumber);

    for (int i = 0; i < threadsNumber; ++i)
    {
        connectionThreads_.append(new QThread(this));
        connectionThreads_.at(i)->start();
    }

    for (int i = 0; i < threadsNumber; ++i)
    {
        displayThreads_.append(new QThread(this));
        displayThreads_.at(i)->start();
    }

    QTcpServer::listen(address, port);
}

void GraphicsSceneMultiThreadedWebServer::addDisplay(GraphicsSceneDisplay *c)
{
    QMutexLocker l(&displayMutex_);
    displays_.insert(c->id(), c);

    int threadIndex = displays_.count() % displayThreads_.count();
    QThread *workerThread = displayThreads_.at(threadIndex);
    c->moveToThread(workerThread); // Move display to thread's event loop

    DPRINTF("New worker thread %p for display id %s", workerThread, c->id().toLatin1().constData());
}

void GraphicsSceneMultiThreadedWebServer::removeDisplay(GraphicsSceneDisplay *c)
{
    QMutexLocker l(&displayMutex_);
    displays_.remove(c->id());
}

GraphicsSceneDisplay *GraphicsSceneMultiThreadedWebServer::getDisplay(const QString &id)
{
    QMutexLocker l(&displayMutex_);
    if (displays_.contains(id))
        return displays_[id];

    return NULL;
}

GraphicsSceneWebControlCommandInterpreter *GraphicsSceneMultiThreadedWebServer::commandInterpreter()
{
    return &commandInterpreter_;
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void GraphicsSceneMultiThreadedWebServer::incomingConnection(qintptr handle)
#else
void GraphicsSceneMultiThreadedWebServer::incomingConnection(int handle)
#endif
{
    DGUARDMETHODTIMED;

    ++incomingConnectionCount_;
    int threadIndex = incomingConnectionCount_ % connectionThreads_.count();

    GraphicsSceneWebServerConnection *connection = new GraphicsSceneWebServerConnection(this, handle);
    connection->moveToThread(connectionThreads_.at(threadIndex)); // Move connection to thread's event loop

    metaObject()->invokeMethod(connection, "setupConnection"); // Dispatch setupConnection call to thread's event loop
}

