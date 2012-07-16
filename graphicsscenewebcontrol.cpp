#include "graphicsscenewebcontrol.h"

#undef DEBUG
#include "debug.h"

#include "WebSocket"
#include "HttpServerRequest"

#include <QByteArray>
#include <QBuffer>
#include <QFile>

#include <QUuid>
#include <QCryptographicHash>

QString getUniqueID()
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

GraphicsSceneWebServerConnection::GraphicsSceneWebServerConnection(GraphicsSceneWebServer *parent, Tufao::HttpServerRequest *request, const QByteArray &head) :
    QObject(parent),
    server_(parent),
    renderer_(NULL),
    frame_(0),
    clientReady_(true),
    urlMode_(true)
{
    socket_ = new Tufao::WebSocket(this);
    socket_->startServerHandshake(request, head);
    socket_->setMessagesType(Tufao::WebSocket::TEXT_MESSAGE);

    id_ = getUniqueID();

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
        renderer_ = new GraphicsSceneBufferRenderer(this);

    commandInterpreter_.setTargetGraphicsScene(server_->scene());

    renderer_->setTargetGraphicsScene(server_->scene());
    renderer_->setEnabled(true);

    connect(renderer_, SIGNAL(damagedRegionAvailable()), this, SLOT(sceneDamagedRegionsAvailable()));
    connect(&timer_, SIGNAL(timeout()), this, SLOT(sendUpdate()));

    connect(&commandInterpreter_, SIGNAL(fullUpdateRequested()), renderer_, SLOT(fullUpdate()));

    QString info = "info(" + id_ + ")";
    socket_->sendMessage(info.toUtf8().constData());

    timer_.setSingleShot(true);
    timer_.start(10);
}

void GraphicsSceneWebServerConnection::clientMessageReceived(QByteArray data)
{
    QString rawCommand = data;

    int paramStartIndex = rawCommand.indexOf("(");
    int paramStopIndex = rawCommand.indexOf(")");

    QString command = rawCommand.mid(0, paramStartIndex);
    QString rawParams = rawCommand.mid(paramStartIndex + 1, paramStopIndex - paramStartIndex - 1);

    QStringList params = rawParams.split(",", QString::KeepEmptyParts);

    DPRINTF("%p -> received message: %s, command: %s, rawParams: %s", socket_, data.constData(), command.toLatin1().constData(), rawParams.toLatin1().constData());

    commandInterpreter_.sendCommand(command, params);
}

void GraphicsSceneWebServerConnection::sceneDamagedRegionsAvailable()
{
    if (clientReady_ && patches_.count() == 0)
    {
        timer_.start(5);
    }
}

Patch *GraphicsSceneWebServerConnection::createPatch(const QRect &rect, bool createBase64)
{
    Patch *patch = new Patch;

    patch->id = getUniqueID();
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

    ++frame_;

    clientReady_ = false;

    QRegion damagedRegion = renderer_->updateBuffer();
    QVector<QRect> rects = damagedRegion.rects();

    for (int i = 0; i < rects.count(); ++i)
    {
        const QRect &rect = rects.at(i);

        if (urlMode_)
        {
            Patch *patch = createPatch(rect, true);

            QString framePatchId = QString::number(frame_) + "_" + QString::number(i);

            QString cmd = QString().sprintf(
                "drawImage(%d,%d,%d,%d,%d,%s):%s",
                frame_, rect.x(), rect.y(), rect.width(), rect.height(), patch->mimeType.toLatin1().constData(),
                QString("fb:" + id() + "/" + framePatchId).toLatin1().constData()
            );

            patches_.insert(framePatchId, patch);
            socket_->sendMessage(cmd.toLatin1());
        }
        else
        {
            Patch *patch = createPatch(rect, true);

            QString format = patch->mimeType + ";base64";
            QString cmd = QString().sprintf("drawImage(%d,%d,%d,%d,%d,%s):", frame_, rect.x(), rect.y(), rect.width(), rect.height(), format.toLatin1().constData());

            socket_->sendMessage(cmd.toLatin1() + patch->dataBase64);
            delete patch;
        }
    }

    clientReady_ = true;
    if (renderer_->updatesAvailable())
        timer_.start(10);
}

void GraphicsSceneWebServerConnection::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    QString url = request->url();

    int indexOfSlash = url.lastIndexOf("/");
    QString patchId = "";

    if (indexOfSlash > -1)
        patchId = url.mid(indexOfSlash + 1);

    if (!patchId.isEmpty() && patches_.contains(patchId))
    {
        Patch *patch = patches_.take(patchId);

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

/* WebServer */

GraphicsSceneWebServer::GraphicsSceneWebServer(QObject *parent, QGraphicsScene *scene) :
    Tufao::HttpServer(parent),
    scene_(scene)
{
    connect(this, SIGNAL(requestReady(Tufao::HttpServerRequest*, Tufao::HttpServerResponse*)),
            this, SLOT(handleRequest(Tufao::HttpServerRequest*, Tufao::HttpServerResponse*)));
}

void GraphicsSceneWebServer::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
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
        if (map_.contains(id))
        {
            GraphicsSceneWebServerConnection *c = map_[id];
            c->handleRequest(request, response);
            return;
        }

        response->writeHead(404);
        response->end("Not found");
    }
}

void GraphicsSceneWebServer::upgrade(Tufao::HttpServerRequest *request, const QByteArray &head)
{
    if (request->url() != "/display") {
        Tufao::HttpServerResponse response(request->socket(), request->responseOptions());
        response.writeHead(404);
        response.end("Not found");
        request->socket()->close();
        return;
    }

    GraphicsSceneWebServerConnection *c = new GraphicsSceneWebServerConnection(this, request, head);
    map_.insert(c->id(), c);
}

void GraphicsSceneWebServer::removeConnection(GraphicsSceneWebServerConnection *c)
{
    map_.remove(c->id());
}
