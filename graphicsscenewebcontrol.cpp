#include "graphicsscenewebcontrol.h"

#undef DEBUG
#include "debug.h"

#include "WebSocket"
#include "HttpServerRequest"

#include <QByteArray>
#include <QBuffer>
#include <QFile>
#include <QThread>

#include <QUuid>
#include <QCryptographicHash>
#include <QMutexLocker>

QString createUniqueID()
{
    QString uuid = QUuid::createUuid().toString();
    return QString(QCryptographicHash::hash(uuid.toUtf8(), QCryptographicHash::Sha1).toHex());
}

/* Patch */

class Patch
{
public:
    QString id;
    QRect rect;
    QBuffer data;
    QString mimeType;
    QByteArray dataBase64;
    QDateTime deadline;
};

/* WebServerConnection */

GraphicsSceneWebServerConnection::GraphicsSceneWebServerConnection(WebServerInterface *parent, Tufao::HttpServerRequest *request, const QByteArray &head) :
    QObject(),
    server_(parent),
    renderer_(NULL),
    frame_(0),
    clientReady_(true),
    urlMode_(true),
    updateCheckInterval_(1),
    patchesMutex_(QMutex::Recursive)
{
    socket_ = new Tufao::WebSocket(this);
    socket_->startServerHandshake(request, head);
    socket_->setMessagesType(Tufao::WebSocket::TEXT_MESSAGE);

    id_ = createUniqueID();

    connect(socket_, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    connect(socket_, SIGNAL(newMessage(QByteArray)), this, SLOT(clientMessageReceived(QByteArray)));

    clientConnected();
}

GraphicsSceneWebServerConnection::~GraphicsSceneWebServerConnection()
{
    qDeleteAll(patches_.values());

    server_->removeConnection(this);
}

void GraphicsSceneWebServerConnection::clientConnected()
{
    if (!renderer_)
    {
        renderer_ = new GraphicsSceneBufferRenderer();
    }

    commandInterpreter_.setTargetGraphicsScene(server_->scene());

    renderer_->setTargetGraphicsScene(server_->scene());
    renderer_->setEnabled(true);

    connect(renderer_, SIGNAL(damagedRegionAvailable()), this, SLOT(sceneDamagedRegionsAvailable()));
    connect(&timer_, SIGNAL(timeout()), this, SLOT(sendUpdate()));

    connect(&commandInterpreter_, SIGNAL(fullUpdateRequested()), renderer_, SLOT(fullUpdate()));

    QString info = "info(" + id_ + ")";
    socket_->sendMessage(info.toUtf8().constData());

    timer_.setSingleShot(false);
    timer_.start(updateCheckInterval_);
}

void GraphicsSceneWebServerConnection::clientMessageReceived(QByteArray data)
{
    QString rawCommand = data;

    int paramStartIndex = rawCommand.indexOf("(");
    int paramStopIndex = rawCommand.indexOf(")");

    QString command = rawCommand.mid(0, paramStartIndex);
    QString rawParams = rawCommand.mid(paramStartIndex + 1, paramStopIndex - paramStartIndex - 1);

    QStringList params = rawParams.split(",", QString::KeepEmptyParts);

    //DPRINTF("%p -> received message: %s, command: %s, rawParams: %s", socket_, data.constData(), command.toLatin1().constData(), rawParams.toLatin1().constData());

    if (command == "ready")
    {
        clientReady_ = true;
        if (renderer_->updatesAvailable())
        {
            DPRINTF("Updates are available, triggering again...");
            if (!timer_.isActive())
                timer_.start(updateCheckInterval_);
        }
    }
    else
        commandInterpreter_.sendCommand(command, params);
}

void GraphicsSceneWebServerConnection::sceneDamagedRegionsAvailable()
{
    if (!timer_.isActive())
        timer_.start(updateCheckInterval_);
}

Patch *GraphicsSceneWebServerConnection::createPatch(const QRect &rect, bool createBase64)
{
    Patch *patch = new Patch;

    patch->id = createUniqueID();
    patch->rect = rect;

    QImage image(rect.size(), QImage::Format_RGB888);

    QPainter p(&image);
    p.drawImage(0, 0, renderer_->buffer(), rect.x(), rect.y());

    patch->data.open(QIODevice::ReadWrite);
    image.save(&patch->data, "JPEG", 90);
    //image.save(&patch->data, "BMP");
    //image.save(&patch->data, "PNG");
    patch->data.close();

    patch->mimeType = "image/jpeg";

    if (createBase64)
    {
        patch->dataBase64 = patch->data.data().toBase64();

        DPRINTF("rect: %d,%d,%d,%d, %s, image.size: %d kB (%d byte), compressed size: %d kB (%d byte), base64 size: %d kB (%d byte)",
                rect.x(), rect.y(), rect.width(), rect.height(), patch->mimeType.toLatin1().constData(),
                image.byteCount() / 1024, image.byteCount(),
                patch->data.size() / 1024, patch->data.size(),
                patch->dataBase64.size() / 1024, patch->dataBase64.size()
                );
    }
    else
    {
        DPRINTF("rect: %d,%d,%d,%d, %s, image.size: %d kB (%d byte), compressed size: %d kB (%d byte)",
                rect.x(), rect.y(), rect.width(), rect.height(), patch->mimeType.toLatin1().constData(),
                image.byteCount() / 1024, image.byteCount(),
                patch->data.size() / 1024, patch->data.size()
                );
    }

    return patch;
}

void GraphicsSceneWebServerConnection::sendUpdate()
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&patchesMutex_);

    DPRINTF("connection: %p  thread: %p  clientReady_: %d  patches_.count(): %d", this, QThread::currentThread(), clientReady_, patches_.count());
    timer_.stop();

    if (clientReady_ && patches_.count() == 0)
    {
        l.unlock();
        QList<UpdateOperation> ops = renderer_->updateBufferExt();

        DPRINTF("Updates available: %d", ops.count());

        if (ops.count() > 0)
        {
            clientReady_ = false;
            ++frame_;
            DPRINTF("New Frame Number: %d", frame_);
        }

        for (int i = 0; i < ops.count(); ++i)
        {
            const UpdateOperation &op = ops.at(i);

            if (op.type == uotUpdate)
            {
                const QRect &rect = op.srcRect;

                if (urlMode_)
                {
                    Patch *patch = createPatch(rect, true);

                    QString framePatchId = QString::number(frame_) + "_" + QString::number(i);

                    QString cmd = QString().sprintf("drawImage(%d,%d,%d,%d,%d,%s):%s",
                        frame_,
                        rect.x(), rect.y(), rect.width(), rect.height(),
                        patch->mimeType.toLatin1().constData(),
                        QString("fb:" + id() + "/" + framePatchId).toLatin1().constData()
                    );

                    l.relock();
                    patches_.insert(framePatchId, patch);
                    l.unlock();

                    socket_->sendMessage(cmd.toLatin1());
                }
                else
                {
                    Patch *patch = createPatch(rect, true);

                    QString format = patch->mimeType + ";base64";
                    QString cmd = QString().sprintf("drawImage(%d,%d,%d,%d,%d,%s):",
                        frame_,
                        rect.x(), rect.y(), rect.width(), rect.height(),
                        format.toLatin1().constData()
                    );

                    socket_->sendMessage(cmd.toLatin1() + patch->dataBase64);
                    delete patch;
                }
            }
            else if (op.type == uotMove)
            {
                const QRect &rect = op.srcRect;

                QString cmd = QString().sprintf("moveImage(%d,%d,%d,%d,%d,%d,%d):",
                    frame_,
                    rect.x(), rect.y(), rect.width(), rect.height(),
                    op.dstPoint.x(), op.dstPoint.y()
                );

                socket_->sendMessage(cmd.toLatin1());
            }
            else if (op.type == uotFill)
            {
                const QRect &rect = op.srcRect;

                QString cmd = QString().sprintf("fillRect(%d,%d,%d,%d,%d,%s):",
                    frame_,
                    rect.x(), rect.y(), rect.width(), rect.height(),
                    op.fillColor.name().toLatin1().constData()
                );

                socket_->sendMessage(cmd.toLatin1());
            }
        }

        if (ops.count() > 0)
            socket_->sendMessage(QString().sprintf("end(%d):", frame_).toLatin1().constData());
    }
}

