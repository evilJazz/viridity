#include "moveanalyzer.h"
#include "imagecomparer.h"

//#define DIAGNOSTIC_OUTPUT
#include "KCL/debug.h"

#define roundDownToMultipleOf(x, s) ((x) & ~((s)-1))

#define USE_MULTITHREADING
//#define USE_GRAYSCALE_OPT

#define USE_NSN // Almost 7x faster than naive memcmp
//#define USE_NSN_MEMCMP

//#define USE_AREAFINGERPRINTS

//#define SHOW_MOVEANALYZER_DEBUGVIEW

#ifdef SHOW_MOVEANALYZER_DEBUGVIEW
#include "private/moveanalyzerdebugview.h"
#undef USE_MULTITHREADING
#endif

#include <QThread>

#ifdef USE_MULTITHREADING
#include <QtConcurrentFilter>
#endif

/* AreaFingerPrint */

AreaFingerPrint::AreaFingerPrint() :
    size_(0),
    data_(NULL)
{
}

AreaFingerPrint::AreaFingerPrint(QImage *image, const QRect &area) :
    size_(0),
    data_(NULL)
{
    DGUARDMETHODPROFILED;
    initFromImage(image, area);
}

AreaFingerPrint::AreaFingerPrint(int height) :
    size_(0),
    data_(NULL)
{
    initFromSize(height);
}

AreaFingerPrint::~AreaFingerPrint()
{
    clear();
}

void AreaFingerPrint::clear()
{
    if (data_)
    {
        delete [] data_;
        data_ = NULL;
    }

    size_ = 0;
}

void AreaFingerPrint::initFromSize(int height)
{
    if (height != size_)
    {
        clear();
        size_ = height;
        data_ = new AreaFingerPrintHash[height];
    }
}

void AreaFingerPrint::initFromImage(QImage *image, const QRect &area)
{
    QRect rect = area.intersected(image->rect());
    initFromSize(rect.height());
    internalUpdateFromImage(image, rect, 0);
}

void AreaFingerPrint::updateFromImage(QImage *image, const QRect &area, int startIndex)
{
    if (startIndex >= size_)
        return;

    QRect rect = area.intersected(image->rect());
    internalUpdateFromImage(image, rect, startIndex);
}

void AreaFingerPrint::internalUpdateFromImage(QImage *image, const QRect &rect, int startIndex)
{
    const quint32 *pBuf;

    int limY = qMin(rect.height(), size_ - startIndex);
    int width = rect.width();

    AreaFingerPrintHash hash;

    for (int y = 0; y < limY; ++y)
    {
        pBuf = (const quint32 *)image->constScanLine(rect.top() + y) + rect.left();

        for (int x = width; x > 0; --x)
        {
            hash.add(*pBuf);
            ++pBuf;
        }

        data_[startIndex + y] = hash;
        hash.reset();
    }
}

int AreaFingerPrint::indexOf(const AreaFingerPrint &needle, int startIndex, int endIndex)
{
    int haystackSize = size();
    int needleSize = needle.size();

    if (needleSize > haystackSize || needleSize == 0 || haystackSize == 0)
        return -1;

    if (startIndex > haystackSize)
        return -1;

    int maxIndex = haystackSize - needleSize;
    if (endIndex <= 0 || endIndex > maxIndex)
        endIndex = maxIndex;

    if (endIndex < startIndex)
        return -1;

    const AreaFingerPrintHash *haystackData = constData();
    const AreaFingerPrintHash *needleData = needle.constData();

    if (needleSize > 1)
    {
#ifndef USE_NSN
        int needleSizeBytes = needleSize * sizeof(AreaFingerPrintHash);

        int index = startIndex;
        for (; index <= endIndex; ++index)
            if (!memcmp(needleData, &haystackData[index], needleSizeBytes))
                return index;
#else
        //*
        // Based on the Not-So-Naive search algorithm:
        // http://www-igm.univ-mlv.fr/~lecroq/string/
        // Adapted to work on AreaFingerPrintHash

        quint32 index, k, l;
#ifdef USE_NSN_MEMCMP
        const size_t needleSizeBytes = (needleSize - 2) * sizeof(AreaFingerPrintHash);
#else
        const int needleSize2 = needleSize - 2;
#endif

        if (needleData[0] == needleData[1])
        {
            k = 2;
            l = 1;
        }
        else
        {
            k = 1;
            l = 2;
        }

        index = startIndex;
        while (index <= endIndex)
        {
            if (needleData[1] != haystackData[index + 1])
            {
                index += k;
            }
            else
            {
#ifdef USE_NSN_MEMCMP
                if (needleData[0] == haystackData[index] &&
                    !memcmp(needleData + 2, haystackData + index + 2, needleSizeBytes))
                    return index;
#else
                if (needleData[0] == haystackData[index])
                {
                    int i = 2;
                    while (i < needleSize2)
                    {
                        if (needleData[i] != haystackData[index + i])
                            break;

                        ++i;
                    }

                    if (i == needleSize2)
                        return index;
                }
#endif

                index += l;
            }
        }
#endif
    }
    else
    {
        for (int index = startIndex; index <= endIndex; ++index)
            if (haystackData[index] == needleData[0])
                return index;
    }

    return -1;
}


