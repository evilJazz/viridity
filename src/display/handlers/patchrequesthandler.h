#ifndef PATCHREQUESTHANDLER_H
#define PATCHREQUESTHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

#include "viridityrequesthandler.h"

class PatchRequestHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit PatchRequestHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~PatchRequestHandler();

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);
};

#endif // PATCHREQUESTHANDLER_H
