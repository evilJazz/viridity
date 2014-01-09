#include "graphicsscenewebcontrol.h"

#undef DEBUG
#include "private/debug.h"

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


QString createUniqueID()
{
    QString uuid = QUuid::createUuid().toString();
    return QString(QCryptographicHash::hash(uuid.toUtf8(), QCryptographicHash::Sha1).toHex());
}

GraphicsSceneInputPostHandler::GraphicsSceneInputPostHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, GraphicsSceneWebServerConnection *connection, QObject *parent) :
    QObject(parent),
    request_(request),
    response_(response),
    connection_(connection)
{
    connect(request_, SIGNAL(data(QByteArray)), this, SLOT(onData(QByteArray)));
    connect(request_, SIGNAL(end()), this, SLOT(onEnd()));
}

void GraphicsSceneInputPostHandler::onData(const QByteArray &chunk)
{
    data_ += chunk;
}

void GraphicsSceneInputPostHandler::onEnd()
{
    QList<QByteArray> commands = data_.split('\n');

    foreach (QByteArray command, commands)
        connection_->clientMessageReceived(command);

    // handle request
    response_->writeHead(Tufao::HttpServerResponse::OK);
    response_->end();
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
    commandsMutex_(QMutex::NonRecursive),
    urlMode_(true),
    updateCheckInterval_(1),
    frame_(0),
    renderer_(NULL),
    clientReady_(true),
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

GraphicsSceneWebServerConnection::GraphicsSceneWebServerConnection(WebServerInterface *parent, Tufao::HttpServerResponse *response) :
    QObject(),
    server_(parent),
    commandsMutex_(QMutex::NonRecursive),
    urlMode_(true),
    updateCheckInterval_(1),
    frame_(0),
    renderer_(NULL),
    clientReady_(true),
    patchesMutex_(QMutex::Recursive)
{
    socket_ = NULL;
    id_ = createUniqueID();

    clientConnected(response);
}

GraphicsSceneWebServerConnection::~GraphicsSceneWebServerConnection()
{
    qDeleteAll(patches_.values());

    server_->removeConnection(this);
}

void GraphicsSceneWebServerConnection::clientConnected(Tufao::HttpServerResponse *response)
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
    if (socket_)
        socket_->sendMessage(info.toUtf8().constData());
    else
    {
        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", "text/plain; charset=utf8");
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(info.toUtf8());
    }

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

    if (socket_)
    {
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
    else
    {
        if (command != "ready")
            commandInterpreter_.sendCommand(command, params);

        clientReady_ = true;
    }
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
    //image.save(&patch->data, "JPEG", 90);
    //image.save(&patch->data, "BMP");
    image.save(&patch->data, "PNG");
    patch->data.close();

    patch->mimeType = "image/png";

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
    if (socket_)
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

                    sendCommand(cmd);
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

                    sendCommand(cmd + patch->dataBase64);

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

                sendCommand(cmd);
            }
            else if (op.type == uotFill)
            {
                const QRect &rect = op.srcRect;

                QString cmd = QString().sprintf("fillRect(%d,%d,%d,%d,%d,%s):",
                    frame_,
                    rect.x(), rect.y(), rect.width(), rect.height(),
                    op.fillColor.name().toLatin1().constData()
                );

                sendCommand(cmd);
            }
        }

        if (ops.count() > 0)
            sendCommand(QString().sprintf("end(%d):", frame_));


        {
            QMutexLocker cl(&commandsMutex_);
            commandsPresent_.wakeAll();
        }
    }
}

void GraphicsSceneWebServerConnection::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    //qDebug("GraphicsSceneWebServerConnection::handleRequest  ->  Thread: %p (%d)", QThread::currentThread(), QThread::currentThreadId());

    // THIS METHOD IS MULTI-THREADED!

    QString url = request->url();

    if (url.startsWith("/display?")) // long polling
    {
        if (request->method() == "GET") // Long polling output
        {
            clientMessageReceived("ready()");

            QByteArray out;

            {
                QMutexLocker cl(&commandsMutex_);

                if (commands_.isEmpty())
                {
                    bool gotData =
                            commandsPresent_.wait(&commandsMutex_, 1800);

                    qDebug(gotData ? "got frame" : "timed out");

                    if (!gotData)
                    {
                        out = commands_.join("\n").toUtf8();
                        commands_.clear();
                    }
                }
                else
                {
                    qDebug("got frame at once");
                    out = commands_.join("\n").toUtf8();
                    commands_.clear();
                }
            }

            response->writeHead(Tufao::HttpServerResponse::OK);
            response->headers().insert("Content-Type", "text/plain; charset=utf8");
            response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
            response->headers().insert("Pragma", "no-cache");
            response->end(out);
        }
        else if (request->method() == "POST") // Long polling input
        {
            GraphicsSceneInputPostHandler *handler = new GraphicsSceneInputPostHandler(request, response, this);
            connect(response, SIGNAL(finished()), handler, SLOT(deleteLater()));
            return;
        }
    }
    else
    {
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
}

void GraphicsSceneWebServerConnection::sendCommand(const QString &cmd)
{
    if (socket_)
        socket_->sendMessage(cmd.toLatin1());
    else
    {
        QMutexLocker cl(&commandsMutex_);
        commands_.append(cmd);
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
    else if (request->url().startsWith("/display?")) // long polling
    {
        QUrl url(request->url());
        QString id = url.queryItemValue("id");
//qDebug("ID is %s", id.toUtf8().constData());

        GraphicsSceneWebServerConnection *c = server_->getConnection(id);

        if (c)
        {
            c->handleRequest(request, response);
            return;
        }
        else if (id.isEmpty()) // start new connection
        {
            c = new GraphicsSceneWebServerConnection(server_, response);
            c->moveToThread(this);
            server_->addConnection(c);
            return;
        }

        response->writeHead(404);
        response->end("Not found");
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


static const char* commandScriptPath = "/command?";


CommandPostHandler::CommandPostHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, QObject *parent) :
    QObject(parent),
    request_(request),
    response_(response)
{
    connect(request_, SIGNAL(data(QByteArray)), this, SLOT(onData(QByteArray)));
    connect(request_, SIGNAL(end()), this, SLOT(onEnd()));
}

void CommandPostHandler::onData(const QByteArray &chunk)
{
    data_ += chunk;
}

void CommandPostHandler::onEnd()
{
    // handle request
    QString id = QString(request_->url()).mid(strlen(commandScriptPath), 40);
    QString command(data_);
qDebug("Command is %s", data_.constData());
    QString result = globalCommandBridge.handleCommandReady(id, command);

    response_->writeHead(Tufao::HttpServerResponse::OK);
    response_->write(result.toUtf8());
    response_->flush();
    response_->end();
//    response_->end(result.toUtf8());
}


CommandWebServer::CommandWebServer(QObject *parent) :
    Tufao::HttpServer(parent)
{
    connect(this, SIGNAL(requestReady(Tufao::HttpServerRequest*,Tufao::HttpServerResponse*)),
            this, SLOT(handleRequest(Tufao::HttpServerRequest*,Tufao::HttpServerResponse*)));
}

void CommandWebServer::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    if (request->method() == "POST" && request->url().startsWith(commandScriptPath))
    {
        CommandPostHandler *handler = new CommandPostHandler(request, response);
        connect(response, SIGNAL(finished()), handler, SLOT(deleteLater()));
    }
}
