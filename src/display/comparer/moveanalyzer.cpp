#include "moveanalyzer.h"

#include "imageaux.h"
#include "imagecomparer.h"
#include "tiles.h"

#include "KCL/imageutils.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

//#define USE_GRAYSCALE_OPT

#ifdef USE_AREAFINGERPRINTS
#include "areafingerprint.h"
#endif

#include <QThread>

/* MoveAnalyzer */

MoveAnalyzer::MoveAnalyzer(QImage *imageBefore, QImage *imageAfter, int templateWidth) :
    QObject(0),
    imageBefore_(imageBefore),
    imageAfter_(imageAfter),
    templateWidth_(templateWidth),
    searchRadius_(200),
    mutex_(QMutex::Recursive),
    searchMissesThreshold_(50),
    movedRectSearchMisses_(0),
    movedRectSearchEnabled_(true)
{
#ifdef USE_AREAFINGERPRINTS
    searchAreaFingerPrints_ = new AreaFingerPrints();
    searchAreaFingerPrints_->initFromImage(imageBefore_, imageBefore_->rect(), templateWidth);
#endif
}

MoveAnalyzer::~MoveAnalyzer()
{
#ifdef USE_AREAFINGERPRINTS
    delete searchAreaFingerPrints_;
    searchAreaFingerPrints_ = NULL;
#endif
}

void MoveAnalyzer::setSearchRadius(int newRadius)
{
    searchRadius_ = newRadius;
}

void MoveAnalyzer::setSearchMissesThreshold(int newThreshold)
{
    searchMissesThreshold_ = newThreshold;
}

void MoveAnalyzer::swap()
{
    QRegion region;
    foreach (const QRect &rect, damagedAreas_)
        region += rect;

    damagedAreas_.clear();

    while (lastSuccessfulMoveVectors_.count() > 200)
        lastSuccessfulMoveVectors_.removeLast();

    QImage *temp = imageBefore_;
    imageBefore_ = imageAfter_;
    imageAfter_ = temp;

#ifdef USE_GRAYSCALE_OPT
    ensureImagesUpdated();
    imageBeforeGray_ = imageAfterGray_;
    imageAfterGray_ = QImage();
#endif

    const QVector<QRect> rects = region.rects();
    foreach (const QRect &rect, rects)
        updateArea(rect);

    emit changed();
}

void MoveAnalyzer::ensureImagesUpdated()
{
#ifdef USE_GRAYSCALE_OPT
    QMutexLocker l(&mutex_);

    if (imageAfterGray_.isNull())
        ImageUtils::convertToGrayscale(*imageAfter_, imageAfterGray_);
#endif
}

void MoveAnalyzer::updateArea(const QRect &rect)
{
#ifdef USE_AREAFINGERPRINTS
    searchAreaFingerPrints_->updateFromImage(imageBefore_, rect);
#endif

    emit changed();
}

void MoveAnalyzer::startNewSearch()
{
    movedRectSearchMisses_ = 0;
    movedRectSearchEnabled_ = true;
}

QRect MoveAnalyzer::processRect(const QRect &rect, QVector<QRect> *additionalSearchAreas)
{
    ensureImagesUpdated();

    QMutexLocker l(&mutex_);
    damagedAreas_.append(rect); // used for updating MoveAnalyzer instance in swap()
    l.unlock();

    QRect movedSrcRect;

    if (movedRectSearchEnabled_)
    {
        QRect movedRectSearchArea;

        if (lastSuccessfulMoveVectors_.count() > 0)
        {
            QMutexLocker l(&mutex_);
            VectorEstimates list = lastSuccessfulMoveVectors_;
            list.detach();
            l.unlock();

            foreach (const VectorEstimate &moveVector, list)
            {
                movedRectSearchArea = rect;
                movedRectSearchArea.translate(-moveVector.vector.toPoint());
                if (moveVector.extentX > 0 || moveVector.extentY > 0)
                    movedRectSearchArea.adjust(-moveVector.extentX, -moveVector.extentY, moveVector.extentX, moveVector.extentY);

                movedSrcRect = findMovedRect(movedRectSearchArea, rect);

                if (!movedSrcRect.isEmpty())
                {
                    DPRINTF("Found move area with existing vector %d, %d", moveVector.vector.x(), moveVector.vector.y());
                    break;
                }
            }
        }

        if (movedSrcRect.isNull())
        {
            QVector<QRect> searchAreas;

            if (additionalSearchAreas && additionalSearchAreas->count() > 0)
                searchAreas = *additionalSearchAreas;

            searchAreas += rect.adjusted(-searchRadius_, -searchRadius_, searchRadius_, searchRadius_);

            foreach (const QRect &searchArea, searchAreas)
            {
                movedRectSearchArea = searchArea;
                movedSrcRect = findMovedRect(movedRectSearchArea, rect);

                if (!movedSrcRect.isEmpty())
                {
                    DPRINTF("Found move area with additional search area %d, %d %d x %d", searchArea.x(), searchArea.y(), searchArea.width(), searchArea.height());
                    break;
                }
                else
                {
                    QMutexLocker l(&mutex_);
                    ++movedRectSearchMisses_;

                    if (movedRectSearchMisses_ == searchMissesThreshold_)
                    {
                        movedRectSearchEnabled_ = false;
                        break;
                    }
                }
            }
        }

        if (!movedSrcRect.isEmpty())
        {
            VectorEstimate currentMoveVector;
            currentMoveVector.vector = QVector2D(rect.topLeft() - movedSrcRect.topLeft());
            currentMoveVector.extentX = 0;
            currentMoveVector.extentY = 0;

            QMutexLocker l(&mutex_);

            int index = lastSuccessfulMoveVectors_.indexOf(currentMoveVector);

            if (index == -1)
                lastSuccessfulMoveVectors_.prepend(currentMoveVector);
            else
                lastSuccessfulMoveVectors_.move(index, 0);

            l.unlock();
        }
    }

    return movedSrcRect;
}

