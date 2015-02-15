#include <QApplication>

#include "graphicsscenedisplayplayer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GraphicsSceneDisplayPlayer player;
    player.setFilename("/home/darkstar/Desktop/full_dump_display1.fgsd");
    player.play();
    player.show();

    return a.exec();
}
