#include <QCoreApplication>

#include "private/graphicsscenedisplaytests.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (a.arguments().count() > 2 && QFile::exists(a.arguments()[1]))
    {
        GraphicsSceneDisplayTests::recodeRecording(a.arguments()[1], a.arguments()[2]);
        return 0;
    }
    else
    {
        qFatal("Please specify a source and destination filename.");
        return 1;
    }
}
