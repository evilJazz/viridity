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
};

class GraphicsSceneDisplay : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneDisplay(const QString &id, QGraphicsScene *scene, GraphicsSceneWebControlCommandInterpreter *commandInterpreter);
    virtual ~GraphicsSceneDisplay();

    QString id() const { return id_; }

    bool isUpdateAvailable() const { return clientReady_ && patches_.count() == 0 && updateAvailable_; }
    Patch *takePatch(const QString &patchId);

    bool handleReceivedMessage(const QByteArray &data);
    bool handleReceivedMessage(const QString &msg, const QStringList &params);

    QStringList getCommandsForPendingUpdates();

    void dispatchAdditionalCommands(const QStringList &commands);

signals:
    void updateAvailable();

private slots:
    void sceneDamagedRegionsAvailable();
    void updateCheckTimerTimeout();
    void clientReady();

private:
    QGraphicsScene *scene_;
    GraphicsSceneWebControlCommandInterpreter *commandInterpreter_;

    QString id_;
    bool urlMode_;
    int updateCheckInterval_;

    bool updateAvailable_;

    int frame_;

    QTimer *updateCheckTimer_;

    GraphicsSceneBufferRenderer *renderer_;
    bool clientReady_;

    QHash<QString, Patch *> patches_;
    QImage patchBuffer_;
    QMutex patchesMutex_;

    QStringList additionalCommands_;

    Patch *createPatch(const QRect &rect, bool createBase64);
    void clearPatches();

    void triggerUpdateCheckTimer();
};

#endif // GRAPHICSSCENEDISPLAY_H
