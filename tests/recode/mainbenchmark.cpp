#include <QtCore/QCoreApplication>
#include <qtest.h>
#include <QtDebug>

#include <QBuffer>
#include <QByteArray>

#include <QPainter>

#include "imagecomparer.h"
#include "moveanalyzer.h"
#include "graphicsscenedisplay.h"
#include "private/graphicsscenedisplaytests.h"

#include "KCL/imageutils.h"

#ifdef USE_IMPROVED_JPEG
#include "private/jpegwriter.h"
#endif

class RecoderTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        inputFileName_ = "/home/darkstar/Desktop/full_dump_display1.fgsd";

        testFrames_ = GraphicsSceneDisplayTests::getDecodedFrames(inputFileName_);
    }

    void benchmarkRecode()
    {
        QBENCHMARK
        {
            GraphicsSceneDisplayTests::nullEncodeFrames(testFrames_);
        }
    }
private:
    QString inputFileName_;

    QList<QImage> testFrames_;
};

QTEST_MAIN(RecoderTest)
#include "mainbenchmark.moc"
