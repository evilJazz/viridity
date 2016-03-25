#ifndef PATCHREQUESTHANDLER_H
#define PATCHREQUESTHANDLER_H

#include <QObject>

#include "viridityrequesthandler.h"

class PatchRequestHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit PatchRequestHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~PatchRequestHandler();

    bool doesHandleRequest(ViridityHttpServerRequest *request);
    void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);
};

#endif // PATCHREQUESTHANDLER_H
