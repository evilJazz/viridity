#include <QtCore/QCoreApplication>

#include "moveanalyzer.h"

#define DEBUG
#include "debug.h"

void testFingerPrints(QImage *image, int templateWidth)
{
    qDebug("Getting fingerprints from %p using safe version...", image);
    AreaFingerPrints fingerPrintsSafe = getAreaFingerPrintsSlow(image, image->rect(), templateWidth);

    qDebug("Getting fingerprints from %p using optimized version...", image);
    AreaFingerPrints fingerPrintsOptimized = getAreaFingerPrintsFast(image, image->rect(), templateWidth);

    qDebug("Comparing fingerprints...");
    if (!fingerPrintsEqual(fingerPrintsSafe, fingerPrintsOptimized))
        qFatal("BUGGY OPTIMIZATION!!!");
    else
        qDebug("OPTIMIZATION IS OKAY.");
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QImage imageBefore;
    imageBefore.load("/home/darkstar/Pictures/Wallpapers/Desktopography 2009/Fight_for_Alpha2-1920x1200.jpg");
    imageBefore = imageBefore.convertToFormat(QImage::Format_ARGB32);

    QImage imageAfter;
    imageAfter.load("/home/darkstar/Pictures/Wallpapers/Desktopography 2009/Genesis-1920x963.jpg");
    imageAfter = imageAfter.convertToFormat(QImage::Format_ARGB32);

    testFingerPrints(&imageBefore, 66);
    testFingerPrints(&imageAfter, 66);

    DPROFILECLEARSTAT;

    AreaFingerPrints fingerPrints;

    for (int i = 0; i < 50; ++i)
        fingerPrints = getAreaFingerPrintsFast(&imageBefore, imageBefore.rect(), 128);


        //MoveAnalyzer moveAnalyzer(&imageBefore, &imageAfter, imageBefore.rect(), 128);

    DPROFILEPRINTSTAT;

    //return a.exec();
    return 0;
}
