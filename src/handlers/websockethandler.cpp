#include "websockethandler.h"

#include <QStringList>

//#undef DEBUG
#include "KCL/debug.h"

#include "viriditywebserver.h"
#include "graphicsscenedisplay.h"

WebSocketHandler::WebSocketHandler(ViridityConnection *parent) :
    QObject(parent),
    connection_(parent),
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
    if (display_)
        connection_->server()->sessionManager()->releaseDisplay(display_);

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

    display_ = connection_->server()->sessionManager()->getNewDisplay();

    connect(display_, SIGNAL(updateAvailable()), this, SLOT(handleDisplayUpdateAvailable()));

    socket_->startServerHandshake(request, head);

    QString info = "info(" + display_->id() + ")";
    socket_->sendMessage(info.toUtf8().constData());
}

bool WebSocketHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    QString id = QString(request->url()).mid(1, 40);
    return connection_->server()->sessionManager()->getDisplay(id) != NULL;
}

void WebSocketHandler::handleDisplayUpdateAvailable()
{
    if (display_ && display_->isUpdateAvailable())
    {
        QStringList commandList = display_->getMessagesForPendingUpdates();

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
