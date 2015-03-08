#include "websockethandler.h"

#include <QStringList>

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

#include "viriditywebserver.h"

WebSocketHandler::WebSocketHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent),
    session_(NULL)
{
    DGUARDMETHODTIMED;
    socket_ = new Tufao::WebSocket(this);

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

    if (request->url().endsWith("/v/ws") || request->url().endsWith("/v/wsb"))
    {
        socket_->setMessagesType(request->url().endsWith("/v/ws") ? Tufao::WebSocket::TEXT_MESSAGE : Tufao::WebSocket::BINARY_MESSAGE);

        QString id = ViriditySession::parseIdFromUrl(request->url());

        ViriditySession *session = server()->sessionManager()->getSession(id);

        if (session)
        {
            QString msg;

            if (session->useCount() == 0)
            {
                session_ = server()->sessionManager()->acquireSession(id);

                connect(session_, SIGNAL(newPendingMessagesAvailable()), this, SLOT(handleMessagesAvailable()));

                socket_->startServerHandshake(request, head);

                msg = "reattached(" + session_->id() + ")";
            }
            else
            {
                socket_->startServerHandshake(request, head);

                msg = "inuse(" + session->id() + ")";
            }

            socket_->sendMessage(msg.toUtf8().constData());
        }
        else
        {
            session_ = server()->sessionManager()->getNewSession();

            connect(session_, SIGNAL(newPendingMessagesAvailable()), this, SLOT(handleMessagesAvailable()));

            socket_->startServerHandshake(request, head);

            QString info = "info(" + session_->id() + ")";
            socket_->sendMessage(info.toUtf8().constData());
        }

        return;
    }

    Tufao::HttpServerResponse response(request->socket(), request->responseOptions());
    response.writeHead(404);
    response.end("Not found");
    request->socket()->close();
}

bool WebSocketHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    return request->url().endsWith("/v/ws") || request->url().endsWith("/v/wsb");
}

void WebSocketHandler::handleMessagesAvailable()
{
    if (session_ && session_->pendingMessagesAvailable())
    {
        QList<QByteArray> messages = session_->takePendingMessages(socket_->messagesType() == Tufao::WebSocket::BINARY_MESSAGE);

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
