#ifndef GRAPHICSSCENEINPUTPOSTHANDLER_H
#define GRAPHICSSCENEINPUTPOSTHANDLER_H

#include "Tufao/HttpServerRequest"



class GraphicsSceneWebServerConnection;

class GraphicsSceneInputPostHandler : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneInputPostHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, GraphicsSceneWebServerConnection *connection, QObject *parent = 0);

private slots:
    void onData(const QByteArray &chunk);
    void onEnd();

private:
    Tufao::HttpServerRequest *request_;
    Tufao::HttpServerResponse *response_;
    GraphicsSceneWebServerConnection *connection_;
    QByteArray data_;
};

#endif // GRAPHICSSCENEINPUTPOSTHANDLER_H