void GraphicsSceneWebServerConnection::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    // THIS METHOD IS MULTI-THREADED!

    QString url = request->url();

    int indexOfSlash = url.lastIndexOf("/");
    QString patchId = "";

    if (indexOfSlash > -1)
        patchId = url.mid(indexOfSlash + 1);

    QMutexLocker l(&patchesMutex_);
    if (!patchId.isEmpty() && patches_.contains(patchId))
    {
        Patch *patch = patches_.take(patchId);
        l.unlock();

        patch->data.open(QIODevice::ReadOnly);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", patch->mimeType.toUtf8().constData());
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(patch->data.readAll());

        delete patch;
    }
    else
    {
        response->writeHead(404);
        response->end("Not found");
    }
}

void GraphicsSceneWebServerConnection::clientDisconnected()
{
    this->deleteLater();
}

/* GraphicsSceneSingleThreadedWebServer */

GraphicsSceneSingleThreadedWebServer::GraphicsSceneSingleThreadedWebServer(QObject *parent, QGraphicsScene *scene) :
    Tufao::HttpServer(parent),
    scene_(scene)
{
    connect(this, SIGNAL(requestReady(Tufao::HttpServerRequest*, Tufao::HttpServerResponse*)),
            this, SLOT(handleRequest(Tufao::HttpServerRequest*, Tufao::HttpServerResponse*)));
}

