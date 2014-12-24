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

#include "graphicsscenewebcontrolcommandinterpreter.h"

class WebSocketHandler;
class LongPollingHandler;
class PatchRequestHandler;
class FileRequestHandler;

class GraphicsSceneMultiThreadedWebServer;
class GraphicsSceneDisplay;

class GraphicsSceneWebServerConnection : public QObject
{
    Q_OBJECT
public:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    explicit GraphicsSceneWebServerConnection(GraphicsSceneMultiThreadedWebServer *parent, qintptr socketDescriptor);
#else
    explicit GraphicsSceneWebServerConnection(GraphicsSceneMultiThreadedWebServer *parent, int socketDescriptor);
#endif

    virtual ~GraphicsSceneWebServerConnection();

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

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    qintptr socketDescriptor_;
#else
    int socketDescriptor_;
#endif
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

    GraphicsSceneWebControlCommandInterpreter *commandInterpreter();

protected:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    virtual void incomingConnection(qintptr handle);
#else
    virtual void incomingConnection(int handle);
#endif

private:
    QGraphicsScene *scene_;
    QMutex displayMutex_;
    QHash<QString, GraphicsSceneDisplay *> displays_;

    GraphicsSceneWebControlCommandInterpreter commandInterpreter_;

    QList<QThread *> connectionThreads_;
    int incomingConnectionCount_;

    QList<QThread *> displayThreads_;
};

#endif // GRAPHICSSCENEWEBCONTROL_H
