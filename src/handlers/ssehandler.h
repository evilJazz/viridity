#ifndef SSEHANDLER_H
#define SSEHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

#include "viridityrequesthandler.h"

class ViriditySession;

class SSEHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit SSEHandler(ViridityConnection *parent);
    virtual ~SSEHandler();

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private slots:
    void handleMessagesAvailable();
    void handleResponseDestroyed();

private:
    ViriditySession *session_;

    Tufao::HttpServerResponse *response_;

    void setUpResponse(Tufao::HttpServerResponse *response);
};

#endif // SSEHANDLER_H