/* AreaFingerPrints */

AreaFingerPrints::AreaFingerPrints() :
    width_(0),
    height_(0),
    fingerPrints_(NULL)
{
}

AreaFingerPrints::~AreaFingerPrints()
{
    clear();
}

void AreaFingerPrints::clear()
{
    if (fingerPrints_)
    {
        for (int i = 0; i < width_; ++i)
        {
            delete fingerPrints_[i];
            fingerPrints_[i] = NULL;
        }

        delete [] fingerPrints_;
        fingerPrints_ = NULL;
    }

    width_ = 0;
    height_ = 0;
}

void AreaFingerPrints::initFromSize(int width, int height, int templateWidth)
{
    int newWidth = qMax(0, width - templateWidth + 1);

    if (newWidth != width_ || height != height_)
    {
        clear();
        height_ = height;
        width_ = newWidth;
        hashedArea_ = QRect(0, 0, width, height);
        fingerPrints_ = new AreaFingerPrint*[width_];

        for (int i = 0; i < width_; ++i)
            fingerPrints_[i] = new AreaFingerPrint(height_);
    }
}

void AreaFingerPrints::initFromImage(QImage *image, const QRect &area, int templateWidth)
{
    DGUARDMETHODPROFILED;

    QRect rect = area.intersected(image->rect());
    initFromSize(rect.width(), rect.height(), templateWidth);

    hashedArea_ = rect;
    updateFromImage(image, rect);
}

void AreaFingerPrints::updateFromImage(QImage *image, const QRect &area)
{
    QRect rect = hashedArea_.intersected(area.intersected(image->rect()));

    QSize size(templateWidth(), rect.height());

    int leftLimit = qMax(0, rect.left() - templateWidth() + 1);
    //int rightLimit = rect.left() + rect.width() - templateWidth() + 1;
    int rightLimit = qMin(image->width() - templateWidth() + 1, rect.left() + rect.width() - 1);

    int startRow = rect.top() - hashedArea_.top();

    int column = leftLimit - hashedArea_.left();

    for (int x = leftLimit; x < rightLimit; ++x)
    {
        fingerPrints_[column]->updateFromImage(image, QRect(QPoint(x, rect.top()), size), startRow);
        ++column;
    }
}

#ifdef USE_MULTITHREADING

struct AreaFingerPrintsThreadedUpdateFromImage
{
    AreaFingerPrintsThreadedUpdateFromImage(QImage *image, AreaFingerPrints *target) :
        image(image), target(target)
    {}

    QImage *image;
    AreaFingerPrints *target;

    bool operator()(const QRect &rect)
    {
        target->updateFromImage(image, rect);
        return true;
    }
};

#endif

void AreaFingerPrints::initFromImageThreaded(QImage *image, const QRect &area, int templateWidth)
{
    DGUARDMETHODPROFILED;

    QRect rect = area.intersected(image->rect());
    initFromSize(rect.width(), rect.height(), templateWidth);

    hashedArea_ = rect;
    updateFromImageThreaded(image, rect);
}

void AreaFingerPrints::updateFromImageThreaded(QImage *image, const QRect &area)
{
    QRect rect = hashedArea_.intersected(area.intersected(image->rect()));

    const int lineHeight = templateWidth() / QThread::idealThreadCount();
    int lineCount = rect.height() / lineHeight;

    if (lineCount == 0)
        updateFromImage(image, rect);
    else
    {
        QVector<QRect> lineRects;

        for (int i = 0; i < lineCount; ++i)
            lineRects.append(QRect(rect.x(), rect.y() + i * lineHeight, rect.width(), lineHeight));

        int partsTotalHeight = lineCount * lineHeight;
        int remainingHeight = rect.height() - partsTotalHeight;
        if (remainingHeight > 0)
            lineRects.append(QRect(rect.x(), rect.y() + partsTotalHeight, rect.width(), remainingHeight));

#ifdef USE_MULTITHREADING
        QtConcurrent::blockingFilter(lineRects, AreaFingerPrintsThreadedUpdateFromImage(image, this));
#else
        foreach (const QRect &partRect, lineRects)
            updateFromImage(image, partRect);
#endif
    }
}

