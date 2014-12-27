#ifndef LONGPOLLINGHANDLER_H
#define LONGPOLLINGHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

class GraphicsSceneWebServerConnection;
class GraphicsSceneDisplay;

class LongPollingHandler : public QObject
{
    Q_OBJECT
public:
    explicit LongPollingHandler(GraphicsSceneWebServerConnection *parent);
    virtual ~LongPollingHandler();

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private slots:
    void handleDisplayUpdateAvailable();
    void handleResponseDestroyed();

private:
    GraphicsSceneWebServerConnection *connection_;
    GraphicsSceneDisplay *display_;

    Tufao::HttpServerResponse *response_;
};

#endif // LONGPOLLINGHANDLER_H
