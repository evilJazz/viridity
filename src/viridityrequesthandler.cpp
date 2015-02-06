#include "viridityrequesthandler.h"

#include "viriditywebserver.h"

/* ViridityBaseRequestHandler */

ViridityBaseRequestHandler::ViridityBaseRequestHandler(ViridityWebServer *server, QObject *parent) :
    QObject(parent),
    server_(server)
{
}

ViridityBaseRequestHandler::~ViridityBaseRequestHandler()
{
}

bool ViridityBaseRequestHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    return false;
}

void ViridityBaseRequestHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
}
