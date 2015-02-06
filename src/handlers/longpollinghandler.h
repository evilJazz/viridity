#ifndef LONGPOLLINGHANDLER_H
#define LONGPOLLINGHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

#include "viridityrequesthandler.h"

class ViriditySession;

class LongPollingHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit LongPollingHandler(ViridityConnection *parent);
    virtual ~LongPollingHandler();

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private slots:
    void handleMessagesAvailable();
    void handleResponseDestroyed();

private:
    ViriditySession *session_;

    Tufao::HttpServerResponse *response_;
};

#endif // LONGPOLLINGHANDLER_H
