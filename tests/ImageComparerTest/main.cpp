#include <QtCore/QCoreApplication>
#include <qtest.h>
#include <QtDebug>

#include <QPainter>

#include "imagecomparer.h"
#include "moveanalyzer.h"

class ImageComparerTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        testWidth_ = 64;

        lena_.load(":/testimages/lena.png");
        lena_ = lena_.convertToFormat(QImage::Format_ARGB32);

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

private:
    int testWidth_;

    QImage lena_;

    QImage imageBefore_;
    QImage imageAfter_;

    QRect testRect1_;
    QRect testTemplateRect_;

    ImageComparer *imageComparer_;
};

QTEST_MAIN(ImageComparerTest)
#include "main.moc"
