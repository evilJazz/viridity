#include "sessionroutingrequesthandler.h"

#include "viriditywebserver.h"

SessionRoutingRequestHandler::SessionRoutingRequestHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent)
{
}

SessionRoutingRequestHandler::~SessionRoutingRequestHandler()
{
}

bool SessionRoutingRequestHandler::doesHandleRequest(ViridityHttpServerRequest *request)
{
    QString id = ViriditySession::parseIdFromUrl(request->url());
    ViriditySession *session = server()->sessionManager()->getSession(id);
    return session && session->doesHandleRequest(request);
}

void SessionRoutingRequestHandler::handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    QString id = ViriditySession::parseIdFromUrl(request->url());
    ViriditySession *session = server()->sessionManager()->getSession(id);

    if (session)
        session->handleRequest(request, response);
}
