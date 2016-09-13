#ifndef DEBUGREQUESTHANDLER_H
#define DEBUGREQUESTHANDLER_H

#include <QObject>

#include "viridityrequesthandler.h"

class DebugRequestHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit DebugRequestHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~DebugRequestHandler();

    bool doesHandleRequest(ViridityHttpServerRequest *request);
    void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);
};

#endif // DEBUGREQUESTHANDLER_H