void GraphicsSceneSingleThreadedWebServer::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    if (request->url() == "/" || request->url() == "/index.html")
    {
        QFile indexFile(":/webcontrol/index.html");
        indexFile.open(QIODevice::ReadOnly);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", "text/html; charset=utf8");
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(indexFile.readAll());
    }
    else if (request->url() == "/displayRenderer.js")
    {
        QFile indexFile(":/webcontrol/displayRenderer.js");
        indexFile.open(QIODevice::ReadOnly);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", "application/javascript; charset=utf8");
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(indexFile.readAll());
    }
    else
    {
        QString id = QString(request->url()).mid(1, 40);
        GraphicsSceneWebServerConnection *c = getConnection(id);

        if (c)
        {
            c->handleRequest(request, response);
            return;
        }

        response->writeHead(404);
        response->end("Not found");
    }
}

void GraphicsSceneSingleThreadedWebServer::upgrade(Tufao::HttpServerRequest *request, const QByteArray &head)
{
    if (request->url() != "/display")
    {
        Tufao::HttpServerResponse response(request->socket(), request->responseOptions());
        response.writeHead(404);
        response.end("Not found");
        request->socket()->close();
        return;
    }

    GraphicsSceneWebServerConnection *c = new GraphicsSceneWebServerConnection(this, request, head);
    addConnection(c);
}

void GraphicsSceneSingleThreadedWebServer::addConnection(GraphicsSceneWebServerConnection *c)
{
    map_.insert(c->id(), c);
}

void GraphicsSceneSingleThreadedWebServer::removeConnection(GraphicsSceneWebServerConnection *c)
{
    map_.remove(c->id());
}

GraphicsSceneWebServerConnection *GraphicsSceneSingleThreadedWebServer::getConnection(const QString &id)
{
    if (map_.contains(id))
        return map_[id];

    return NULL;
}

/* GraphicsSceneWebServerThread */

GraphicsSceneWebServerThread::GraphicsSceneWebServerThread(GraphicsSceneMultiThreadedWebServer *parent) :
    QThread(),
    server_(parent)
{
    // need this to make the signals and slots run in current thread
    moveToThread(this);
    connect(parent, SIGNAL(destroyed()), this, SLOT(quit()));
    connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    connect(this, SIGNAL(newConnection(int)), this, SLOT(onNewConnection(int)));
}

void GraphicsSceneWebServerThread::addConnection(int socketDescriptor)
{
    emit newConnection(socketDescriptor);
}

