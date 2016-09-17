#include "ssehandler.h"

#include <QStringList>

#include "viriditywebserver.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

SSEHandler::SSEHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent),
    session_(NULL)
{
    DGUARDMETHODTIMED;
}

SSEHandler::~SSEHandler()
{
    if (session_)
        server()->sessionManager()->releaseSession(session_);

    DGUARDMETHODTIMED;
}

bool SSEHandler::staticDoesHandleRequest(ViridityWebServer *server, ViridityHttpServerRequest *request)
{
    return request->url().endsWith("/v/ev");
}

bool SSEHandler::doesHandleRequest(ViridityHttpServerRequest *request)
{
    return staticDoesHandleRequest(server(), request);
}

void SSEHandler::handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    DGUARDMETHODTIMED;

    QString id = ViriditySession::parseIdFromUrl(request->url());

    ViriditySession *session = server()->sessionManager()->getSession(id);

    if (session)
    {
        if (session->useCount() == 0)
        {
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
            session_ = NULL;
            QString info = "data: inuse(" + session->id() + ")\n\n";

            setUpResponse(request, response);
            response_->write(info.toUtf8());
            response_->flush();
        }

        return;
    }
    else // start new connection
    {
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

    if (response_ && socket_)
    {
        if (session_ && session_->lastUsed() > 1.5 * session_->interactionCheckInterval())
        {
            socket_->close();
            return;
        }

        response_->write("data: ping()\n\n");
        response_->flush();
    }
}

void SSEHandler::setUpResponse(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    response_ = response;

    response_->headers().insert("Content-Type", "text/event-stream");
    response_->addNoCachingResponseHeaders();
    response_->writeHead(ViridityHttpServerResponse::OK);

    connect(response_, SIGNAL(destroyed()), this, SLOT(handleResponseDestroyed()));

    if (session_)
    {
        connect(session_, SIGNAL(newPendingMessagesAvailable()), this, SLOT(handleMessagesAvailable()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));
        connect(session_, SIGNAL(interactionDormant()), this, SLOT(handleSessionInteractionDormant()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));
    }

    socket_ = request->socket();
}

void SSEHandler::handleMessagesAvailable()
{
    DGUARDMETHODTIMED;

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

void SSEHandler::handleResponseDestroyed()
{
    DGUARDMETHODTIMED;

    response_ = NULL;
    socket_ = NULL;
}
