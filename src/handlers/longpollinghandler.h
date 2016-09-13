#ifndef LONGPOLLINGHANDLER_H
#define LONGPOLLINGHANDLER_H

#include <QObject>
#include <QTimer>

#include "viridityrequesthandler.h"

class ViriditySession;

class LongPollingHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit LongPollingHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~LongPollingHandler();

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

    void pushMessageAndEnd(ViridityHttpServerResponse *response, const QByteArray &msg);
};

#endif // LONGPOLLINGHANDLER_H
