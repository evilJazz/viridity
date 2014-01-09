#ifndef GRAPHICSSCENEWEBCONTROL_H
#define GRAPHICSSCENEWEBCONTROL_H

#include "viridity_global.h"

#include "graphicsscenebufferrenderer.h"
#include "graphicsscenewebcontrolcommandinterpreter.h"

#include <QTimer>
#include <QThread>

#include <QtNetwork/QTcpSocket>
#include <Tufao/HttpServer>
#include <Tufao/WebSocket>
#include <QBuffer>
#include <QMutex>
#include <QWaitCondition>


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

class WebServerInterface;
class Patch;

class GraphicsSceneWebServerConnection : public QObject
{
    Q_OBJECT

    friend class GraphicsSceneInputPostHandler;
public:
    explicit GraphicsSceneWebServerConnection(WebServerInterface *parent, Tufao::HttpServerRequest *request, const QByteArray &head);
    explicit GraphicsSceneWebServerConnection(WebServerInterface *parent, Tufao::HttpServerResponse *response);
    virtual ~GraphicsSceneWebServerConnection();

    QString id() const { return id_; }

private slots:
    void clientConnected(Tufao::HttpServerResponse *response = NULL);
    void clientDisconnected();
    void clientMessageReceived(QByteArray data);

    void sceneDamagedRegionsAvailable();

    void sendUpdate();

public slots:
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private:
    Patch *createPatch(const QRect &rect, bool createBase64);

    void sendCommand(const QString &cmd);

private:
    WebServerInterface *server_;
    Tufao::WebSocket *socket_;

    QStringList commands_;
    QMutex commandsMutex_;
    QWaitCondition commandsPresent_;

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

class CommandPostHandler : public QObject
{
    Q_OBJECT

public:
    explicit CommandPostHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, QObject *parent = 0);

private slots:
    void onData(const QByteArray &chunk);
    void onEnd();

private:
    Tufao::HttpServerRequest *request_;
    Tufao::HttpServerResponse *response_;
    QByteArray data_;
};

class CommandWebServer : public Tufao::HttpServer
{
    Q_OBJECT

public:
    explicit CommandWebServer(QObject *parent);

public slots:
    void handleRequest(Tufao::HttpServerRequest *request,
                       Tufao::HttpServerResponse * response);
};

#endif // GRAPHICSSCENEWEBCONTROL_H