MoveOperationList MoveAnalyzer::findMoveOperations(const QRect &searchArea, QRegion &leftovers, QVector<QRect> *additionalSearchAreas)
{
    MoveOperationList result;

    startNewSearch();

    QList<QRect> tiles = TileOperations::splitRectIntoTiles(searchArea, templateWidth_, templateWidth_);

    foreach (const QRect &rect, tiles)
    {
        QRect movedSrcRect = processRect(rect, additionalSearchAreas);

        if (movedSrcRect.isEmpty())
            leftovers += rect;
        else
        {
            MoveOperation op;
            op.srcRect = movedSrcRect;
            op.dstPoint = rect.topLeft();
            result += op;
        }
    }

    return result;
}

#ifdef USE_AREAFINGERPRINTS
struct MoveAnalyzerAreaFingerPrintsPositionMatcher : public AreaFingerPrintsPositionMatcher
{
    MoveAnalyzer *moveAnalyzer;
    QRect templateRect;

    virtual bool positionMatches(const QPoint &pos)
    {
        return ImageAux::contentMatches<QRgb>(moveAnalyzer->imageBefore_, moveAnalyzer->imageAfter_, pos, templateRect);
    }
};
#endif

QRect MoveAnalyzer::findMovedRect(const QRect &searchArea, const QRect &templateRect)
{
#ifdef USE_AREAFINGERPRINTS
    if (templateRect.width() % searchAreaFingerPrints_->templateWidth() == 0)
		return findMovedRectAreaFingerPrint(searchArea, templateRect);        
    else
#endif
        return findMovedRectExhaustive(searchArea, templateRect);
}

#ifdef USE_AREAFINGERPRINTS
QRect MoveAnalyzer::findMovedRectAreaFingerPrint(const QRect &searchArea, const QRect &templateRect)
{
    AreaFingerPrint templateFingerPrint(imageAfter_, templateRect);

    QPoint result;
    MoveAnalyzerAreaFingerPrintsPositionMatcher matcher;
    matcher.moveAnalyzer = this;
    matcher.templateRect = templateRect;

    if (searchAreaFingerPrints_->findPosition(templateFingerPrint, searchArea, result, &matcher))
        return QRect(result, templateRect.size());
    else
        return QRect();
}
#endif

QRect MoveAnalyzer::findMovedRectExhaustive(const QRect &searchArea, const QRect &templateRect)
{
    QRect roi = searchArea.intersected(imageBefore_->rect()).intersected(imageAfter_->rect());

    if (roi.isEmpty())
        return QRect();

    int templateWidth = templateRect.width();
    int templateHeight = templateRect.height();

    int roiBottom = roi.height() - templateHeight + 1;
    int roiRight = roi.width() - templateWidth + 1;

    if (roiBottom < 1 || roiRight < 1)
        return QRect();

#ifdef USE_GRAYSCALE_OPT
    const int bytes = templateWidth * sizeof(uchar);
    const int stride = imageBeforeGray_.bytesPerLine() / sizeof(uchar);

    register const uchar *pBuf1, *pBufStart1, *pBufStart2;
    pBufStart1 = (uchar *)imageBeforeGray_.constScanLine(roi.top()) + roi.left();
    pBufStart2 = (uchar *)imageAfterGray_.constScanLine(templateRect.y()) + templateRect.x();
#else
    const int bytes = templateWidth * sizeof(QRgb);
    const int stride = imageBefore_->bytesPerLine() / sizeof(QRgb);

    register const QRgb *pBuf1, *pBufStart1, *pBufStart2;
    pBufStart1 = (QRgb *)imageBefore_->constScanLine(roi.top()) + roi.left();
    pBufStart2 = (QRgb *)imageAfter_->constScanLine(templateRect.y()) + templateRect.x();
#endif

    if (templateWidth >= 4)
        for (int y = 0; y < roiBottom; ++y)
        {
            pBuf1 = pBufStart1 + y * stride;

            for (int x = 0; x < roiRight; ++x)
            {
#ifndef USE_GRAYSCALE_OPT
                if (*pBuf1 == *pBufStart2 &&
                    *(pBuf1 + 1) == *(pBufStart2 + 1) &&
                    *(pBuf1 + 2) == *(pBufStart2 + 2) &&
                    *(pBuf1 + 3) == *(pBufStart2 + 3) &&
                    ImageAux::contentMatches<QRgb>(pBuf1, pBufStart2, stride, bytes, templateHeight))
                {
#else
                if (ImageAux::contentMatches<uchar>(pBuf1, pBufStart2, stride, bytes, templateHeight))
                {
                    if (ImageAux::contentMatches<QRgb>(imageBefore_, imageAfter_, x, y, templateRect.x(), templateRect.y(), templateWidth, templateHeight))
#endif
                    return QRect(roi.left() + x, roi.top() + y, templateWidth, templateHeight);
                }

                ++pBuf1;
            }
        }
    else
        for (int y = 0; y < roiBottom; ++y)
        {
            pBuf1 = pBufStart1 + y * stride;

            for (int x = 0; x < roiRight; ++x)
            {
                if (*pBuf1 == *pBufStart2 &&
                    ImageAux::contentMatches<QRgb>(pBuf1, pBufStart2, stride, bytes, templateHeight))
                {
                    return QRect(roi.left() + x, roi.top() + y, templateWidth, templateHeight);
                }

                ++pBuf1;
            }
        }

    return QRect();
}
