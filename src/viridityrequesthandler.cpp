#include "viridityrequesthandler.h"

#include "viriditywebserver.h"

/* ViridityHttpServerRequest */

ViridityHttpServerRequest::ViridityHttpServerRequest(QAbstractSocket *socket, QObject *parent) :
    Tufao::HttpServerRequest(socket, parent)
{
}

ViridityHttpServerRequest::~ViridityHttpServerRequest()
{
}

QByteArray ViridityHttpServerRequest::getPeerAddressFromRequest() const
{
    if (!socket())
        return QByteArray();

    QByteArray peerAddress = socket()->peerAddress().toString().toLatin1();
    if (headers().contains("X-Forwarded-For"))
        peerAddress = headers().value("X-Forwarded-For");

    return peerAddress;
}

/* ViridityHttpServerResponse */

ViridityHttpServerResponse::ViridityHttpServerResponse(QIODevice *device, Tufao::HttpServerResponse::Options options, QObject *parent) :
    Tufao::HttpServerResponse(device, options, parent)
{
}

ViridityHttpServerResponse::~ViridityHttpServerResponse()
{
}

void ViridityHttpServerResponse::addNoCachingResponseHeaders()
{
    headers().insert("Cache-Control", "no-store, no-cache, must-revalidate");
    headers().insert("Pragma", "no-cache");
    headers().insert("Expires", "0");
}

/* ViridityBaseRequestHandler */

ViridityBaseRequestHandler::ViridityBaseRequestHandler(ViridityWebServer *server, QObject *parent) :
    QObject(parent),
    server_(server)
{
}

ViridityBaseRequestHandler::~ViridityBaseRequestHandler()
{
}

bool ViridityBaseRequestHandler::doesHandleRequest(ViridityHttpServerRequest *request)
{
    return false;
}

void ViridityBaseRequestHandler::handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
}
