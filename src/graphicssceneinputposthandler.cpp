#include "graphicssceneinputposthandler.h"

#include "graphicsscenewebcontrol.h"


GraphicsSceneInputPostHandler::GraphicsSceneInputPostHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, GraphicsSceneDisplay *display, QObject *parent) :
    QObject(parent),
    request_(request),
    response_(response),
    display_(display)
{
    connect(request_, SIGNAL(data(QByteArray)), this, SLOT(onData(QByteArray)));
    connect(request_, SIGNAL(end()), this, SLOT(onEnd()));
}

void GraphicsSceneInputPostHandler::onData(const QByteArray &chunk)
{
    data_ += chunk;
}

void GraphicsSceneInputPostHandler::onEnd()
{
    QList<QByteArray> commands = data_.split('\n');

    foreach (QByteArray command, commands)
        display_->clientMessageReceived(command);

    // handle request
    response_->writeHead(Tufao::HttpServerResponse::OK);
    response_->end();
}

