#ifndef SESSIONROUTINGREQUESTHANDLER_H
#define SESSIONROUTINGREQUESTHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

#include "viridityrequesthandler.h"

class ViriditySession;

class SessionRoutingRequestHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit SessionRoutingRequestHandler(ViridityConnection *parent);
    virtual ~SessionRoutingRequestHandler();

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);
};

#endif // SESSIONROUTINGREQUESTHANDLER_H
