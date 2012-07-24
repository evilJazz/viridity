#ifndef GRAPHICSSCENEWEBCONTROL_H
#define GRAPHICSSCENEWEBCONTROL_H

#include "graphicsscenebufferrenderer.h"
#include "graphicsscenewebcontrolcommandinterpreter.h"

#include <QTimer>
#include <QThread>

#include <QtNetwork/QTcpSocket>
#include <HttpServer>
#include <WebSocket>
#include <QBuffer>

class WebServerInterface;
class Patch;

class GraphicsSceneWebServerConnection : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneWebServerConnection(WebServerInterface *parent, Tufao::HttpServerRequest *request, const QByteArray &head);
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
    WebServerInterface *server_;
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
    QMutex patchesMutex_;

    Patch *createPatch(const QRect &rect, bool createBase64);
};

class WebServerInterface
{
public:
    virtual ~WebServerInterface() {}
    virtual QGraphicsScene *scene() const = 0;

    virtual void addConnection(GraphicsSceneWebServerConnection *c) = 0;
    virtual void removeConnection(GraphicsSceneWebServerConnection *c) = 0;
    virtual GraphicsSceneWebServerConnection *getConnection(const QString &id) = 0;
};

class GraphicsSceneSingleThreadedWebServer : public Tufao::HttpServer, public WebServerInterface
{
    Q_OBJECT
public:
    explicit GraphicsSceneSingleThreadedWebServer(QObject *parent, QGraphicsScene *scene);
    QGraphicsScene *scene() const { return scene_; }

public slots:
    void handleRequest(Tufao::HttpServerRequest *request,
                       Tufao::HttpServerResponse *response);

protected:
    void upgrade(Tufao::HttpServerRequest *request, const QByteArray &head);
    friend class GraphicsSceneWebServerConnection;

    void addConnection(GraphicsSceneWebServerConnection *c);
    void removeConnection(GraphicsSceneWebServerConnection *c);
    GraphicsSceneWebServerConnection *getConnection(const QString &id);

private:
    QGraphicsScene *scene_;
    QHash<QString, GraphicsSceneWebServerConnection *> map_;
};

class GraphicsSceneMultiThreadedWebServer;

class GraphicsSceneWebServerThread : public QThread
{
    Q_OBJECT
public:
    explicit GraphicsSceneWebServerThread(GraphicsSceneMultiThreadedWebServer *parent);

    void addConnection(int socketDescriptor);

signals:
    void newConnection(int socketDescriptor);

private slots:
    void onNewConnection(int socketDescriptor);
    void onRequestReady();
    void onUpgrade(const QByteArray &);

private:
    GraphicsSceneMultiThreadedWebServer *server_;
};

class GraphicsSceneMultiThreadedWebServer : public QTcpServer, public WebServerInterface
{
    Q_OBJECT
public:
    explicit GraphicsSceneMultiThreadedWebServer(QObject *parent, QGraphicsScene *scene);
    QGraphicsScene *scene() const { return scene_; }

    virtual ~GraphicsSceneMultiThreadedWebServer();

    void listen(const QHostAddress &address, quint16 port, int threadsNumber);

    void addConnection(GraphicsSceneWebServerConnection *c);
    void removeConnection(GraphicsSceneWebServerConnection *c);
    GraphicsSceneWebServerConnection *getConnection(const QString &id);

signals:
    void newConnection(int socketDescriptor);

protected:
    void incomingConnection(int handle);

private:
    QList<GraphicsSceneWebServerThread*> threads;
    int i;

    QGraphicsScene *scene_;
    QMutex mapMutex_;
    QHash<QString, GraphicsSceneWebServerConnection *> map_;
};

#endif // GRAPHICSSCENEWEBCONTROL_H