bool AreaFingerPrints::findPosition(const AreaFingerPrint &needle, QPoint &result, AreaFingerPrintsPositionMatcher *matcher)
{
    DGUARDMETHODPROFILED;

    for (int line = 0; line < width_; ++line)
    {
        int index = fingerPrints_[line]->indexOf(needle);

        if (index > -1)
        {
            result = QPoint(line, index);
            if (!matcher || (matcher && matcher->positionMatches(result)))
                return true;
        }
    }

    return false;
}

bool AreaFingerPrints::findPosition(const AreaFingerPrint &needle, const QRect &searchArea, QPoint &result, AreaFingerPrintsPositionMatcher *matcher)
{
    DGUARDMETHODPROFILED;

    QRect rect = hashedArea_.intersected(searchArea);

    if (rect.height() < needle.size())
        return false;

    int tempWidth = templateWidth();
    rect.setWidth(rect.width() - tempWidth + 1);
    rect.moveTo(rect.topLeft() - hashedArea_.topLeft());

    int startIndex = rect.top();
    int endIndex = rect.bottom() + 1 - needle.size();

    int colEnd = rect.right() + 1;
    for (int column = rect.left(); column < colEnd; ++column)
    {
        int index = fingerPrints_[column]->indexOf(needle, startIndex, endIndex);

        if (index > -1)
        {
            result = QPoint(column, index);
            if (!matcher || (matcher && matcher->positionMatches(result)))
                return true;
        }
    }

    return false;
}

bool AreaFingerPrints::isEqual(const AreaFingerPrints &other)
{
    if (width() != other.width())
        return false;

    for (int y = 0; y < width(); ++y)
    {
        if (fingerPrints_[y]->size() != other.fingerPrints_[y]->size())
            return false;

        if (memcmp(fingerPrints_[y]->data(), other.fingerPrints_[y]->data(), fingerPrints_[y]->size() * sizeof(AreaFingerPrintHash)) != 0)
            return false;
    }

    return true;
}

#ifdef USE_GRAYSCALE_OPT
QImage convertToGrayscale(const QImage &image)
{
    QImage result(image.width(), image.height(), QImage::Format_Indexed8);

    QVector<QRgb> grayTable(256);

    for (int i = 0; i < 256; ++i)
        grayTable[i] = qRgb(i, i, i);

    result.setColorTable(grayTable);

    int y;
    int height = image.height();
    int x;
    int width = image.width();
    register const QRgb *input;
    register uchar *output;

    for (y = 0; y < height; ++y)
    {
        input = (QRgb *)image.constScanLine(y);
        output = result.scanLine(y);

        for (x = 0; x < width; ++x)
        {
            *output = qGray(*input);

            ++output;
            ++input;
        }
    }

    //result.save("/home/darkstar/Desktop/test.png", "PNG");

    return result;
}
#endif

/* MoveAnalyzer */

MoveAnalyzer::MoveAnalyzer(QImage *imageBefore, QImage *imageAfter, const QRect &hashArea, int templateWidth) :
    QObject(0),
    imageBefore_(imageBefore),
    imageAfter_(imageAfter),
    hashArea_(hashArea),
    templateWidth_(templateWidth),
    searchRadius_(100),
    debugView_(NULL),
    mutex_(QMutex::Recursive),
    searchMissesThreshold_(40),
    movedRectSearchMisses_(0),
    movedRectSearchEnabled_(true)
{
    //hashArea_ = imageBefore_->rect();
#ifdef USE_AREAFINGERPRINTS
    searchAreaFingerPrints_.initFromImage(imageBefore_, hashArea_, templateWidth);
#endif

#ifdef SHOW_MOVEANALYZER_DEBUGVIEW
    debugView_ = new MoveAnalyzerDebugView();
    debugView_->setMoveAnalyzer(this);
    debugView_->show();
#endif
}

MoveAnalyzer::~MoveAnalyzer()
{
#ifdef SHOW_MOVEANALYZER_DEBUGVIEW
    if (debugView_)
        debugView_->deleteLater();
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

    while (lastSuccessfulMoveVectors_.count() > 100)
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
    if (imageAfterGray_.isNull())
        imageAfterGray_ = convertToGrayscale(*imageAfter_);
#endif
}

