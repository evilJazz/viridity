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

    bool isUpdateAvailable() const { return clientReady_ && patches_.count() == 0 && updateAvailable_; }
    Patch *takePatch(const QString &patchId);

    bool handleReceivedMessage(const QByteArray &data);
    bool handleReceivedMessage(const QString &msg, const QStringList &params);

    QStringList getCommandsForPendingUpdates();

signals:
    void updateAvailable();

private slots:
    void sceneDamagedRegionsAvailable();
    void sendUpdate();
    void clientReady();

private:
    GraphicsSceneMultiThreadedWebServer *server_;

    QString id_;
    bool urlMode_;
    int updateCheckInterval_;

    bool updateAvailable_;

    int frame_;

    QTimer *timer_;
    QThread *workerThread_;

    GraphicsSceneBufferRenderer *renderer_;
    bool clientReady_;

    QHash<QString, Patch *> patches_;
    QImage patchBuffer_;
    QMutex patchesMutex_;

    Patch *createPatch(const QRect &rect, bool createBase64);
    void clearPatches();
};

#endif // GRAPHICSSCENEDISPLAY_H
