#include <QCoreApplication>

#include "graphicsscenedisplay.h"
#include "graphicsscenedisplayplayer.h"
#include "graphicsscenedisplayrecorder.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (a.arguments().count() > 2 && QFile::exists(a.arguments()[1]))
    {
        GraphicsSceneDisplay d("display", NULL, NULL);

        // Record again...
        GraphicsSceneDisplayRecorder *recorder = new GraphicsSceneDisplayRecorder(&d);
        recorder->setDiffFrameFilename(a.arguments()[2]);

        GraphicsSceneDisplayDumpIterator it;
        it.setFilename(a.arguments()[1]);

        while (it.advanceToNextFrame() > 0)
        {
            d.renderer().setSize(it.outputFrame().width(), it.outputFrame().height());
            d.renderer().pushFullFrame(it.outputFrame());
            static_cast<ViridityMessageHandler *>(&d)->takePendingMessages();
        }

        return 0;
    }
    else
    {
        qFatal("Please specify a source and destination filename.");
        return 1;
    }
}
