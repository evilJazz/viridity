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

#include "ssehandler.h"

#include <QStringList>

#include "viriditywebserver.h"
#include "viridityconnection.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

SSEHandler::SSEHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent),
    sessionMutex_(QMutex::Recursive),
    session_(NULL)
{
    DGUARDMETHODTIMED;
}

SSEHandler::~SSEHandler()
{
    releaseSessionAndCloseConnection();
    DGUARDMETHODTIMED;
}

bool SSEHandler::staticDoesHandleRequest(ViridityWebServer *server, QSharedPointer<ViridityHttpServerRequest> request)
{
    return request->url().endsWith("/v/ev");
}

bool SSEHandler::doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request)
{
    return staticDoesHandleRequest(server(), request);
}

void SSEHandler::doHandleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
{
    DGUARDMETHODTIMED;

    QString id = ViriditySession::parseIdFromUrl(request->url());

    ViriditySession *session = server()->sessionManager()->getSession(id);

    if (session)
    {
        session->testForInteractivity(5000, true);

        if (session->useCount() == 0)
        {
            QMutexLocker m(&sessionMutex_);

            session_ = server()->sessionManager()->acquireSession(id);
            QString msg = "data: reattached(" + session_->id() + ")\n\n";

            setUpResponse(request, response);
            response_->write(msg.toUtf8());
            response_->flush();

            if (session_->pendingMessagesAvailable())
                handleMessagesAvailable();
        }
        else
        {
            QMutexLocker m(&sessionMutex_);

            session_ = NULL;
            QString info = "data: inuse(" + session->id() + ")\n\n";

            setUpResponse(request, response);
            response_->end(info.toUtf8());
        }

        return;
    }
    else // start new connection
    {
        QMutexLocker m(&sessionMutex_);

        session_ = server()->sessionManager()->getNewSession(request->getPeerAddressFromRequest());

        DPRINTF("NEW SESSION: %s", session_->id().toLatin1().constData());

        QString info = "data: info(" + session_->id() + ")\n\n";

        setUpResponse(request, response);
        response_->write(info.toUtf8());
        response_->flush();

        return;
    }

    response->writeHead(404);
    response->end("Not found");
}

void SSEHandler::handleSessionInteractionDormant()
{
    DGUARDMETHODTIMED;
    QMutexLocker m(&sessionMutex_);

    if (response_ && socket_)
    {
        if (session_ && session_->lastUsed() > 1.5 * session_->interactionCheckInterval())
        {
            releaseSessionAndCloseConnection();
            return;
        }

        response_->write("data: ping()\n\n");
        response_->flush();
    }
}

void SSEHandler::releaseSessionAndCloseConnection()
{
    DGUARDMETHODTIMED;
    QMutexLocker m(&sessionMutex_);

    if (session_)
    {
        server()->sessionManager()->releaseSession(session_);
        session_ = NULL;
    }

    if (!handlingRequest() && socket_)
        socket_->close();
}

void SSEHandler::handleSessionReleaseRequired()
{
    DGUARDMETHODTIMED;
    releaseSessionAndCloseConnection();
}

void SSEHandler::setUpResponse(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
{
    response_ = response;

    response_->headers().insert("Content-Type", "text/event-stream");
    response_->addNoCachingResponseHeaders();
    response_->writeHead(ViridityHttpServerResponse::OK);

    connect(response_.data(), SIGNAL(finished()), this, SLOT(handleResponseFinished()));

    {
        QMutexLocker m(&sessionMutex_);
        if (session_)
        {
            connect(session_, SIGNAL(destroyed()), this, SLOT(handleSessionDestroyed()), (Qt::ConnectionType)(Qt::DirectConnection | Qt::UniqueConnection));
            connect(session_, SIGNAL(newPendingMessagesAvailable()), this, SLOT(handleMessagesAvailable()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));
            connect(session_, SIGNAL(interactionDormant()), this, SLOT(handleSessionInteractionDormant()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));
            connect(session_, SIGNAL(releaseRequired()), this, SLOT(handleSessionReleaseRequired()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));
        }
    }

    socket_ = request->socket();
    connect(socket_.data(), SIGNAL(disconnected()), this, SLOT(handleSocketDisconnected()));
}

void SSEHandler::handleMessagesAvailable()
{
    DGUARDMETHODTIMED;
    QMutexLocker m(&sessionMutex_);

    if (response_ && session_ && session_->pendingMessagesAvailable())
    {
        QList<QByteArray> messages = session_->takePendingMessages();

        QByteArray out;
        foreach (const QByteArray &message, messages)
            out += "data: " + message + "\n";

        response_->write(out + "\n");
        response_->flush();
    }
}

void SSEHandler::handleResponseFinished()
{
    DGUARDMETHODTIMED;

    response_.clear();
    socket_.clear();
}

void SSEHandler::handleSocketDisconnected()
{
    response_.clear();
    socket_.clear();
}

void SSEHandler::handleSessionDestroyed()
{
    QMutexLocker m(&sessionMutex_);
    session_ = NULL;
    QMetaObject::invokeMethod(this, "close");
}

void SSEHandler::close()
{
    releaseSessionAndCloseConnection();
}
