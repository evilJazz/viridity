#ifndef WEBSOCKETHANDLER_H
#define WEBSOCKETHANDLER_H

#include <QObject>
#include <QByteArray>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

class ViridityConnection;
class ViriditySession;

class WebSocketHandler : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketHandler(ViridityConnection *parent);
    virtual ~WebSocketHandler();

    void handleUpgrade(Tufao::HttpServerRequest *request, const QByteArray &head);
    bool doesHandleRequest(Tufao::HttpServerRequest *request);

private slots:
    void handleMessagesAvailable();

    void clientMessageReceived(QByteArray data);
    void clientDisconnected();

private:
    ViridityConnection *connection_;
    ViriditySession *session_;

    Tufao::WebSocket *socket_;
};

#endif // WEBSOCKETHANDLER_H
