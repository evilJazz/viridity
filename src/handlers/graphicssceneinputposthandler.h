#ifndef GRAPHICSSCENEINPUTPOSTHANDLER_H
#define GRAPHICSSCENEINPUTPOSTHANDLER_H

#include "Tufao/HttpServerRequest"

class GraphicsSceneDisplay;

class GraphicsSceneInputPostHandler : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneInputPostHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, GraphicsSceneDisplay *display, QObject *parent = 0);

private slots:
    void onData(const QByteArray &chunk);
    void onEnd();

private:
    Tufao::HttpServerRequest *request_;
    Tufao::HttpServerResponse *response_;
    GraphicsSceneDisplay *display_;
    QByteArray data_;
};

#endif // GRAPHICSSCENEINPUTPOSTHANDLER_H
