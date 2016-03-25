#ifndef SESSIONROUTINGREQUESTHANDLER_H
#define SESSIONROUTINGREQUESTHANDLER_H

#include <QObject>

#include "viridityrequesthandler.h"

class ViriditySession;

class SessionRoutingRequestHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit SessionRoutingRequestHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~SessionRoutingRequestHandler();

    bool doesHandleRequest(ViridityHttpServerRequest *request);
    void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);
};

#endif // SESSIONROUTINGREQUESTHANDLER_H
