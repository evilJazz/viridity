#include "sessionroutingrequesthandler.h"

#include "viriditywebserver.h"

SessionRoutingRequestHandler::SessionRoutingRequestHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent)
{
}

SessionRoutingRequestHandler::~SessionRoutingRequestHandler()
{
}

bool SessionRoutingRequestHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    QString id = ViriditySession::parseIdFromUrl(request->url());
    ViriditySession *session = server()->sessionManager()->getSession(id);
    return session && session->doesHandleRequest(request);
}

void SessionRoutingRequestHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    QString id = ViriditySession::parseIdFromUrl(request->url());
    ViriditySession *session = server()->sessionManager()->getSession(id);

    if (session)
        session->handleRequest(request, response);
}
