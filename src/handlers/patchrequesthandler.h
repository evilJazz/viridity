#ifndef PATCHREQUESTHANDLER_H
#define PATCHREQUESTHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

class GraphicsSceneWebServerTask;
class GraphicsSceneDisplay;

class PatchRequestHandler : public QObject
{
    Q_OBJECT
public:
    explicit PatchRequestHandler(GraphicsSceneWebServerTask *parent);

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private:
    GraphicsSceneWebServerTask *task_;
};

#endif // PATCHREQUESTHANDLER_H