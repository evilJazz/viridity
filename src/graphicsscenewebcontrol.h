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
#include <QElapsedTimer>

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

    GraphicsSceneDisplay *createDisplay();

    GraphicsSceneDisplay *getDisplay(const QString &id);

    GraphicsSceneDisplay *acquireDisplay(const QString &id);
    void releaseDisplay(GraphicsSceneDisplay *display);

    GraphicsSceneWebControlCommandInterpreter *commandInterpreter();

protected:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    virtual void incomingConnection(qintptr handle);
#else
    virtual void incomingConnection(int handle);
#endif

    void removeDisplay(GraphicsSceneDisplay *display);

private slots:
    void killObsoleteDisplays();

private:
    QGraphicsScene *scene_;
    QMutex displayMutex_;
    QHash<QString, GraphicsSceneDisplay *> displays_;

    GraphicsSceneWebControlCommandInterpreter commandInterpreter_;

    QList<QThread *> connectionThreads_;
    int incomingConnectionCount_;

    QList<QThread *> displayThreads_;

    struct DisplayResource {
        GraphicsSceneDisplay *display;
        QElapsedTimer lastUsed;
        int useCount;
    };

    QHash<GraphicsSceneDisplay *, DisplayResource> displayResources_;
    QTimer cleanupTimer_;
};

#endif // GRAPHICSSCENEWEBCONTROL_H
