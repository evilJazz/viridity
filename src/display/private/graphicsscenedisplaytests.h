#ifndef GRAPHICSSCENEDISPLAYTESTS_H
#define GRAPHICSSCENEDISPLAYTESTS_H

#include <QString>
#include <QList>
#include <QImage>

struct EncoderSettings;
struct ComparerSettings;

class GraphicsSceneDisplayTests
{
public:
    static void recodeRecording(const QString &inputDumpFileName, const QString &outputFileName, EncoderSettings *encoderSettings = NULL, ComparerSettings *comparerSettings = NULL);
    static void nullRecodeRecording(const QString &inputDumpFileName, EncoderSettings *encoderSettings = NULL, ComparerSettings *comparerSettings = NULL);
    static QList<QImage> getDecodedFrames(const QString &inputDumpFileName, int maxFrameCount = 0);
    static void nullEncodeFrames(const QList<QImage> frames, EncoderSettings *encoderSettings = NULL, ComparerSettings *comparerSettings = NULL);
};

#endif // GRAPHICSSCENEDISPLAYTESTS_H
