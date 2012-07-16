#ifndef GRAPHICSSCENEWEBCONTROL_H
#define GRAPHICSSCENEWEBCONTROL_H

#include "graphicsscenebufferrenderer.h"
#include "graphicsscenewebcontrolcommandinterpreter.h"

#include <QTimer>

#include <HttpServer>
#include <WebSocket>
#include <QBuffer>

class GraphicsSceneWebServer;
class Patch;

class GraphicsSceneWebServerConnection : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneWebServerConnection(GraphicsSceneWebServer *parent, Tufao::HttpServerRequest *request, const QByteArray &head);
    virtual ~GraphicsSceneWebServerConnection();

    QString id() const { return id_; }

private slots:
    void clientConnected();
    void clientDisconnected();
    void clientMessageReceived(QByteArray data);

    void sceneDamagedRegionsAvailable();

    void sendUpdate();

public slots:
    void handleRequest(Tufao::HttpServerRequest *request,
                       Tufao::HttpServerResponse *response);

private:
    GraphicsSceneWebServer *server_;
    Tufao::WebSocket *socket_;

    QString id_;
    bool urlMode_;
    int updateCheckInterval_;

    int frame_;

    GraphicsSceneWebControlCommandInterpreter commandInterpreter_;

    QTimer timer_;

    GraphicsSceneBufferRenderer *renderer_;
    bool clientReady_;

    QHash<QString, Patch *> patches_;

    Patch *createPatch(const QRect &rect, bool createBase64);
};

class GraphicsSceneWebServer : public Tufao::HttpServer
{
    Q_OBJECT
public:
    explicit GraphicsSceneWebServer(QObject *parent, QGraphicsScene *scene);
    QGraphicsScene *scene() const { return scene_; }

public slots:
    void handleRequest(Tufao::HttpServerRequest *request,
                       Tufao::HttpServerResponse *response);

protected:
    void upgrade(Tufao::HttpServerRequest *request, const QByteArray &head);
    friend class GraphicsSceneWebServerConnection;
    void removeConnection(GraphicsSceneWebServerConnection *c);

private:
    QGraphicsScene *scene_;
    QHash<QString, GraphicsSceneWebServerConnection *> map_;
};

#endif // GRAPHICSSCENEWEBCONTROL_H
