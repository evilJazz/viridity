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
#include "graphicsscenedisplaysessionmanager.h"

class WebSocketHandler;
class SSEHandler;
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
    SSEHandler *sseHandler_;
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
    explicit GraphicsSceneMultiThreadedWebServer(QObject *parent, GraphicsSceneDisplaySessionManager *sessionManager);
    virtual ~GraphicsSceneMultiThreadedWebServer();

    void listen(const QHostAddress &address, quint16 port, int threadsNumber);

    GraphicsSceneDisplaySessionManager *sessionManager();

protected:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    virtual void incomingConnection(qintptr handle);
#else
    virtual void incomingConnection(int handle);
#endif

private slots:
    void newDisplayCreated(GraphicsSceneDisplay *display);

private:
    GraphicsSceneDisplaySessionManager *sessionManager_;

    QList<QThread *> connectionThreads_;
    int incomingConnectionCount_;

    QList<QThread *> displayThreads_;
};

#endif // GRAPHICSSCENEWEBCONTROL_H
