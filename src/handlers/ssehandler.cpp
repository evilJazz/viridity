#include "ssehandler.h"

#include <QUrl>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QUrlQuery>
#endif

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

bool SSEHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    return request->url().endsWith("/v/ev");
}

void SSEHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
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

            setUpResponse(response);
            response_->write(msg.toUtf8());
            response_->flush();

            if (session_->pendingMessagesAvailable())
                handleMessagesAvailable();
        }
        else
        {
            session_ = NULL;
            QString info = "data: inuse(" + session->id() + ")\n\n";

            setUpResponse(response);
            response_->write(info.toUtf8());
            response_->flush();
        }

        return;
    }
    else // start new connection
    {
        session_ = server()->sessionManager()->getNewSession();

        DPRINTF("NEW SESSION: %s", session_->id().toLatin1().constData());

        QString info = "data: info(" + session_->id() + ")\n\n";

        setUpResponse(response);
        response_->write(info.toUtf8());
        response_->flush();

        return;
    }

    response->writeHead(404);
    response->end("Not found");
}

void SSEHandler::setUpResponse(Tufao::HttpServerResponse *response)
{
    response_ = response;

    response_->headers().insert("Content-Type", "text/event-stream");
    ViridityConnection::addNoCachingResponseHeaders(response_);
    response_->writeHead(Tufao::HttpServerResponse::OK);

    connect(response_, SIGNAL(destroyed()), this, SLOT(handleResponseDestroyed()));

    if (session_)
        connect(session_, SIGNAL(newPendingMessagesAvailable()), this, SLOT(handleMessagesAvailable()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));
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
}
