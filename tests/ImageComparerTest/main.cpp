#include <QtCore/QCoreApplication>
#include <qtest.h>
#include <QtDebug>

#include <QBuffer>
#include <QByteArray>

#include <QPainter>

#include "comparer/imagecomparer.h"
#include "comparer/moveanalyzer.h"
#include "comparer/areafingerprint.h"
#include "comparer/imageaux.h"
#include "graphicsscenedisplay.h"

#include "KCL/imageutils.h"

#ifdef USE_IMPROVED_JPEG
#include "private/jpegwriter.h"
#endif

#ifdef USE_IMPROVED_PNG
#include "private/pngwriter.h"
#endif

class ImageComparerTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        testWidth_ = 64;

        lena_.load(":/testimages/lena.png");
        lena_ = lena_.convertToFormat(QImage::Format_ARGB32);

        wikitext_.load(":/testimages/wikitext.png");
        wikitext_ = wikitext_.convertToFormat(QImage::Format_ARGB32);

        imageBefore_ = QImage(1920, 1200, QImage::Format_ARGB32);

        QPainter p;
        p.begin(&imageBefore_);
        p.drawImage(200, 200, lena_);
        p.end();

        imageAfter_ = QImage(1921, 1201, QImage::Format_ARGB32);
        p.begin(&imageAfter_);
        p.drawImage(400, 100, lena_);
        p.end();

        testRect1_ = QRect(imageBefore_.width() / 2, imageBefore_.height() / 2, imageBefore_.width(), imageBefore_.height());

        testTemplateRect_ = QRect(lena_.width() - testWidth_, lena_.height() - testWidth_, testWidth_, testWidth_);

        imageComparer_ = new ImageComparer(&imageBefore_, &imageAfter_);
    }

    void checkInitFromImageEqual()
    {
        qDebug("Getting fingerprints from %p using safe version...", &lena_);
        AreaFingerPrints fingerPrintsSafe;
        fingerPrintsSafe.initFromImage(&lena_, lena_.rect(), testWidth_);

        qDebug("Getting fingerprints from %p using optimized version...", &lena_);
        AreaFingerPrints fingerPrintsOptimized;
        fingerPrintsOptimized.initFromImageThreaded(&lena_, lena_.rect(), testWidth_);

        qDebug("Comparing fingerprints...");
        QVERIFY(fingerPrintsSafe.isEqual(fingerPrintsOptimized));
    }

    void benchmarkInitFromImage()
    {
        AreaFingerPrints fingerPrints;

        QBENCHMARK
        {
            fingerPrints.initFromImage(&imageBefore_, imageBefore_.rect(), testWidth_);
        }
    }

    void benchmarkInitFromImageThreaded()
    {
        AreaFingerPrints fingerPrints;

        QBENCHMARK
        {
            fingerPrints.initFromImageThreaded(&imageBefore_, imageBefore_.rect(), testWidth_);
        }
    }

    void benchmarkFPinitFromImage()
    {
        AreaFingerPrint templateFP;

        QBENCHMARK
        {
            templateFP.initFromImage(&lena_, testTemplateRect_);
        }
    }

    void benchmarkFindPosition()
    {
        AreaFingerPrints fingerPrints;
        fingerPrints.initFromImage(&lena_, lena_.rect(), testWidth_);

        AreaFingerPrint templateFP;
        templateFP.initFromImage(&lena_, testTemplateRect_);

        QPoint result;
        QBENCHMARK
        {
            fingerPrints.findPosition(templateFP, testTemplateRect_, result);
        }

        //qDebug() << "Result: " << result;
    }

    void benchmarkFindMovedRectExhaustive()
    {
        QBENCHMARK
        {
            imageComparer_->swap();
            imageComparer_->moveAnalyzer_->findMovedRectExhaustive(imageBefore_.rect(), testTemplateRect_);
        }
    }

    void benchmarkFindMovedRectAreaFingerPrint()
    {
        QBENCHMARK
        {
            imageComparer_->swap();
            imageComparer_->moveAnalyzer_->findMovedRectAreaFingerPrint(imageBefore_.rect(), testTemplateRect_);
        }
    }

    void benchmarkFindMovedRect()
    {
        QBENCHMARK
        {
            imageComparer_->swap();
            imageComparer_->moveAnalyzer_->findMovedRect(imageBefore_.rect(), testTemplateRect_);
        }
    }

    void benchmarkImageComprarerFindDifferences()
    {
        QBENCHMARK
        {
            imageComparer_->swap();
            imageComparer_->findDifferences();
        }
    }

    void benchmarkImageComparerSwap()
    {
        QBENCHMARK
        {
            imageComparer_->swap();
        }
    }

    void benchmarkMoveAnalyzerGrayscaleConversion()
    {
        QBENCHMARK
        {
            imageComparer_->swap();
            imageComparer_->moveAnalyzer_->ensureImagesUpdated();
        }
    }

    void checkImageComparerFindUpdateOperations()
    {
        imageComparer_->swap();
        QVERIFY(imageComparer_->findUpdateOperations(imageBefore_.rect()).count() > 0);
    }

    void benchmarkImageComparerFindUpdateOperations()
    {
        QBENCHMARK
        {
            imageComparer_->swap();
            imageComparer_->findUpdateOperations(imageBefore_.rect());
        }
    }

    void benchmarkSaveToBMP()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            lena_.save(&buffer, "BMP");
            buffer.close();
        }
    }

    void benchmarkEstimatePNGCompressionLena()
    {
        QBENCHMARK
        {
            ImageAux::estimatePNGCompression(lena_);
        }
    }

    void benchmarkEstimatePNGCompressionWikiText()
    {
        QBENCHMARK
        {
            ImageAux::estimatePNGCompression(wikitext_);
        }
    }

    void benchmarkSaveToPNG1Lena()
    {
        QBuffer buffer;

        QBENCHMARK
        {
            buffer.open(QIODevice::ReadWrite);
            lena_.save(&buffer, "PNG", ImageAux::zlibCompressionLevelToQPNGHandlerQuality(1));
            buffer.reset();
            buffer.close();
        }

        qDebug("buffer.size: %d", buffer.size());
    }

    void benchmarkSaveToPNG1WikiText()
    {
        QBuffer buffer;

        QBENCHMARK
        {
            buffer.open(QIODevice::ReadWrite);
            wikitext_.save(&buffer, "PNG", ImageAux::zlibCompressionLevelToQPNGHandlerQuality(1));
            buffer.reset();
            buffer.close();
        }

        qDebug("buffer.size: %d", buffer.size());
    }

    void benchmarkSaveToPNG9Lena()
    {
        QBuffer buffer;

        QBENCHMARK
        {
            buffer.open(QIODevice::ReadWrite);
            lena_.save(&buffer, "PNG", ImageAux::zlibCompressionLevelToQPNGHandlerQuality(9));
            buffer.reset();
            buffer.close();
        }

        qDebug("buffer.size: %d", buffer.size());
    }

    void benchmarkSaveToPNG9WikiText()
    {
        QBuffer buffer;

        QBENCHMARK
        {
            buffer.open(QIODevice::ReadWrite);
            wikitext_.save(&buffer, "PNG", ImageAux::zlibCompressionLevelToQPNGHandlerQuality(9));
            buffer.reset();
            buffer.close();
        }

        qDebug("buffer.size: %d", buffer.size());
    }

    void benchmarkSaveToPNGOpt1Lena()
    {
        QBuffer buffer;

        QBENCHMARK
        {
            buffer.open(QIODevice::ReadWrite);
            writePNG(lena_, &buffer, 1, (PNGFilterFlag)(PNGFilterNone | PNGFilterSub | PNGFilterUp | PNGFilterAvg));
            buffer.reset();
            buffer.close();
        }

        qDebug("buffer.size: %d", buffer.size());
    }

    void benchmarkSaveToPNGOpt1WikiText()
    {
        QBuffer buffer;

        QBENCHMARK
        {
            buffer.open(QIODevice::ReadWrite);
            writePNG(wikitext_, &buffer, 1, (PNGFilterFlag)(PNGFilterNone | PNGFilterSub | PNGFilterUp | PNGFilterAvg));
            buffer.reset();
            buffer.close();
        }

        qDebug("buffer.size: %d", buffer.size());
    }

    void benchmarkSaveToPNGOpt9Lena()
    {
        QBuffer buffer;

        QBENCHMARK
        {
            buffer.open(QIODevice::ReadWrite);
            writePNG(lena_, &buffer, 9, (PNGFilterFlag)(PNGFilterNone | PNGFilterSub | PNGFilterUp | PNGFilterAvg));
            buffer.reset();
            buffer.close();
        }

        qDebug("buffer.size: %d", buffer.size());
    }

    void benchmarkSaveToPNGOpt9WikiText()
    {
        QBuffer buffer;

        QBENCHMARK
        {
            buffer.open(QIODevice::ReadWrite);
            writePNG(wikitext_, &buffer, 9, (PNGFilterFlag)(PNGFilterNone | PNGFilterSub | PNGFilterUp | PNGFilterAvg));
            buffer.reset();
            buffer.close();
        }

        qDebug("buffer.size: %d", buffer.size());
    }

    void benchmarkSaveToQCompress1Lena()
    {
        QBENCHMARK
        {
            QByteArray compressedRaw = qCompress(lena_.constBits(), lena_.byteCount(), 1);
        }
    }

    void benchmarkSaveToQCompress9Lena()
    {
        QBENCHMARK
        {
            QByteArray compressedRaw = qCompress(lena_.constBits(), lena_.byteCount(), 9);
        }
    }

    void benchmarkSaveToQCompress1WikiText()
    {
        QBENCHMARK
        {
            QByteArray compressedRaw = qCompress(wikitext_.constBits(), wikitext_.byteCount(), 1);
        }
    }

    void benchmarkSaveToQCompress9WikiText()
    {
        QBENCHMARK
        {
            QByteArray compressedRaw = qCompress(wikitext_.constBits(), wikitext_.byteCount(), 9);
        }
    }

    void benchmarkSaveToGIF()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            lena_.save(&buffer, "GIF");
            buffer.close();
        }
    }

    void benchmarkConvertTo8bitLena()
    {
        QBENCHMARK
        {
            QImage image = lena_.convertToFormat(QImage::Format_Indexed8, Qt::NoOpaqueDetection | Qt::OrderedDither | Qt::ColorOnly);
            //qDebug("colors used: %d", image.colorCount());
        }
    }

    void benchmarkConvertTo8bitWikiText()
    {
        QBENCHMARK
        {
            QImage image = wikitext_.convertToFormat(QImage::Format_Indexed8, Qt::NoOpaqueDetection | Qt::ThresholdDither | Qt::ColorOnly);

            QVector<QRgb> colors = image.colorTable();


            //qDebug("colors used: %d", image.colorCount());
        }
    }

    void benchmarkHasAlphaValuesLena()
    {
        QBENCHMARK
        {
            bool result = ImageUtils::hasAlphaValues(lena_);

            //qDebug("hasAlphaValues: %s", result ? "true" : "false");
        }
    }

    void benchmarkHasAlphaValuesWikiText()
    {
        QBENCHMARK
        {
            bool result = ImageUtils::hasAlphaValues(wikitext_);

            if (!result)
                QImage image = wikitext_.convertToFormat(QImage::Format_RGB888);

            //qDebug("hasAlphaValues: %s", result ? "true" : "false");
        }
    }

    void benchmarkSaveToJPEG90()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            lena_.save(&buffer, "JPEG", 90);
            buffer.close();
        }
    }

    void benchmarkSaveToJPEG94()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            lena_.save(&buffer, "JPEG", 94);
            buffer.close();
        }
    }

