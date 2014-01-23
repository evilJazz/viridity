#ifndef GRAPHICSSCENEWEBCONTROL_H
#define GRAPHICSSCENEWEBCONTROL_H

#include "viridity_global.h"

#include "KCL/backgroundtasks.h"

#include <QTimer>

#include <QtNetwork/QTcpSocket>
#include <Tufao/HttpServer>
#include <Tufao/WebSocket>
#include <QBuffer>
#include <QMutex>
#include <QWaitCondition>

#include <QGraphicsScene>

class WebSocketHandler;
class LongPollingHandler;
class PatchRequestHandler;
class FileRequestHandler;

class GraphicsSceneMultiThreadedWebServer;
class GraphicsSceneDisplay;

class GraphicsSceneWebServerTask : public EventLoopTask
{
    Q_OBJECT
public:
    explicit GraphicsSceneWebServerTask(GraphicsSceneMultiThreadedWebServer *parent, int socketDescriptor);

    GraphicsSceneMultiThreadedWebServer *server() { return server_; }

public slots:
    void setupConnection();

private slots:
    void onRequestReady();
    void onUpgrade(const QByteArray &);

private:
    WebSocketHandler *webSocketHandler_;
    LongPollingHandler *longPollingHandler_;
    PatchRequestHandler *patchRequestHandler_;
    FileRequestHandler *fileRequestHandler_;

    GraphicsSceneMultiThreadedWebServer *server_;
    int socketDescriptor_;
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
    QGraphicsScene *scene_;
    QMutex mapMutex_;
    QHash<QString, GraphicsSceneDisplay *> map_;

    TaskProcessingController *taskController_;
};

#endif // GRAPHICSSCENEWEBCONTROL_H
