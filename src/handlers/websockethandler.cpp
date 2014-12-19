#include "websockethandler.h"

#include <QStringList>

#include "KCL/debug.h"

#include "graphicsscenewebcontrol.h"
#include "graphicsscenedisplay.h"

WebSocketHandler::WebSocketHandler(GraphicsSceneWebServerTask *parent) :
    QObject(parent),
    task_(parent),
    display_(NULL)
{
    DGUARDMETHODTIMED;
    socket_ = new Tufao::WebSocket(this);
    socket_->setMessagesType(Tufao::WebSocket::TEXT_MESSAGE);

    connect(socket_, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    connect(socket_, SIGNAL(newMessage(QByteArray)), this, SLOT(clientMessageReceived(QByteArray)));
}

WebSocketHandler::~WebSocketHandler()
{
    DGUARDMETHODTIMED;
}

void WebSocketHandler::handleUpgrade(Tufao::HttpServerRequest *request, const QByteArray &head)
{
    DGUARDMETHODTIMED;

    if (request->url() != "/display")
    {
        Tufao::HttpServerResponse response(request->socket(), request->responseOptions());
        response.writeHead(404);
        response.end("Not found");
        request->socket()->close();
        return;
    }

    display_ = new GraphicsSceneDisplay(task_->server());
    display_->moveToThread(QThread::currentThread());
    task_->server()->addDisplay(display_);

    connect(display_, SIGNAL(updateAvailable()), this, SLOT(handleDisplayUpdateAvailable()));

    socket_->startServerHandshake(request, head);

    QString info = "info(" + display_->id() + ")";
    socket_->sendMessage(info.toUtf8().constData());
}

bool WebSocketHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    QString id = QString(request->url()).mid(1, 40);
    return task_->server()->getDisplay(id) != NULL;
}

void WebSocketHandler::handleDisplayUpdateAvailable()
{
    if (display_ && display_->isUpdateAvailable())
    {
        QStringList commandList = display_->getCommandsForPendingUpdates();

        foreach (QString command, commandList)
            socket_->sendMessage(command.toLatin1());
    }
}

void WebSocketHandler::clientMessageReceived(QByteArray data)
{
    if (display_)
        display_->handleReceivedMessage(data);
}

void WebSocketHandler::clientDisconnected()
{

}
