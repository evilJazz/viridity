#include "websockethandler.h"

#include <QStringList>

//#undef DEBUG
#include "KCL/debug.h"

#include "viriditywebserver.h"

WebSocketHandler::WebSocketHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent),
    session_(NULL)
{
    DGUARDMETHODTIMED;
    socket_ = new Tufao::WebSocket(this);
    socket_->setMessagesType(Tufao::WebSocket::TEXT_MESSAGE);

    connect(socket_, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    connect(socket_, SIGNAL(newMessage(QByteArray)), this, SLOT(clientMessageReceived(QByteArray)));
}

WebSocketHandler::~WebSocketHandler()
{
    if (session_)
        server()->sessionManager()->releaseSession(session_);

    DGUARDMETHODTIMED;
}

void WebSocketHandler::handleUpgrade(Tufao::HttpServerRequest *request, const QByteArray &head)
{
    DGUARDMETHODTIMED;

    if (request->url() != "/viridity")
    {
        Tufao::HttpServerResponse response(request->socket(), request->responseOptions());
        response.writeHead(404);
        response.end("Not found");
        request->socket()->close();
        return;
    }

    session_ = server()->sessionManager()->getNewSession();

    connect(session_, SIGNAL(newPendingMessagesAvailable()), this, SLOT(handleMessagesAvailable()));

    socket_->startServerHandshake(request, head);

    QString info = "info(" + session_->id() + ")";
    socket_->sendMessage(info.toUtf8().constData());
}

bool WebSocketHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    QString id = ViriditySession::parseIdFromUrl(request->url());
    return server()->sessionManager()->getSession(id) != NULL;
}

void WebSocketHandler::handleMessagesAvailable()
{
    if (session_ && session_->pendingMessagesAvailable())
    {
        QList<QByteArray> messages = session_->takePendingMessages();

        foreach (const QByteArray &message, messages)
            socket_->sendMessage(message);
    }
}

void WebSocketHandler::clientMessageReceived(QByteArray data)
{
    if (session_)
        session_->dispatchMessageToHandlers(data);
}

void WebSocketHandler::clientDisconnected()
{
}
