#include "longpollinghandler.h"

#include <QUrl>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QUrlQuery>
#endif

#include <QStringList>

#include "viriditywebserver.h"
#include "inputposthandler.h"

#undef DEBUG
#include "KCL/debug.h"

LongPollingHandler::LongPollingHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent),
    session_(NULL)
{
    DGUARDMETHODTIMED;
}

LongPollingHandler::~LongPollingHandler()
{
    if (session_)
        server()->sessionManager()->releaseSession(session_);

    DGUARDMETHODTIMED;
}

bool LongPollingHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    return request->url().startsWith("/viridity?");
}

void LongPollingHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    DGUARDMETHODTIMED;

    QString id = UrlQuery(request->url()).queryItemValue("id");

    ViriditySession *session = server()->sessionManager()->getSession(id);

    if (session)
    {
        if (request->url().startsWith("/viridity?")) // long polling
        {
            if (request->method() == "GET") // long polling output
            {
                session_ = server()->sessionManager()->acquireSession(id);

                response_ = response;
                connect(response, SIGNAL(destroyed()), this, SLOT(handleResponseDestroyed()));

                connect(session_, SIGNAL(newPendingMessagesAvailable()), this, SLOT(handleMessagesAvailable()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));

                if (session_->pendingMessagesAvailable())
                    handleMessagesAvailable();

                return;
            }
            else if (request->method() == "POST") // long polling input
            {
                InputPostHandler *handler = new InputPostHandler(request, response, session);
                connect(response, SIGNAL(destroyed()), handler, SLOT(deleteLater()));
                return;
            }
        }

        return;
    }
    else if (id.isEmpty()/* && request->method() == "GET" && url.startsWith("/viridity?")*/) // start new connection
    {
        session = server()->sessionManager()->getNewSession();

        DPRINTF("NEW DISPLAY: %s", session->id().toLatin1().constData());

        QString info = "info(" + session->id() + ")";

        server()->sessionManager()->releaseSession(session);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", "text/plain; charset=utf8");
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(info.toUtf8());

        return;
    }

    response->writeHead(404);
    response->end("Not found");
}

void LongPollingHandler::handleMessagesAvailable()
{
    DGUARDMETHODTIMED;

    if (response_ && session_ && session_->pendingMessagesAvailable())
    {
        QList<QByteArray> messages = session_->takePendingMessages();

        server()->sessionManager()->releaseSession(session_);
        session_ = NULL;

        QByteArray out;
        foreach (const QByteArray &message, messages)
            out += message + "\n";

        response_->writeHead(Tufao::HttpServerResponse::OK);
        response_->headers().insert("Content-Type", "text/plain; charset=utf8");
        response_->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response_->headers().insert("Pragma", "no-cache");
        response_->end(out);
    }
}

void LongPollingHandler::handleResponseDestroyed()
{
    DGUARDMETHODTIMED;

    response_ = NULL;
}

