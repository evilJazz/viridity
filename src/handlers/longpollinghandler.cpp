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

#include "longpollinghandler.h"

#include <QUrl>

#include <QStringList>

#include "viriditywebserver.h"
#include "inputposthandler.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

LongPollingHandler::LongPollingHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent),
    session_(NULL)
{
    DGUARDMETHODTIMED;
}

LongPollingHandler::~LongPollingHandler()
{
    releaseSession();
    DGUARDMETHODTIMED;
}

bool LongPollingHandler::staticDoesHandleRequest(ViridityWebServer *server, ViridityHttpServerRequest *request)
{
    QList<QByteArray> parts = request->url().split('?');
    return parts.count() > 0 && parts[0].endsWith("/v");
}

bool LongPollingHandler::doesHandleRequest(ViridityHttpServerRequest *request)
{
    return staticDoesHandleRequest(server(), request);
}

void LongPollingHandler::doHandleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    DGUARDMETHODTIMED;

    QString id = ViriditySession::parseIdFromUrl(request->url());

    ViriditySession *session = server()->sessionManager()->getSession(id);

    if (session)
    {
        if (request->method() == "GET") // long polling output
        {
            if (request->url().contains("?a=init"))
            {
                QByteArray msg;

                session->testForInteractivity(5000, true);

                if (session->useCount() == 0)
                {
                    session = server()->sessionManager()->acquireSession(id);
                    msg = "reattached(" + session->id().toLatin1() + ")";
                    server()->sessionManager()->releaseSession(session);

                    // NOTE: Pending messages will be handed out with the next poll below, i.e. when url does not contain ?a=init
                }
                else
                {
                    msg = "inuse(" + session->id().toLatin1() + ")";
                }

                pushMessageAndEnd(response, msg);
            }
            else
            {
                session_ = server()->sessionManager()->acquireSession(id);

                response_ = response;
                connect(response, SIGNAL(destroyed()), this, SLOT(handleResponseDestroyed()));

                connect(session_, SIGNAL(newPendingMessagesAvailable()), this, SLOT(handleMessagesAvailable()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));
                connect(session_, SIGNAL(interactionDormant()), this, SLOT(handleSessionInteractionDormant()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));
                connect(session_, SIGNAL(releaseRequired()), this, SLOT(handleSessionReleaseRequired()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));

                if (session_->pendingMessagesAvailable())
                    handleMessagesAvailable();
            }

            return;
        }
        else if (request->method() == "POST") // long polling input
        {
            InputPostHandler *handler = new InputPostHandler(request, response, session);
            connect(response, SIGNAL(destroyed()), handler, SLOT(deleteLater()));
            return;
        }
    }
    else // start new connection
    {
        session = server()->sessionManager()->getNewSession(request->getPeerAddressFromRequest());

        DPRINTF("NEW SESSION: %s", session->id().toLatin1().constData());

        QByteArray info = "info(" + session->id().toLatin1() + ")";

        server()->sessionManager()->releaseSession(session);

        pushMessageAndEnd(response, info);

        return;
    }

    response->writeHead(404);
    response->end("Not found");
}

void LongPollingHandler::handleSessionInteractionDormant()
{
    DGUARDMETHODTIMED;

    if (response_)
    {
        releaseSession();

        pushMessageAndEnd(response_, "ping()");
    }
}

void LongPollingHandler::releaseSession()
{
    DGUARDMETHODTIMED;

    if (session_)
    {
        server()->sessionManager()->releaseSession(session_);
        session_ = NULL;
    }
}

void LongPollingHandler::handleSessionReleaseRequired()
{
    DGUARDMETHODTIMED;
    releaseSession();
}

void LongPollingHandler::handleMessagesAvailable()
{
    DGUARDMETHODTIMED;

    if (response_ && session_ && session_->pendingMessagesAvailable())
    {
        QList<QByteArray> messages = session_->takePendingMessages();

        if (messages.count() > 0)
        {
            server()->sessionManager()->releaseSession(session_);
            session_ = NULL;

            QByteArray out;
            foreach (const QByteArray &message, messages)
                out += message + "\n";

            pushMessageAndEnd(response_, out);
        }
    }
}

void LongPollingHandler::handleResponseDestroyed()
{
    DGUARDMETHODTIMED;

    response_ = NULL;
}

void LongPollingHandler::pushMessageAndEnd(ViridityHttpServerResponse *response, const QByteArray &msg)
{
    response->writeHead(ViridityHttpServerResponse::OK);
    response->headers().insert("Content-Type", "text/plain; charset=utf8");
    response->addNoCachingResponseHeaders();
    response->end(msg);
}

