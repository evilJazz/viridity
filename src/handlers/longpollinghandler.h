#ifndef LONGPOLLINGHANDLER_H
#define LONGPOLLINGHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

class GraphicsSceneWebServerTask;
class GraphicsSceneDisplay;

class LongPollingHandler : public QObject
{
    Q_OBJECT
public:
    explicit LongPollingHandler(GraphicsSceneWebServerTask *parent);

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private slots:
    void handleDisplayUpdateAvailable();
    void handleResponseDestroyed();

private:
    GraphicsSceneWebServerTask *task_;
    GraphicsSceneDisplay *display_;

    Tufao::HttpServerResponse *response_;
};

#endif // LONGPOLLINGHANDLER_H
