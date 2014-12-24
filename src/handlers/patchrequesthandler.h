#ifndef PATCHREQUESTHANDLER_H
#define PATCHREQUESTHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

class GraphicsSceneWebServerConnection;
class GraphicsSceneDisplay;

class PatchRequestHandler : public QObject
{
    Q_OBJECT
public:
    explicit PatchRequestHandler(GraphicsSceneWebServerConnection *parent);

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private:
    GraphicsSceneWebServerConnection *task_;
};

#endif // PATCHREQUESTHANDLER_H
