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

/* GraphicsSceneFramePatch */

class GraphicsSceneFramePatch
{
public:
    QString id;
    QRect rect;
    int artefactMargin;
    QByteArray data;
    QByteArray mimeType;
    bool packedAlpha;

    QByteArray toBase64() const { return data.toBase64(); }
};

class GraphicsSceneDisplay;

class GraphicsSceneDisplayLocker
{
public:
    GraphicsSceneDisplayLocker(GraphicsSceneDisplay *display);
private:
    QMutexLocker m_;
};

struct EncoderSettings
{
    EncoderSettings() :
        useMultithreading(true),
        patchEncodingFormat(EncodingFormat_Auto),
        alphaChannelEnabled(true),
        jpegQuality(94),
        inlineMaxBytes(2048)
    {}

    bool useMultithreading;

    enum EncodingFormat
    {
        EncodingFormat_Raw = 1,
        EncodingFormat_PNG = 2,
        EncodingFormat_JPEG = 4,
        EncodingFormat_Auto = EncodingFormat_PNG | EncodingFormat_JPEG
    };

    EncodingFormat patchEncodingFormat;
    bool alphaChannelEnabled;
    int jpegQuality;
    int inlineMaxBytes;
};

class GraphicsSceneDisplay : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
public:
    explicit GraphicsSceneDisplay(const QString &id, QGraphicsScene *scene, GraphicsSceneWebControlCommandInterpreter *commandInterpreter);
    virtual ~GraphicsSceneDisplay();

    QString id() const { return id_; }

    const EncoderSettings &encoderSettings() const { return encoderSettings_; }
    void setEncoderSettings(const EncoderSettings &encoderSettings);

    const ComparerSettings comparerSettings() const;
    void setComparerSettings(const ComparerSettings &comparerSettings);

    QGraphicsScene *scene() const { return scene_; }

    bool isUpdateAvailable() const;
    GraphicsSceneFramePatch *takePatch(const QString &patchId);

    void requestFullUpdate();

signals:
    void updateAvailable();
    void newFrameMessagesGenerated(const QList<QByteArray> &messages);

protected:
    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual QList<QByteArray> takePendingMessages(bool returnBinary = false);

private slots:
    void sceneDamagedRegionsAvailable();
    void updateCheckTimerTimeout();
    void clientReady();

private:
    QGraphicsScene *scene_;
    GraphicsSceneWebControlCommandInterpreter *commandInterpreter_;

    QString id_;
    EncoderSettings encoderSettings_;

    int updateCheckInterval_;

    bool updateAvailable_;

    int frame_;

    QTimer *updateCheckTimer_;

    GraphicsSceneBufferRenderer *renderer_;
    bool clientReady_;

    friend class GraphicsSceneDisplayLocker;
    QHash<QString, GraphicsSceneFramePatch *> patches_;
    mutable QMutex patchesMutex_;
    bool fullUpdateRequested_;

    friend class GraphicsSceneDisplayThreadedCreatePatch;
    GraphicsSceneFramePatch *createPatch(const QRect &rect);

    friend class GraphicsSceneDisplayTests;
    friend class GraphicsSceneDisplayRecorder;
    const QHash<QString, GraphicsSceneFramePatch *> &patches() const { return patches_; } // use GraphicsSceneDisplayLocker to access or suffer!
    GraphicsSceneBufferRenderer &renderer() const { return *renderer_; }
    void clearPatches();

    void triggerUpdateCheckTimer();
};

#endif // GRAPHICSSCENEDISPLAY_H