void MoveAnalyzer::updateArea(const QRect &rect)
{
#ifdef USE_AREAFINGERPRINTS
    searchAreaFingerPrints_.updateFromImage(imageBefore_, rect);
    //searchAreaFingerPrints_.updateFromImageSlow(imageBefore_, imageBefore_->rect());
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
                movedRectSearchArea.translate(-moveVector.vector);
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
            currentMoveVector.vector = rect.topLeft() - movedSrcRect.topLeft();
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

    QList<QRect> tiles = splitRectIntoTiles(searchArea, templateWidth_, templateWidth_);

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

struct MoveAnalyzerAreaFingerPrintsPositionMatcher : public AreaFingerPrintsPositionMatcher
{
    MoveAnalyzer *moveAnalyzer;
    QRect templateRect;

    virtual bool positionMatches(const QPoint &pos)
    {
        return contentMatches<QRgb>(moveAnalyzer->imageBefore_, moveAnalyzer->imageAfter_, pos, templateRect);
    }
};

QRect MoveAnalyzer::findMovedRect(const QRect &searchArea, const QRect &templateRect)
{
    //DGUARDMETHODTIMED;
    //return findMovedRectExhaustive(searchArea, templateRect);

#ifdef USE_AREAFINGERPRINTS
    if (templateRect.width() % searchAreaFingerPrints_.templateWidth() == 0)
		return findMovedRectAreaFingerPrint(searchArea, templateRect);        
    else
#endif
        return findMovedRectExhaustive(searchArea, templateRect);

}

QRect MoveAnalyzer::findMovedRectAreaFingerPrint(const QRect &searchArea, const QRect &templateRect)
{
    AreaFingerPrint templateFingerPrint(imageAfter_, templateRect);

    QPoint result;
    MoveAnalyzerAreaFingerPrintsPositionMatcher matcher;
    matcher.moveAnalyzer = this;
    matcher.templateRect = templateRect;

    if (searchAreaFingerPrints_.findPosition(templateFingerPrint, searchArea, result, &matcher))
        return QRect(hashArea_.topLeft() + result, templateRect.size());
    else
        return QRect();
}

/*
QRect MoveAnalyzer::findMovedRectExhaustive(const QRect &searchArea, const QRect &templateRect)
{
    //DGUARDMETHODTIMED;
    QRect roi = searchArea.intersected(imageBefore_->rect()).intersected(imageAfter_->rect());

    int templateWidth = templateRect.width();
    int templateHeight = templateRect.height();

    int roiBottom = roi.top() + roi.height() - templateHeight + 1;
    int roiRight = roi.left() + roi.width() - templateWidth + 1;

    for (int y = roi.top(); y < roiBottom; ++y)
    {
        for (int x = roi.left(); x < roiRight; ++x)
        {
            if (
#ifdef USE_GRAYSCALE_OPT
                contentMatches<uchar>(&imageBeforeGray_, &imageAfterGray_, x, y, templateRect.x(), templateRect.y(), templateWidth, templateHeight) &&
#endif
                contentMatches<QRgb>(imageBefore_, imageAfter_, x, y, templateRect.x(), templateRect.y(), templateWidth, templateHeight))
                return QRect(x, y, templateWidth, templateHeight);
        }
    }

    return QRect();
}
//*/

//*
QRect MoveAnalyzer::findMovedRectExhaustive(const QRect &searchArea, const QRect &templateRect)
{
    //DGUARDMETHODTIMED;
    QRect roi = searchArea.intersected(imageBefore_->rect()).intersected(imageAfter_->rect());

    int templateWidth = templateRect.width();
    int templateHeight = templateRect.height();

    int roiBottom = roi.height() - templateHeight + 1;
    int roiRight = roi.width() - templateWidth + 1;

    const int bytes = templateWidth * sizeof(QRgb);
    const int stride = imageBefore_->bytesPerLine() / sizeof(QRgb);

    register const QRgb *pBuf1, *pBufStart1, *pBufStart2;
    pBufStart1 = (QRgb *)imageBefore_->constScanLine(roi.top()) + roi.left();
    pBufStart2 = (QRgb *)imageAfter_->constScanLine(templateRect.y()) + templateRect.x();

    for (int y = 0; y < roiBottom; ++y)
    {
        pBuf1 = pBufStart1 + y * stride;

        for (int x = 0; x < roiRight; ++x)
        {
            if (*pBuf1 == *pBufStart2 &&
                *(pBuf1 + 1) == *(pBufStart2 + 1) &&
                *(pBuf1 + 2) == *(pBufStart2 + 2) &&
                *(pBuf1 + 3) == *(pBufStart2 + 3) &&
                contentMatches<QRgb>(pBuf1, pBufStart2, stride, bytes, templateHeight))
                return QRect(roi.left() + x, roi.top() + y, templateWidth, templateHeight);

            ++pBuf1;
        }
    }

    return QRect();
}
//*/
