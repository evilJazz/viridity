#include "viridityrequesthandler.h"

#include "viriditywebserver.h"

/* ViridityBaseRequestHandler */

ViridityBaseRequestHandler::ViridityBaseRequestHandler(ViridityConnection *parent) :
    QObject(parent),
    connection_(parent)
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
