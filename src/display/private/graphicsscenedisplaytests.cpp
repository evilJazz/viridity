#include "graphicsscenedisplaytests.h"

#include "graphicsscenedisplay.h"
#include "tools/graphicsscenedisplayplayer.h"
#include "tools/graphicsscenedisplayrecorder.h"

void GraphicsSceneDisplayTests::recodeRecording(const QString &inputDumpFileName, const QString &outputFileName, EncoderSettings *encoderSettings, ComparerSettings *comparerSettings)
{
    GraphicsSceneDisplay d("display", NULL, NULL);

    if (encoderSettings)
        d.setEncoderSettings(*encoderSettings);

    if (comparerSettings)
        d.setComparerSettings(*comparerSettings);

    // Record again...
    GraphicsSceneDisplayRecorder recorder(&d);
    recorder.setDiffFrameFilename(outputFileName);

    GraphicsSceneDisplayDumpIterator it;
    it.setFilename(inputDumpFileName);

    while (it.advanceToNextFrame() > 0)
    {
        d.renderer().setSize(it.outputFrame().width(), it.outputFrame().height());
        d.renderer().pushFullFrame(it.outputFrame());
        recorder.setNextFrameTimeStamp(it.currentFrameTimeStamp());
        static_cast<ViridityMessageHandler *>(&d)->takePendingMessages();
        d.clearPatches();
    }
}

void GraphicsSceneDisplayTests::nullRecodeRecording(const QString &inputDumpFileName, EncoderSettings *encoderSettings, ComparerSettings *comparerSettings)
{
    GraphicsSceneDisplay d("display", NULL, NULL);

    if (encoderSettings)
        d.setEncoderSettings(*encoderSettings);

    if (comparerSettings)
        d.setComparerSettings(*comparerSettings);

    GraphicsSceneDisplayDumpIterator it;
    it.setFilename(inputDumpFileName);

    while (it.advanceToNextFrame() > 0)
    {
        d.renderer().setSize(it.outputFrame().width(), it.outputFrame().height());
        d.renderer().pushFullFrame(it.outputFrame());
        static_cast<ViridityMessageHandler *>(&d)->takePendingMessages();
        d.clearPatches();
    }
}

QList<QImage> GraphicsSceneDisplayTests::getDecodedFrames(const QString &inputDumpFileName, int maxFrameCount)
{
    QList<QImage> frames;

    GraphicsSceneDisplayDumpIterator it;
    it.setFilename(inputDumpFileName);

    int imageNo = 0;

    while (it.advanceToNextFrame() > 0)
    {
        frames << it.outputFrame();
        ++imageNo;

        if (maxFrameCount > 0 && imageNo == maxFrameCount)
            break;
    }

    return frames;
}

void GraphicsSceneDisplayTests::nullEncodeFrames(const QList<QImage> frames, EncoderSettings *encoderSettings, ComparerSettings *comparerSettings)
{
    GraphicsSceneDisplay d("display", NULL, NULL);

    if (encoderSettings)
        d.setEncoderSettings(*encoderSettings);

    if (comparerSettings)
        d.setComparerSettings(*comparerSettings);

    foreach (const QImage &frame, frames)
    {
        d.renderer().setSize(frame.width(), frame.height());
        d.renderer().pushFullFrame(frame);
        static_cast<ViridityMessageHandler *>(&d)->takePendingMessages();
        d.clearPatches();
    }
}
