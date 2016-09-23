#ifndef LONGPOLLINGHANDLER_H
#define LONGPOLLINGHANDLER_H

#include <QObject>

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
    void doHandleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);

private slots:
    void handleSessionInteractionDormant();
    void handleSessionReleaseRequired();
    void handleMessagesAvailable();
    void handleResponseDestroyed();

private:
    ViriditySession *session_;

    ViridityHttpServerResponse *response_;

    void pushMessageAndEnd(ViridityHttpServerResponse *response, const QByteArray &msg);
    void releaseSession();
};

#endif // LONGPOLLINGHANDLER_H
