#include <QtCore/QCoreApplication>
#include <qtest.h>
#include <QtDebug>

#include <QBuffer>
#include <QByteArray>

#include <QPainter>

#include "display/graphicsscenedisplay.h"
#include "display/comparer/imagecomparer.h"
#include "display/private/graphicsscenedisplaytests.h"

#include "KCL/imageutils.h"

#ifdef VIRIDITY_USE_IMPROVED_JPEG
#include "private/jpegwriter.h"
#endif

class RecoderTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        inputFileName_ = "/home/darkstar/Desktop/full_dump_display1.fgsd";
        //inputFileName_ = "/home/darkstar/Desktop/diff_dump_display1.dgsd";

        testFrames_ = GraphicsSceneDisplayTests::getDecodedFrames(inputFileName_);

        comparerSettings_.useMultithreading = true;

        encoderSettings_.useMultithreading = true;
        encoderSettings_.patchEncodingFormat = EncoderSettings::EncodingFormat_PNG;

        qDebug("testFrames count: %d", testFrames_.count());
    }

    void benchmarkRecode()
    {
        QBENCHMARK
        {
            GraphicsSceneDisplayTests::nullEncodeFrames(testFrames_, &encoderSettings_, &comparerSettings_);
        }
    }
private:
    QString inputFileName_;

    QList<QImage> testFrames_;

    EncoderSettings encoderSettings_;
    ComparerSettings comparerSettings_;
};

QTEST_MAIN(RecoderTest)
#include "mainbenchmark.moc"