#ifdef USE_IMPROVED_JPEG
    void benchmarkSaveToJPEG90Optimized()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            writeJPEG(lena_, &buffer, 90, true, false);
            buffer.close();
        }
    }

    void benchmarkSaveToJPEG94Optimized()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            writeJPEG(lena_, &buffer, 94, true, false);
            buffer.close();
        }
    }

    void benchmarkSaveToJPEG90Progressive()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            writeJPEG(lena_, &buffer, 90, false, true);
            buffer.close();
        }
    }

    void benchmarkSaveToJPEG94Progressive()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            writeJPEG(lena_, &buffer, 94, false, true);
            buffer.close();
        }
    }
#endif

    void benchmarkSaveToWEBP90()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            lena_.save(&buffer, "WEBP", 90);
            buffer.close();
        }
    }

    void benchmarkSaveToWEBP94()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            lena_.save(&buffer, "WEBP", 94);
            buffer.close();
        }
    }

    void benchmarkSaveToTIFF()
    {
        QBENCHMARK
        {
            QBuffer buffer;
            buffer.open(QIODevice::ReadWrite);
            lena_.save(&buffer, "TIFF");
            buffer.close();
        }
    }
private:
    int testWidth_;

    QImage lena_;
    QImage wikitext_;

    QImage imageBefore_;
    QImage imageAfter_;

    QRect testRect1_;
    QRect testTemplateRect_;

    ImageComparer *imageComparer_;
};

QTEST_MAIN(ImageComparerTest)
#include "main.moc"
