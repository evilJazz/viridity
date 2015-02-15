#include <QApplication>

#include "graphicsscenedisplayplayer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GraphicsSceneDisplayPlayer player;
    player.setFilename("/home/darkstar/Desktop/diff_dump_display1.dgsd");
    player.play();
    player.show();

    return a.exec();
}
