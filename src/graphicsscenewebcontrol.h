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
#include <QRunnable>


class Patch;
class GraphicsSceneMultiThreadedWebServer;

class GraphicsSceneDisplay : public QObject
{
    Q_OBJECT

    friend class GraphicsSceneInputPostHandler;
public:
    explicit GraphicsSceneDisplay(GraphicsSceneMultiThreadedWebServer *parent, Tufao::HttpServerRequest *request, const QByteArray &head);
    explicit GraphicsSceneDisplay(GraphicsSceneMultiThreadedWebServer *parent, Tufao::HttpServerResponse *response);
    virtual ~GraphicsSceneDisplay();

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
    GraphicsSceneMultiThreadedWebServer *server_;
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

class GraphicsSceneMultiThreadedWebServer;

class GraphicsSceneWebServerThread : public QThread
{
    Q_OBJECT
public:
    explicit GraphicsSceneWebServerThread(GraphicsSceneMultiThreadedWebServer *parent);

public slots:
    void newConnection(int socketDescriptor);

private slots:
    void onRequestReady();
    void onUpgrade(const QByteArray &);

private:
    GraphicsSceneMultiThreadedWebServer *server_;
};

class GraphicsSceneMultiThreadedWebServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit GraphicsSceneMultiThreadedWebServer(QObject *parent, QGraphicsScene *scene);
    QGraphicsScene *scene() const { return scene_; }

    virtual ~GraphicsSceneMultiThreadedWebServer();

    void listen(const QHostAddress &address, quint16 port, int threadsNumber);

    void addDisplay(GraphicsSceneDisplay *c);
    void removeDisplay(GraphicsSceneDisplay *c);
    GraphicsSceneDisplay *getDisplay(const QString &id);

protected:
    virtual void incomingConnection(int handle);

private:
    QList<GraphicsSceneWebServerThread*> threads;
    int i;

    QGraphicsScene *scene_;
    QMutex mapMutex_;
    QHash<QString, GraphicsSceneDisplay *> map_;
};


#endif // GRAPHICSSCENEWEBCONTROL_H
