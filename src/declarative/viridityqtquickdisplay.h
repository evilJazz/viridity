#ifndef VIRIDITYQTQUICKDISPLAYMANAGER_H
#define VIRIDITYQTQUICKDISPLAYMANAGER_H

#include <QObject>

#include "viriditydeclarative.h"
#include "graphicsscenedisplay.h"

class PrivateQtQuickDisplayManager;

class ViridityEncoderSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool useMultithreading READ useMultithreading WRITE setUseMultithreading NOTIFY useMultithreadingChanged)
    Q_PROPERTY(EncodingFormat patchEncodingFormat READ patchEncodingFormat WRITE setPatchEncodingFormat NOTIFY patchEncodingFormatChanged)
    Q_PROPERTY(bool alphaChannelEnabled READ alphaChannelEnabled WRITE setAlphaChannelEnabled NOTIFY alphaChannelEnabledChanged)
    Q_PROPERTY(int jpegQuality READ jpegQuality WRITE setJpegQuality NOTIFY jpegQualityChanged)
    Q_PROPERTY(int compressionLevel READ compressionLevel WRITE setCompressionLevel NOTIFY compressionLevelChanged)
    Q_PROPERTY(int inlineMaxBytes READ inlineMaxBytes WRITE setInlineMaxBytes NOTIFY inlineMaxBytesChanged)
    Q_ENUMS(EncodingFormat)
public:
    explicit ViridityEncoderSettings(QObject *parent = NULL);
    virtual ~ViridityEncoderSettings();

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

    void setUseMultithreading(bool value);
    bool useMultithreading() const { return settings_.useMultithreading; }

    void setPatchEncodingFormat(EncodingFormat format);
    EncodingFormat patchEncodingFormat() const { return static_cast<EncodingFormat>(settings_.patchEncodingFormat); }

    bool alphaChannelEnabled() const { return settings_.alphaChannelEnabled; }
    void setAlphaChannelEnabled(bool value);

    int jpegQuality() const { return settings_.jpegQuality; }
    void setJpegQuality(int value);

    int compressionLevel() const { return settings_.compressionLevel; }
    void setCompressionLevel(int value);

    int inlineMaxBytes() const { return settings_.inlineMaxBytes; }
    void setInlineMaxBytes(int value);

    const EncoderSettings &encoderSettings() const { return settings_; }

signals:
    void useMultithreadingChanged();
    void patchEncodingFormatChanged();
    void alphaChannelEnabledChanged();
    void jpegQualityChanged();
    void compressionLevelChanged();
    void inlineMaxBytesChanged();

    void settingsChanged();

private:
    EncoderSettings settings_;
};

class ViridityComparerSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int tileWidth READ tileWidth WRITE setTileWidth NOTIFY tileWidthChanged)
    Q_PROPERTY(bool useMultithreading READ useMultithreading WRITE setUseMultithreading NOTIFY useMultithreadingChanged)
    Q_PROPERTY(bool minifyTiles READ minifyTiles WRITE setMinifyTiles NOTIFY minifyTilesChanged)
    Q_PROPERTY(int minifyTileCountThreshold READ minifyTileCountThreshold WRITE setMinifyTileCountThreshold NOTIFY minifyTileCountThresholdChanged)
    Q_PROPERTY(bool analyzeFills READ analyzeFills WRITE setAnalyzeFills NOTIFY analyzeFillsChanged)
    Q_PROPERTY(bool analyzeMoves READ analyzeMoves WRITE setAnalyzeMoves NOTIFY analyzeMovesChanged)
    Q_PROPERTY(bool fineGrainedMoves READ fineGrainedMoves WRITE setFineGrainedMoves NOTIFY fineGrainedMovesChanged)
public:
    explicit ViridityComparerSettings(QObject *parent = NULL);
    virtual ~ViridityComparerSettings();

    void setTileWidth(int value);
    int tileWidth() const { return settings_.tileWidth; }

    void setUseMultithreading(bool value);
    bool useMultithreading() const { return settings_.useMultithreading; }

    void setMinifyTiles(bool value);
    bool minifyTiles() const { return settings_.minifyTiles; }

    void setMinifyTileCountThreshold(int value);
    int minifyTileCountThreshold() const { return settings_.minifyTileCountThreshold; }

    void setAnalyzeFills(bool value);
    bool analyzeFills() const { return settings_.analyzeFills; }

    void setAnalyzeMoves(bool value);
    bool analyzeMoves() const { return settings_.analyzeMoves; }

    void setFineGrainedMoves(bool value);
    bool fineGrainedMoves() const { return settings_.fineGrainedMoves; }

    const ComparerSettings &comparerSettings() const { return settings_; }

signals:
    void tileWidthChanged();
    void useMultithreadingChanged();
    void minifyTilesChanged();
    void minifyTileCountThresholdChanged();
    void analyzeFillsChanged();
    void analyzeMovesChanged();
    void fineGrainedMovesChanged();

    void settingsChanged();

private:
    ComparerSettings settings_;
};

class ViridityQtQuickDisplay : public ViridityDeclarativeBaseObject
{
    Q_OBJECT
    Q_PROPERTY(QString targetId READ targetId WRITE setTargetId)
    Q_PROPERTY(bool autoSize READ autoSize WRITE setAutoSize)
    Q_PROPERTY(QObject *displayItem READ displayItem WRITE setDisplayItem)
    Q_PROPERTY(ViridityEncoderSettings *encoderSettings READ encoderSettings NOTIFY encoderSettingsChanged)
    Q_PROPERTY(ViridityComparerSettings *comparerSettings READ comparerSettings NOTIFY comparerSettingsChanged)
    Q_CLASSINFO("DefaultProperty", "displayItem")
public:
    ViridityQtQuickDisplay(QObject *parent = NULL);
    virtual ~ViridityQtQuickDisplay();

    virtual void componentComplete();

    QObject *displayItem();
    void setDisplayItem(QObject *displayItem);

    QString targetId() const;
    bool autoSize() const;

    ViridityEncoderSettings *encoderSettings();
    ViridityComparerSettings *comparerSettings();

public slots:
    void setTargetId(QString targetId);
    void setAutoSize(bool autoSize);

signals:
    void encoderSettingsChanged();
    void comparerSettingsChanged();

private:
    PrivateQtQuickDisplayManager *manager_;

    QObject *displayItem_;
    QString targetId_;
    bool autoSize_;

    ViridityEncoderSettings *encoderSettings_;
    ViridityComparerSettings *comparerSettings_;
};

#endif // VIRIDITYQTQUICKDISPLAYMANAGER_H
