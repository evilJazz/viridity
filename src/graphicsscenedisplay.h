#ifndef GRAPHICSSCENEDISPLAY_H
#define GRAPHICSSCENEDISPLAY_H

#include <QObject>
#include <QSharedPointer>
#include <QRect>
#include <QByteArray>
#include <QBuffer>
#include <QDateTime>
#include <QStringList>
#include <QTimer>

#include <QMutex>
#include <QWaitCondition>
#include <QThread>

#include <Tufao/HttpServer>
#include <Tufao/WebSocket>

#include "graphicsscenebufferrenderer.h"
#include "graphicsscenewebcontrolcommandinterpreter.h"

class GraphicsSceneMultiThreadedWebServer;

/* Patch */

class Patch
{
public:
    QString id;
    QRect rect;
    QBuffer data;
    QString mimeType;
    QByteArray dataBase64;
    QDateTime deadline;
};

typedef QSharedPointer<Patch> SharedDisplayPatch;

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

#endif // GRAPHICSSCENEDISPLAY_H
