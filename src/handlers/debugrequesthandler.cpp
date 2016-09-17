#include "debugrequesthandler.h"

#include "viriditywebserver.h"
#include "viriditysessionmanager.h"

#include <QCoreApplication>
#include <QStringList>

DebugRequestHandler::DebugRequestHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent)
{
}

DebugRequestHandler::~DebugRequestHandler()
{
}

bool DebugRequestHandler::doesHandleRequest(ViridityHttpServerRequest *request)
{
    return request->url().endsWith("/debug") || request->url().endsWith("/quit");
}

void DebugRequestHandler::handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    response->writeHead(ViridityHttpServerResponse::OK);
    response->headers().insert("Content-Type", "text/plain");
    response->addNoCachingResponseHeaders();

    if (request->url().endsWith("/debug"))
    {
        if (server()->sessionManager()->sessionCount() > 0)
        {
            response->write("Running sessions:\n\n");

            foreach (const QString &sessionId, server()->sessionManager()->sessionIds())
            {
                ViriditySession *session = server()->sessionManager()->getSession(sessionId);
                response->write(
                    QString().sprintf(
                        "%s (%s) attached: %s, use count: %d, last used %ld ms ago.\n",
                        sessionId.toUtf8().constData(),
                        session->initialPeerAddress().constData(),
                        session->isAttached() ? "true" : "false",
                        session->useCount(),
                        session->lastUsed()
                    ).toUtf8()
                );
            }
        }
        else
        {
            response->write("No running session.\n");
        }
    }
    else if (request->url().endsWith("/quit"))
    {
        response->write("Sent message to quit server.\n");
        QMetaObject::invokeMethod(qApp, "quit");
    }

    response->end();
}
