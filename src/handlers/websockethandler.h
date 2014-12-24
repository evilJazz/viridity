#ifndef WEBSOCKETHANDLER_H
#define WEBSOCKETHANDLER_H

#include <QObject>
#include <QByteArray>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

class GraphicsSceneWebServerConnection;
class GraphicsSceneDisplay;

class WebSocketHandler : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketHandler(GraphicsSceneWebServerConnection *parent);
    virtual ~WebSocketHandler();

    void handleUpgrade(Tufao::HttpServerRequest *request, const QByteArray &head);
    bool doesHandleRequest(Tufao::HttpServerRequest *request);

private slots:
    void handleDisplayUpdateAvailable();

    void clientMessageReceived(QByteArray data);
    void clientDisconnected();

private:
    GraphicsSceneWebServerConnection *task_;
    GraphicsSceneDisplay *display_;

    Tufao::WebSocket *socket_;
};

#endif // WEBSOCKETHANDLER_H
