#include "moveanalyzer.h"
#include "imagecomparer.h"

#include <QtConcurrentFilter>

#define DEBUG
#include "private/debug.h"

#define roundDownToMultipleOf(x, s) ((x) & ~((s)-1))
#define OPTIMIZE_UNROLL_LOOPS
#define OPTIMIZE_CONDITIONAL_OPT

//#define USE_MULTITHREADING

#define SHOW_MOVEANALYZER_DEBUGVIEW

#ifdef SHOW_MOVEANALYZER_DEBUGVIEW
#include "private/moveanalyzerdebugview.h"
#undef USE_MULTITHREADING
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
        delete data_;

    data_ = 0;
    size_ = 0;
}

void AreaFingerPrint::initFromSize(int height)
{
    if (height != size_)
    {
        clear();
        size_ = height;
        data_ = new quint32[height];
    }
}

void AreaFingerPrint::initFromImage(QImage *image, const QRect &area)
{
    QRect rect = area.intersected(image->rect());
    initFromSize(rect.height());

    quint32 *pBuf;
#ifdef OPTIMIZE_UNROLL_LOOPS
    int lim = roundDownToMultipleOf(rect.width(), 16);
#endif

    for (int y = 0; y < rect.height(); ++y)
    {
        pBuf = (quint32 *)image->scanLine(rect.top() + y) + rect.left();

        quint32 hash = 0;
        int x = 0;

#ifdef OPTIMIZE_UNROLL_LOOPS
        for (; x < lim; x += 16)
        {
            hash += pBuf[x + 0];
            hash += pBuf[x + 1];
            hash += pBuf[x + 2];
            hash += pBuf[x + 3];
            hash += pBuf[x + 4];
            hash += pBuf[x + 5];
            hash += pBuf[x + 6];
            hash += pBuf[x + 7];
            hash += pBuf[x + 8];
            hash += pBuf[x + 9];
            hash += pBuf[x + 10];
            hash += pBuf[x + 11];
            hash += pBuf[x + 12];
            hash += pBuf[x + 13];
            hash += pBuf[x + 14];
            hash += pBuf[x + 15];
        }

        pBuf += x;
#endif
        for (; x < rect.width(); ++x)
        {
            hash += (*pBuf);
            ++pBuf;
        }

        data_[y] = hash;
    }
}

