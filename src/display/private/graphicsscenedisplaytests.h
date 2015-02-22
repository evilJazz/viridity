#ifndef GRAPHICSSCENEDISPLAYTESTS_H
#define GRAPHICSSCENEDISPLAYTESTS_H

#include <QString>
#include <QList>
#include <QImage>

class GraphicsSceneDisplayTests
{
public:
    static void recodeRecording(const QString &inputDumpFileName, const QString &outputFileName);
    static void nullRecodeRecording(const QString &inputDumpFileName);
    static QList<QImage> getDecodedFrames(const QString &inputDumpFileName, int maxFrameCount = 0);
    static void nullEncodeFrames(const QList<QImage> frames);
};

#endif // GRAPHICSSCENEDISPLAYTESTS_H
