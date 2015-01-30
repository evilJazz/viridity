#ifndef SSEHANDLER_H
#define SSEHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

class ViridityConnection;
class GraphicsSceneDisplay;

class SSEHandler : public QObject
{
    Q_OBJECT
public:
    explicit SSEHandler(ViridityConnection *parent);
    virtual ~SSEHandler();

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private slots:
    void handleDisplayUpdateAvailable();
    void handleResponseDestroyed();

private:
    ViridityConnection *connection_;
    GraphicsSceneDisplay *display_;

    Tufao::HttpServerResponse *response_;

    void setUpResponse(Tufao::HttpServerResponse *response);
};

#endif // SSEHANDLER_H