void AreaFingerPrint::updateFromImage(QImage *image, const QRect &area, int startIndex)
{
    if (startIndex >= size_)
        return;

    QRect rect = area.intersected(image->rect());

    //int endIndex = qMin(size() - 1, startIndex + rect.height() - 1);

    quint32 *pBuf;
#ifdef OPTIMIZE_UNROLL_LOOPS
    int limX = roundDownToMultipleOf(rect.width(), 16);
#endif

    int limY = qMin(rect.height(), size_ - startIndex);

    for (int y = 0; y < limY; ++y)
    {
        pBuf = (quint32 *)image->scanLine(rect.top() + y) + rect.left();

        quint32 hash = 0;
        int x = 0;

#ifdef OPTIMIZE_UNROLL_LOOPS
        for (; x < limX; x += 16)
        {
            hash += pBuf[x + 0];
            hash += pBuf[x + 1];
            hash += pBuf[x + 2];
            hash += pBuf[x + 3];
            hash += pBuf[x + 4];
            hash += pBuf[x + 5];
            hash += pBuf[x + 6];
            hash += pBuf[x + 7];
            hash += pBuf[x + 8];
            hash += pBuf[x + 9];
            hash += pBuf[x + 10];
            hash += pBuf[x + 11];
            hash += pBuf[x + 12];
            hash += pBuf[x + 13];
            hash += pBuf[x + 14];
            hash += pBuf[x + 15];
        }

        pBuf += x;
#endif
        for (; x < rect.width(); ++x)
        {
            hash += (*pBuf);
            ++pBuf;
        }

        data_[startIndex + y] = hash;
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

    const quint32 *haystackData = constData();
    const quint32 *needleData = needle.constData();

    if (needleSize > 1)
    {
        int needleSizeBytes = needleSize * sizeof(quint32);

        int index = startIndex;
        for (; index <= endIndex; ++index)
            if (!memcmp(needleData, &haystackData[index], needleSizeBytes))
                return index;

        /*
        // Based on the Not-So-Naive search algorithm:
        // http://www-igm.univ-mlv.fr/~lecroq/string/
        // Adapted to work on quint32

        quint32 index, k, l;

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
                if (!memcmp(needleData + 2, haystackData + index + 2, (needleSize - 2) * sizeof(quint32)) &&
                    needleData[0] == haystackData[index])
                    return index;

                index += l;
            }
        }
        */
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
            delete fingerPrints_[i];

        delete fingerPrints_;
        fingerPrints_ = 0;
    }

    width_ = 0;
    height_ = 0;
}

void AreaFingerPrints::initFromSize(int width, int height, int templateWidth)
{
    int newWidth = qMax(0, width - templateWidth + 1);

    if (newWidth != width_ && height != height_)
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

void AreaFingerPrints::initFromImageSlow(QImage *image, const QRect &area, int templateWidth)
{
    DGUARDMETHODPROFILED;

    QRect rect = area.intersected(image->rect());
    initFromSize(rect.width(), rect.height(), templateWidth);

    hashedArea_ = rect;

    updateFromImageSlow(image, rect);
}

void AreaFingerPrints::updateFromImageSlow(QImage *image, const QRect &area)
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

void AreaFingerPrints::initFromImageFast(QImage *image, const QRect &area, int templateWidth)
{
    DGUARDMETHODPROFILED;

    QRect rect = area.intersected(image->rect());
    initFromSize(rect.width(), rect.height(), templateWidth);

    hashedArea_ = rect;
    updateFromImageFast(image, rect);
}

void AreaFingerPrints::updateFromImageFast(QImage *image, const QRect &area)
{
    QRect rect = hashedArea_.intersected(area.intersected(image->rect()));

    quint32 *pBuf;
    const int templateWidthM1 = templateWidth() - 1;

#ifdef OPTIMIZE_CONDITIONAL_OPT
    QVector<quint32> lineVector(rect.width() + templateWidth(), 0);
#else
    QVector<quint32> lineVector(rect.width(), 0);
#endif
    quint32 *line = lineVector.data();

    for (int y = rect.top(); y < rect.bottom() + 1; ++y)
    {
        pBuf = (quint32 *)image->scanLine(y) + rect.left();

#ifdef OPTIMIZE_CONDITIONAL_OPT
        for (int index = templateWidthM1; index < lineVector.size(); ++index)
        {
            int xw = index - templateWidthM1;
#else
        for (int index = 0; index < lineVector.size(); ++index)
        {
            int xw = qMax(0, index - templateWidthM1);
#endif
            quint32 pixelValue = (*pBuf);

#ifdef OPTIMIZE_UNROLL_LOOPS
            int lim = xw + roundDownToMultipleOf(index - xw, 16);
            for (; xw < lim; xw += 16)
            {
                line[xw] += pixelValue;
                line[xw + 1] += pixelValue;
                line[xw + 2] += pixelValue;
                line[xw + 3] += pixelValue;
                line[xw + 4] += pixelValue;
                line[xw + 5] += pixelValue;
                line[xw + 6] += pixelValue;
                line[xw + 7] += pixelValue;
                line[xw + 8] += pixelValue;
                line[xw + 9] += pixelValue;
                line[xw + 10] += pixelValue;
                line[xw + 11] += pixelValue;
                line[xw + 12] += pixelValue;
                line[xw + 13] += pixelValue;
                line[xw + 14] += pixelValue;
                line[xw + 15] += pixelValue;
            }
#endif

            for (; xw <= index; ++xw)
                line[xw] += pixelValue;

            ++pBuf;
        }

#ifdef OPTIMIZE_CONDITIONAL_OPT
        for (int index = 0; index < width_; ++index)
            fingerPrints_[index]->data()[y - hashedArea().top()] = line[templateWidthM1 + index];
#else
        for (int index = 0; index < width_; ++index)
            fingerPrints_[index]->data()[y - hashedArea().top()] = line[index];
#endif

        lineVector.fill(0);
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
        target->updateFromImageFast(image, rect);
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

    const int partHeight = 20;
    int parts = rect.height() / partHeight;

    if (parts == 0)
        updateFromImageFast(image, rect);
    else
    {
        QVector<QRect> rects;

        for (int i = 0; i < parts; ++i)
            rects.append(QRect(rect.x(), rect.y() + i * partHeight, rect.width(), partHeight));

        int partsTotalHeight = parts * partHeight;
        int remainingHeight = rect.height() - partsTotalHeight;
        if (remainingHeight > 0)
            rects.append(QRect(rect.x(), rect.y() + partsTotalHeight, rect.width(), remainingHeight));

#ifdef USE_MULTITHREADING
        QtConcurrent::blockingFilter(rects, AreaFingerPrintsThreadedUpdateFromImage(image, this));
#else
        foreach (const QRect &partRect, rects)
            updateFromImageFast(image, partRect);
#endif
    }
}

bool AreaFingerPrints::findPosition(const AreaFingerPrint &needle, QPoint &result)
{
    DGUARDMETHODPROFILED;

    for (int line = 0; line < width_; ++line)
    {
        int index = fingerPrints_[line]->indexOf(needle);

        if (index > -1)
        {
            result = QPoint(line, index);
            return true;
        }
    }

    return false;
}

bool AreaFingerPrints::findPosition(const AreaFingerPrint &needle, const QRect &searchArea, QPoint &result)
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

    //for (int line = 0; line < width_; ++line)
    for (int column = rect.left(); column < rect.right() + 1; ++column)
    {
        int index = fingerPrints_[column]->indexOf(needle, startIndex, endIndex);

        if (index > -1)
        {
            result = QPoint(column, index);
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

        if (memcmp(fingerPrints_[y]->data(), other.fingerPrints_[y]->data(), fingerPrints_[y]->size() * sizeof(quint32)) != 0)
            return false;
    }

    return true;
}


/* MoveAnalyzer */

MoveAnalyzer::MoveAnalyzer(QImage *imageBefore, QImage *imageAfter, const QRect &hashArea, int templateWidth) :
    QObject(0),
    imageBefore_(imageBefore),
    imageAfter_(imageAfter),
    hashArea_(hashArea),
    templateWidth_(templateWidth),
    debugView_(NULL)
{
    //hashArea_ = imageBefore_->rect();
    searchAreaFingerPrints_.initFromImageSlow(imageBefore_, hashArea_, templateWidth);

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

void MoveAnalyzer::swap()
{
    QImage *temp = imageBefore_;
    imageBefore_ = imageAfter_;
    imageAfter_ = temp;

    emit changed();
}

void MoveAnalyzer::updateArea(const QRect &rect)
{
    searchAreaFingerPrints_.updateFromImageSlow(imageBefore_, rect);
    //searchAreaFingerPrints_.updateFromImageSlow(imageBefore_, imageBefore_->rect());
    emit changed();
}

QRect MoveAnalyzer::findMovedRect(const QRect &searchArea, const QRect &templateRect)
{
    //DGUARDMETHODTIMED;
    //return findMovedRectNaive(searchArea, templateRect);

    AreaFingerPrint templateFingerPrint(imageAfter_, templateRect);

    QPoint result;

    if (searchAreaFingerPrints_.findPosition(templateFingerPrint, searchArea, result))
        return QRect(hashArea_.topLeft() + result, templateRect.size());
    else
        return QRect();
}

QRect MoveAnalyzer::findMovedRectNaive(const QRect &searchArea, const QRect &templateRect)
{
    //DGUARDMETHODTIMED;
    QRect roi = searchArea.intersected(imageBefore_->rect()).intersected(imageAfter_->rect());

    int roiBottom = roi.top() + roi.height() - templateRect.height() + 1;
    int roiRight = roi.left() + roi.width() - templateRect.width() + 1;

    QPoint srcPoint;

    for (int y = roi.top(); y < roiBottom; ++y)
    {
        srcPoint.setY(y);

        for (int x = roi.left(); x < roiRight; ++x)
        {
            srcPoint.setX(x);

            if (contentMatches(imageBefore_, imageAfter_, srcPoint, templateRect))
                return QRect(srcPoint, templateRect.size());
        }
    }

    return QRect();
}