#ifndef WEBSOCKETHANDLER_H
#define WEBSOCKETHANDLER_H

#include <QObject>
#include <QByteArray>

#include "viridityrequesthandler.h"

class ViriditySession;

namespace Tufao {
    class WebSocket;
}

class WebSocketHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit WebSocketHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~WebSocketHandler();

    void handleUpgrade(ViridityHttpServerRequest *request, const QByteArray &head);
    bool doesHandleRequest(ViridityHttpServerRequest *request);

private slots:
    void handleMessagesAvailable();

    void clientMessageReceived(QByteArray data);
    void clientDisconnected();

private:
    ViriditySession *session_;

    Tufao::WebSocket *socket_;
};

#endif // WEBSOCKETHANDLER_H
