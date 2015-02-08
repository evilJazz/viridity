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

/* Patch */

class Patch
{
public:
    QString id;
    QRect rect;
    int artefactMargin;
    QBuffer data;
    QByteArray mimeType;
    bool packedAlpha;

    QByteArray toBase64() const { return data.data().toBase64(); }
};

class GraphicsSceneDisplay : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
public:
    explicit GraphicsSceneDisplay(const QString &id, QGraphicsScene *scene, GraphicsSceneWebControlCommandInterpreter *commandInterpreter);
    virtual ~GraphicsSceneDisplay();

    QString id() const { return id_; }

    bool isUpdateAvailable() const;
    Patch *takePatch(const QString &patchId);

signals:
    void updateAvailable();

protected:
    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual QList<QByteArray> takePendingMessages();

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
    mutable QMutex patchesMutex_;

    Patch *createPatch(const QRect &rect);
    void clearPatches();

    void triggerUpdateCheckTimer();
    QImage createPackedAlphaPatch(const QImage &input);
};

#endif // GRAPHICSSCENEDISPLAY_H
