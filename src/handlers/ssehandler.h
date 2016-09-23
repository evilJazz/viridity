#ifndef SSEHANDLER_H
#define SSEHANDLER_H

#include <QObject>

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
    void doHandleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);

private slots:
    void handleSessionInteractionDormant();
    void handleSessionReleaseRequired();
    void handleMessagesAvailable();
    void handleResponseDestroyed();

private:
    ViriditySession *session_;

    ViridityHttpServerResponse *response_;

    QAbstractSocket *socket_;

    void setUpResponse(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);
    void releaseSessionAndCloseConnection();
};

#endif // SSEHANDLER_H
