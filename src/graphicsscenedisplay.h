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

class GraphicsSceneDisplay : public QObject
{
    Q_OBJECT

    friend class GraphicsSceneInputPostHandler;
public:
    explicit GraphicsSceneDisplay(GraphicsSceneMultiThreadedWebServer *parent);
    virtual ~GraphicsSceneDisplay();

    QString id() const { return id_; }

    bool isUpdateAvailable() { return updateAvailable_; }
    Patch *takePatch(const QString &patchId);

    bool sendCommand(const QByteArray &data);
    bool sendCommand(const QString &command, const QStringList &params);

    QStringList getUpdateCommandList();

signals:
    void updateAvailable();

public slots:
    void clientReady();

private slots:
    void sceneDamagedRegionsAvailable();
    void sendUpdate();

private:
    Patch *createPatch(const QRect &rect, bool createBase64);

private:
    GraphicsSceneMultiThreadedWebServer *server_;

    QStringList commands_;
    QMutex commandsMutex_;

    QString id_;
    bool urlMode_;
    int updateCheckInterval_;

    bool updateAvailable_;

    int frame_;

    GraphicsSceneWebControlCommandInterpreter commandInterpreter_;

    QTimer timer_;

    GraphicsSceneBufferRenderer *renderer_;
    bool clientReady_;

    QHash<QString, Patch *> patches_;
    QMutex patchesMutex_;
};

#endif // GRAPHICSSCENEDISPLAY_H
