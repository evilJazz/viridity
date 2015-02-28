#include <QApplication>

#include "display/tools/graphicsscenedisplayplayer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (a.arguments().count() > 1 && QFile::exists(a.arguments()[1]))
    {
        GraphicsSceneDisplayPlayer player;
        player.setFilename(a.arguments()[1]);
        player.play();
        player.show();

        return a.exec();
    }
    else
    {
        qFatal("Please specify a filename.");
        return 1;
    }
}