void GraphicsSceneWebServerThread::onNewConnection(int socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);

    if (!socket->setSocketDescriptor(socketDescriptor))
    {
        delete socket;
        return;
    }

    Tufao::HttpServerRequest *handle = new Tufao::HttpServerRequest(socket, this);

    connect(handle, SIGNAL(ready()), this, SLOT(onRequestReady()));
    connect(handle, SIGNAL(upgrade(QByteArray)), this, SLOT(onUpgrade(QByteArray)));
    connect(socket, SIGNAL(disconnected()), handle, SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
}

void GraphicsSceneWebServerThread::onRequestReady()
{
    Tufao::HttpServerRequest *request = qobject_cast<Tufao::HttpServerRequest *>(sender());

    QAbstractSocket *socket = request->socket();
    Tufao::HttpServerResponse *response = new Tufao::HttpServerResponse(socket, request->responseOptions(), this);

    connect(socket, SIGNAL(disconnected()), response, SLOT(deleteLater()));
    connect(response, SIGNAL(finished()), response, SLOT(deleteLater()));

    if (request->headers().contains("Expect", "100-continue"))
        response->writeContinue();

    if (request->url() == "/" || request->url() == "/index.html")
    {
        QFile indexFile(":/webcontrol/index.html");
        indexFile.open(QIODevice::ReadOnly);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", "text/html; charset=utf8");
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(indexFile.readAll());
    }
    else if (request->url() == "/displayRenderer.js")
    {
        QFile indexFile(":/webcontrol/displayRenderer.js");
        indexFile.open(QIODevice::ReadOnly);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", "application/javascript; charset=utf8");
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(indexFile.readAll());
    }
    else
    {
        QString id = QString(request->url()).mid(1, 40);
        GraphicsSceneWebServerConnection *c = server_->getConnection(id);

        if (c)
        {
            c->handleRequest(request, response);
            return;
        }

        response->writeHead(404);
        response->end("Not found");
    }

}

void GraphicsSceneWebServerThread::onUpgrade(const QByteArray &head)
{
    Tufao::HttpServerRequest *request = qobject_cast<Tufao::HttpServerRequest *>(sender());

    if (request->url() != "/display")
    {
        Tufao::HttpServerResponse response(request->socket(), request->responseOptions());
        response.writeHead(404);
        response.end("Not found");
        request->socket()->close();
        return;
    }

    GraphicsSceneWebServerConnection *c = new GraphicsSceneWebServerConnection(server_, request, head);
    c->moveToThread(this);
    server_->addConnection(c);
}


/* GraphicsSceneMultiThreadedWebServer */

GraphicsSceneMultiThreadedWebServer::GraphicsSceneMultiThreadedWebServer(QObject *parent, QGraphicsScene *scene) :
    QTcpServer(parent),
    i(0),
    scene_(scene),
    mapMutex_(QMutex::Recursive)
{
}

GraphicsSceneMultiThreadedWebServer::~GraphicsSceneMultiThreadedWebServer()
{
    // the thread can't have a parent then...
    foreach (GraphicsSceneWebServerThread* t, threads)
    {
        t->quit();
    }

    foreach (GraphicsSceneWebServerThread* t, threads)
    {
        t->wait();
        delete t;
    }
}

void GraphicsSceneMultiThreadedWebServer::listen(const QHostAddress &address, quint16 port, int threadsNumber)
{
    threads.reserve(threadsNumber);

    for (int i = 0; i < threadsNumber; ++i)
    {
        threads.push_back(new GraphicsSceneWebServerThread(this));
        threads[i]->start();
    }

    QTcpServer::listen(address, port);
}

void GraphicsSceneMultiThreadedWebServer::addConnection(GraphicsSceneWebServerConnection *c)
{
    QMutexLocker l(&mapMutex_);
    map_.insert(c->id(), c);
}

void GraphicsSceneMultiThreadedWebServer::removeConnection(GraphicsSceneWebServerConnection *c)
{
    QMutexLocker l(&mapMutex_);
    map_.remove(c->id());
}

GraphicsSceneWebServerConnection *GraphicsSceneMultiThreadedWebServer::getConnection(const QString &id)
{
    QMutexLocker l(&mapMutex_);
    if (map_.contains(id))
        return map_[id];

    return NULL;
}

void GraphicsSceneMultiThreadedWebServer::incomingConnection(int handle)
{
    threads[(i++) % threads.size()]->addConnection(handle);
}
