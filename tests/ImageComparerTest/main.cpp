#include <QtCore/QCoreApplication>

#include "moveanalyzer.h"

#define DEBUG
#include "private/debug.h"

void testFingerPrints(QImage *image, int templateWidth)
{
    qDebug("Getting fingerprints from %p using safe version...", image);
    AreaFingerPrints fingerPrintsSafe;
    fingerPrintsSafe.initFromImageSlow(image, image->rect(), templateWidth);

    qDebug("Getting fingerprints from %p using optimized version...", image);
    AreaFingerPrints fingerPrintsOptimized;
    fingerPrintsOptimized.initFromImageFast(image, image->rect(), templateWidth);

    qDebug("Comparing fingerprints...");
    if (!fingerPrintsSafe.isEqual(fingerPrintsOptimized))
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

    const int testWidth = 128;

    QRect testRect1 = QRect(imageBefore.width() / 2, imageBefore.height() / 2, imageBefore.width(), imageBefore.height());
    QRect testRect2 = QRect(imageBefore.width() - testWidth, imageBefore.height() - testWidth, testWidth, testWidth);

    AreaFingerPrint templateFP;

/*
    for (int i = 0; i < 500; ++i)
        templateFP.initFromImage(&imageBefore, testRect2);

    for (int i = 0; i < 2; ++i)
        fingerPrints.initFromImageFast(&imageBefore, QRect(imageBefore.width() / 2, imageBefore.height() / 2, imageBefore.width(), imageBefore.height()), testWidth);
*/

    for (int i = 0; i < 50; ++i)
        fingerPrints.initFromImageThreaded(&imageBefore, imageBefore.rect(), testWidth);

    for (int i = 0; i < 50; ++i)
        fingerPrints.initFromImageFast(&imageBefore, imageBefore.rect(), testWidth);

    for (int i = 0; i < 50; ++i)
        fingerPrints.initFromImageSlow(&imageBefore, imageBefore.rect(), testWidth);

/*
    for (int i = 0; i < 10; ++i)
    {
        QPoint result;
        fingerPrints.findPosition(templateFP, testRect2, result);
    }
*/
    //MoveAnalyzer moveAnalyzer(&imageBefore, &imageAfter);

    DPROFILEPRINTSTAT;

    //return a.exec();
    return 0;
}
