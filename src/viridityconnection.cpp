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

#include "viridityconnection.h"

#include <QThread>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>

#include "viriditywebserver.h"

#include "handlers/inputposthandler.h"
#include "handlers/websockethandler.h"
#include "handlers/ssehandler.h"
#include "handlers/longpollinghandler.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

/* ViridityTcpSocket */

ViridityTcpSocket::ViridityTcpSocket(QSharedPointer<ViridityConnection> connection) :
    QTcpSocket(),
    connection_(connection)
{
    DGUARDMETHODTIMED;
}

ViridityTcpSocket::~ViridityTcpSocket()
{
    DGUARDMETHODTIMED;
}

void ViridityTcpSocket::sharedPointerDeleteLater(ViridityTcpSocket *socket)
{
    DGUARDMETHODTIMED;
    socket->deleteLater();
}


/* ViridityConnection */

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
ViridityConnection::ViridityConnection(ViridityWebServer *server, qintptr socketDescriptor) :
#else
ViridityConnection::ViridityConnection(ViridityWebServer *server, int socketDescriptor) :
#endif
    QObject(),
    sharedMemberMREW_(QReadWriteLock::Recursive),
    webSocketHandler_(NULL),
    sseHandler_(NULL),
    longPollingHandler_(NULL),
    server_(server),
    socket_(),
    request_(),
    response_(),
    socketDescriptor_(socketDescriptor),
    reUseCount_(0)
{
    DGUARDMETHODTIMED;
    created_.start();
    lastUsed_.start();
}

ViridityConnection::~ViridityConnection()
{
    DGUARDMETHODTIMED;
    close();
    handleSocketDisconnected();
}

QVariant ViridityConnection::stats()
{
    QReadLocker l(&sharedMemberMREW_);

    QVariantMap result;

    result.insert("request.url", requestUrl_);
    result.insert("request.method", requestMethod_);

    result.insert("socket.peerAddress", !socket_.isNull() && socket_->state() == QAbstractSocket::ConnectedState ? socket_->peerAddress().toString() : QVariant());

    result.insert("socketDescriptor", socketDescriptor_);
    result.insert("age", created_.elapsed());
    result.insert("lastUsed", lastUsed_.elapsed());
    result.insert("reUseCount", reUseCount_);
    result.insert("livingInThread", thread()->objectName());

    return result;
}

void ViridityConnection::close(QSharedPointer<ViridityConnection> reference)
{
    DGUARDMETHODTIMED;

    if (socket_ && socket_->isOpen())
        socket_->close();
}

void ViridityConnection::sharedPointerDeleteLater(ViridityConnection *connection)
{
    connection->deleteLater();
}

void ViridityConnection::setupConnection(QSharedPointer<ViridityConnection> reference)
{
    DGUARDMETHODTIMED;
    if (reference.data() != this) return;

    QWriteLocker l(&sharedMemberMREW_);

    // Open socket
    socket_ = QSharedPointer<ViridityTcpSocket>(
        new ViridityTcpSocket(reference),
        &ViridityTcpSocket::sharedPointerDeleteLater
    );

    connect(socket_.data(), SIGNAL(disconnected()), this, SLOT(handleSocketDisconnected()));

    // Attach incomming connection to socket
    if (!socket_->setSocketDescriptor(socketDescriptor_))
    {
        socket_.clear();
        return;
    }

    // Hand-off incoming connection to Tufao to parse request...
    request_ = QSharedPointer<ViridityHttpServerRequest>(
        new ViridityHttpServerRequest(socket_),
        &ViridityHttpServerRequest::sharedPointerDeleteLater
    );

    connect(request_.data(), SIGNAL(ready()), this, SLOT(handleRequestReady()));
    connect(request_.data(), SIGNAL(upgrade(QByteArray)), this, SLOT(handleRequestUpgrade(QByteArray)));

    requestUrl_ = request_->url();
    requestMethod_ = request_->method();

    DPRINTF("New connection from %s.", socket_->peerAddress().toString().toLatin1().constData());
}

void ViridityConnection::handleSocketDisconnected()
{
    DGUARDMETHODTIMED;

    // This mutex is required because we need exclusive access when changing our shared pointer members that are accessed from different threads!
    QWriteLocker l(&sharedMemberMREW_);

    handleResponseFinished();

    if (request_)
    {
        disconnect(request_.data(), SIGNAL(ready()), this, SLOT(handleRequestReady()));
        disconnect(request_.data(), SIGNAL(upgrade(QByteArray)), this, SLOT(handleRequestUpgrade(QByteArray)));
    }

    request_.clear();

    // Deref the socket. If no other requests or responses hold references, it will be freed.
    // If it will be freed and it holds the last reference to this connection, the connection will also be freed.
    if (socket_)
        disconnect(socket_.data(), SIGNAL(disconnected()), this, SLOT(handleSocketDisconnected()));

    socket_.clear();
}

void ViridityConnection::handleResponseFinished()
{
    DGUARDMETHODTIMED;

    // This mutex is required because we need exclusive access when changing our shared pointer members that are accessed from different threads!
    QWriteLocker l(&sharedMemberMREW_);

    if (response_)
        disconnect(response_.data(), SIGNAL(finished()), this, SLOT(handleResponseFinished()));

    response_.clear();
}

void ViridityConnection::handleRequestReady()
{
    //DGUARDMETHODTIMED;

    {
        QWriteLocker l(&sharedMemberMREW_);
        lastUsed_.restart();
        ++reUseCount_;

        requestUrl_ = request_->url();
        requestMethod_ = request_->method();
    }

    DPRINTF("Request for %s of connection from %s ready.", request_->url().constData(), request_->getPeerAddressFromRequest().constData());

    response_ = QSharedPointer<ViridityHttpServerResponse>(
        new ViridityHttpServerResponse(socket_, request_->responseOptions()),
        &ViridityHttpServerResponse::sharedPointerDeleteLater
    );

    connect(response_.data(), SIGNAL(finished()), this, SLOT(handleResponseFinished()));

    if (request_->headers().contains("Expect", "100-continue"))
        response_->writeContinue();

    if (server_->doesHandleRequest(request_))
    {
        server_->handleRequest(request_, response_);
    }
    else if (SSEHandler::staticDoesHandleRequest(server_, request_))
    {
        if (!sseHandler_)
            sseHandler_ = new SSEHandler(server_, this);

        sseHandler_->handleRequest(request_, response_);
    }
    else if (LongPollingHandler::staticDoesHandleRequest(server_, request_))
    {
        if (!longPollingHandler_)
            longPollingHandler_ = new LongPollingHandler(server_, this);

        longPollingHandler_->handleRequest(request_, response_);
    }
    else
    {
        response_->writeHead(404);
        response_->end("Not found");
    }
}

void ViridityConnection::handleRequestUpgrade(const QByteArray &head)
{
    lastUsed_.restart();
    ++reUseCount_;

    DPRINTF("Request for %s of connection from %s wants upgrade.", request_->url().constData(), request_->getPeerAddressFromRequest().constData());

    if (!webSocketHandler_)
        webSocketHandler_ = new WebSocketHandler(server_, this);

    webSocketHandler_->handleUpgrade(request_, head);
}

