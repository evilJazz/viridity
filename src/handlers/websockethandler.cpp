/****************************************************************************
**
** Copyright (C) 2012-2016 Andre Beckedorf, Meteora Softworks
** Contact: info@meteorasoftworks.com
**
** This file is part of Viridity
**
** $VIRIDITY_BEGIN_LICENSE:COMMERCIAL_AGPL$
**
** This library is licensed under either a separately available commercial
** license or the GNU Affero General Public License Version 3.0,
** published 19 November 2007.
**
** See https://www.gnu.org/licenses/agpl-3.0.html or LICENSE-agpl-3.0.txt for
** details.
**
** If you wish to use and distribute the Viridity library in your commercial
** product without making your sourcecode available to the public, please
** contact us for a commercial license at info@meteorasoftworks.com
**
** $VIRIDITY_END_LICENSE$
**
****************************************************************************/

#include "websockethandler.h"

#include <QStringList>

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

#include "Tufao/WebSocket"

#include "viriditywebserver.h"

WebSocketHandler::WebSocketHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent),
    sessionMutex_(QMutex::Recursive),
    session_(NULL),
    socket_(NULL)
{
    DGUARDMETHODTIMED;
    websocket_ = new Tufao::WebSocket(this);

    connect(websocket_, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    connect(websocket_, SIGNAL(newMessage(QByteArray)), this, SLOT(clientMessageReceived(QByteArray)));
}

WebSocketHandler::~WebSocketHandler()
{
    QMutexLocker m(&sessionMutex_);

    if (session_)
    {
        disconnect(session_, SIGNAL(interactionDormant()), this, SLOT(handleSessionInteractionDormant()));
        server()->sessionManager()->releaseSession(session_);
        session_ = NULL;
    }

    DGUARDMETHODTIMED;
}

void WebSocketHandler::handleUpgrade(ViridityHttpServerRequest *request, const QByteArray &head)
{
    DGUARDMETHODTIMED;

    if (request->url().endsWith("/v/ws") || request->url().endsWith("/v/wsb"))
    {
        websocket_->setMessagesType(request->url().endsWith("/v/ws") ? Tufao::WebSocket::TEXT_MESSAGE : Tufao::WebSocket::BINARY_MESSAGE);

        QString id = ViriditySession::parseIdFromUrl(request->url());

        ViriditySession *session = server()->sessionManager()->getSession(id);

        if (session)
        {
            QString msg;

            if (session->useCount() == 0)
            {
                QMutexLocker m(&sessionMutex_);
                session_ = server()->sessionManager()->acquireSession(id);

                connect(session_, SIGNAL(newPendingMessagesAvailable()), this, SLOT(handleMessagesAvailable()));

                websocket_->startServerHandshake(request, head);

                msg = "reattached(" + session_->id() + ")";
                websocket_->sendMessage(msg.toUtf8().constData());
            }
            else
            {
                websocket_->startServerHandshake(request, head);

                msg = "inuse(" + session->id() + ")";
                websocket_->sendMessage(msg.toUtf8().constData());
            }
        }
        else
        {
            QMutexLocker m(&sessionMutex_);
            session_ = server()->sessionManager()->getNewSession(request->getPeerAddressFromRequest());

            websocket_->startServerHandshake(request, head);

            QString info = "info(" + session_->id() + ")";
            websocket_->sendMessage(info.toUtf8().constData());
        }

        QMutexLocker m(&sessionMutex_);
        if (session_)
        {
            connect(session_, SIGNAL(destroyed()), this, SLOT(handleSessionDestroyed()), (Qt::ConnectionType)(Qt::DirectConnection | Qt::UniqueConnection));
            connect(session_, SIGNAL(newPendingMessagesAvailable()), this, SLOT(handleMessagesAvailable()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));
            connect(session_, SIGNAL(interactionDormant()), this, SLOT(handleSessionInteractionDormant()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));

            if (session_->pendingMessagesAvailable())
                handleMessagesAvailable();
        }

        socket_ = request->socket();
        return;
    }

    ViridityHttpServerResponse response(request->socket(), request->responseOptions());
    response.writeHead(404);
    response.end("Not found");
    request->socket()->close();
}

bool WebSocketHandler::doesHandleRequest(ViridityHttpServerRequest *request)
{
    return request->url().endsWith("/v/ws") || request->url().endsWith("/v/wsb");
}

void WebSocketHandler::handleMessagesAvailable()
{
    QMutexLocker m(&sessionMutex_);

    if (session_ && session_->pendingMessagesAvailable())
    {
        QList<QByteArray> messages = session_->takePendingMessages(websocket_->messagesType() == Tufao::WebSocket::BINARY_MESSAGE);

        foreach (const QByteArray &message, messages)
            websocket_->sendMessage(message);
    }
}

void WebSocketHandler::handleSessionInteractionDormant()
{
    DGUARDMETHODTIMED;
    QMutexLocker m(&sessionMutex_);

    if (socket_ && websocket_)
    {
        if (session_ && session_->lastUsed() > 1.5 * session_->interactionCheckInterval())
        {
            close();
            return;
        }

        websocket_->sendMessage("ping()");
    }
}

void WebSocketHandler::close()
{
    if (socket_ && websocket_)
    {
        websocket_->close();
        socket_->close();
    }
}

void WebSocketHandler::handleSessionDestroyed()
{
    QMutexLocker m(&sessionMutex_);
    session_ = NULL;
    QMetaObject::invokeMethod(this, "close");
}

void WebSocketHandler::clientMessageReceived(QByteArray data)
{
    QMutexLocker m(&sessionMutex_);

    if (session_)
        session_->dispatchMessageToHandlers(data);
}

void WebSocketHandler::clientDisconnected()
{
    socket_ = NULL;
}
