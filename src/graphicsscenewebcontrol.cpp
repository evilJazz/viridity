#include "graphicsscenewebcontrol.h"

//#undef DEBUG
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

/* GraphicsSceneWebServerThread */

GraphicsSceneWebServerTask::GraphicsSceneWebServerTask(GraphicsSceneMultiThreadedWebServer *parent, int socketDescriptor) :
    EventLoopTask(),
    server_(parent),
    socketDescriptor_(socketDescriptor),
    webSocketHandler_(NULL),
    longPollingHandler_(NULL),
    patchRequestHandler_(NULL)
{
    connect(this, SIGNAL(started(Task*, QThread*)), this, SLOT(setupConnection()));
}

void GraphicsSceneWebServerTask::setupConnection()
{
    DGUARDMETHODTIMED;

    QTcpSocket *socket = new QTcpSocket(this);

    if (!socket->setSocketDescriptor(socketDescriptor_))
    {
        delete socket;
        return;
    }

    Tufao::HttpServerRequest *handle = new Tufao::HttpServerRequest(socket, this);

    connect(handle, SIGNAL(ready()), this, SLOT(onRequestReady()));
    connect(handle, SIGNAL(upgrade(QByteArray)), this, SLOT(onUpgrade(QByteArray)));
    connect(socket, SIGNAL(disconnected()), handle, SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));

    connect(socket, SIGNAL(disconnected()), this, SLOT(quit()));

    webSocketHandler_ = new WebSocketHandler(this);
    longPollingHandler_ = new LongPollingHandler(this);
    patchRequestHandler_ = new PatchRequestHandler(this);
}

void GraphicsSceneWebServerTask::onRequestReady()
{
    DGUARDMETHODTIMED;

    Tufao::HttpServerRequest *request = qobject_cast<Tufao::HttpServerRequest *>(sender());

    QAbstractSocket *socket = request->socket();
    Tufao::HttpServerResponse *response = new Tufao::HttpServerResponse(socket, request->responseOptions(), this);

    connect(socket, SIGNAL(disconnected()), response, SLOT(deleteLater()));
    connect(response, SIGNAL(finished()), response, SLOT(deleteLater()));

    if (request->headers().contains("Expect", "100-continue"))
        response->writeContinue();

    if (request->url() == "/" || request->url() == "/index.html")
    {
        QFile file(":/webcontrol/index.html");
        file.open(QIODevice::ReadOnly);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", "text/html; charset=utf8");
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(file.readAll());
    }
    else if (request->url() == "/displayRenderer.js")
    {
        QFile file(":/webcontrol/displayRenderer.js");
        file.open(QIODevice::ReadOnly);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", "application/javascript; charset=utf8");
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(file.readAll());
    }
    else if (request->url() == "/jquery.mousewheel.js")
    {
        QFile file(":/webcontrol/jquery.mousewheel.js");
        file.open(QIODevice::ReadOnly);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", "application/javascript; charset=utf8");
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(file.readAll());
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

void GraphicsSceneWebServerTask::onUpgrade(const QByteArray &head)
{
    Tufao::HttpServerRequest *request = qobject_cast<Tufao::HttpServerRequest *>(sender());
    webSocketHandler_->handleUpgrade(request, head);
}



/* GraphicsSceneMultiThreadedWebServer */

GraphicsSceneMultiThreadedWebServer::GraphicsSceneMultiThreadedWebServer(QObject *parent, QGraphicsScene *scene) :
    QTcpServer(parent),
    scene_(scene),
    mapMutex_(QMutex::Recursive),
    taskController_(new TaskProcessingController(this))
{

}

GraphicsSceneMultiThreadedWebServer::~GraphicsSceneMultiThreadedWebServer()
{
    taskController_->waitForDone();
}

void GraphicsSceneMultiThreadedWebServer::listen(const QHostAddress &address, quint16 port, int threadsNumber)
{
    QTcpServer::listen(address, port);
}

void GraphicsSceneMultiThreadedWebServer::addDisplay(GraphicsSceneDisplay *c)
{
    QMutexLocker l(&mapMutex_);
    map_.insert(c->id(), c);
}

void GraphicsSceneMultiThreadedWebServer::removeDisplay(GraphicsSceneDisplay *c)
{
    QMutexLocker l(&mapMutex_);
    map_.remove(c->id());
}

GraphicsSceneDisplay *GraphicsSceneMultiThreadedWebServer::getDisplay(const QString &id)
{
    QMutexLocker l(&mapMutex_);
    if (map_.contains(id))
        return map_[id];

    return NULL;
}

void GraphicsSceneMultiThreadedWebServer::incomingConnection(int handle)
{
    DGUARDMETHODTIMED;
    GraphicsSceneWebServerTask *task = new GraphicsSceneWebServerTask(this, handle);
    taskController_->addTask(task);
}

