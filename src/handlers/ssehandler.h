#ifndef SSEHANDLER_H
#define SSEHANDLER_H

#include <QObject>
#include <QTimer>

#include "viridityrequesthandler.h"

class ViriditySession;

class SSEHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit SSEHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~SSEHandler();

    static bool staticDoesHandleRequest(ViridityWebServer *server, ViridityHttpServerRequest *request);

    bool doesHandleRequest(ViridityHttpServerRequest *request);
    void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);

private slots:
    void handlePingTimerTimeout();
    void handleMessagesAvailable();
    void handleResponseDestroyed();

private:
    ViriditySession *session_;

    ViridityHttpServerResponse *response_;

    QTimer *pingTimer_;

    void setUpResponse(ViridityHttpServerResponse *response);
};

#endif // SSEHANDLER_H
