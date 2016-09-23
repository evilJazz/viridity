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

    bool doesHandleRequest(ViridityHttpServerRequest *request);
    void handleUpgrade(ViridityHttpServerRequest *request, const QByteArray &head);

private slots:
    void handleMessagesAvailable();
    void handleSessionInteractionDormant();

    void clientMessageReceived(QByteArray data);
    void clientDisconnected();

private:
    ViriditySession *session_;

    QAbstractSocket *socket_;
    Tufao::WebSocket *websocket_;
};

#endif // WEBSOCKETHANDLER_H
