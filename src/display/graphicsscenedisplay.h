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
#include "graphicsscenedisplaycommandinterpreter.h"

/* GraphicsSceneFramePatch */
/*! \internal */
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

/*! \internal */
class GraphicsSceneDisplayLocker
{
public:
    GraphicsSceneDisplayLocker(GraphicsSceneDisplay *display);
private:
    QMutexLocker m_;
};

/*!
    \addtogroup gsd
    @{
*/

/*!
 * The EncoderSettings struct holds settings used by the encoder of update patches of a GraphicsSceneDisplay.
 */

struct EncoderSettings
{
    EncoderSettings() :
        useMultithreading(true),
        patchEncodingFormat(EncodingFormat_Auto),
        alphaChannelEnabled(true),
        jpegQuality(94),
        compressionLevel(1),
        inlineMaxBytes(2048)
    {}

    /*! Defines whether to use multithreading for encoding image patches to their output format. */
    bool useMultithreading;

    /*! Enum defining which encoding format to use for image patches. */
    enum EncodingFormat
    {
        /*! Send as raw uncompressed image patches, currently BMP format is used internally. */
        EncodingFormat_Raw = 1,

        /*! Use lossless PNG format for image patches */
        EncodingFormat_PNG = 2,

        /*! Use lossy JPEG format for image patches */
        EncodingFormat_JPEG = 4,

        /*! Heuristically determine which format is best for a given image patch. */
        EncodingFormat_Auto = EncodingFormat_PNG | EncodingFormat_JPEG
    };

    /*! Specifies which encoding format to use for image patches. */
    EncodingFormat patchEncodingFormat;

    /*! Defines whether to include an alpha channel in the image patches or not.
     * Depends on whether the used GraphicsSceneAdapter has support for alpha channel rendering and if your application actually makes use of the alpha channel, ie. either on server- or client-side.
     * Disabling the alpha channel will reduce the size of image patches.
     *
     * NOTE: Since JPEG does not natively support alpha channel information, if EncoderSettings::patchEncodingFormat is set to EncoderSettings::EncodingFormat_JPEG,
     * the alpha channel will be put into the lower half of the image patch encoded to JPEG format.
     * The display rendering code on the client-side will properly re-combine the RGB content with the alpha channel.
     */
    bool alphaChannelEnabled;

    /*! Specifies which JPEG quality in percent is used to encode the image patches when EncoderSettings::patchEncodingFormat is set to EncoderSettings::EncodingFormat_JPEG.
     *
     * jpegQuality = 100 means highest quality, lowest compression.
     *
     * jpegQuality = 0 means lowest quality, highest compression.
     */
    int jpegQuality;

    /*! Specifies which lossless compression level should be used, eg. when EncoderSettings::patchEncodingFormat is set to EncoderSettings::EncodingFormat_PNG.
     *
     * compressionLevel = 1 means lowest compression.
     *
     * compressionLevel = 6 means highest compression, also slowest and barely useful.
     *
     * compressionLevel = 2 or 3 are a good trade-off between compression ratio and performance.
     */
    int compressionLevel;

    /*! Determines the maximal size of patches in octets/bytes that are send inline with the update commands issued to the client. Image patches bigger than the maximal size will be send via
     * a separate data url. The client can leverage pipelining to alleviate the higher overhead on non-inlined image patches.
     *
     * inlineMaxBytes = 0 means never inline data, always make available via separate data url.
     *
     * inlineMaxBytes = -1 means always inline data.
     */
    int inlineMaxBytes;
};

/*!
 * The GraphicsSceneDisplay class encapsulates a graphics scene adapter and command interpreter to represent one display and its state to a client.
 * One instance of GraphicsSceneDisplay is never shared between clients. It always holds the synchronized frame state of one client.
 */

class GraphicsSceneDisplay : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
public:
    explicit GraphicsSceneDisplay(const QString &id, AbstractGraphicsSceneAdapter *adapter, GraphicsSceneDisplayCommandInterpreter *commandInterpreter);
    virtual ~GraphicsSceneDisplay();

    QString id() const { return id_; }

    const EncoderSettings &encoderSettings() const { return encoderSettings_; }
    void setEncoderSettings(const EncoderSettings &encoderSettings);

    const ComparerSettings comparerSettings() const;
    void setComparerSettings(const ComparerSettings &comparerSettings);

    AbstractGraphicsSceneAdapter *adapter() const { return adapter_; }

    bool isUpdateAvailable() const;
    GraphicsSceneFramePatch *takePatch(const QString &patchId);

    void requestFullUpdate(bool forced = false);

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
    AbstractGraphicsSceneAdapter *adapter_;
    GraphicsSceneDisplayCommandInterpreter *commandInterpreter_;

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

/*! @}*/

#endif // GRAPHICSSCENEDISPLAY_H
