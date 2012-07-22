#include "moveanalyzer.h"
#include "imagecomparer.h"

#define DEBUG
#include "debug.h"

AreaFingerPrint getAreaFingerPrint(QImage *image, const QRect &area)
{
    QRect rect = area.intersected(image->rect());

    AreaFingerPrint result;
    result.reserve(rect.height());

    QRgb *pBuf;

    for (int y = 0; y < rect.height(); ++y)
    {
        pBuf = (QRgb *)image->scanLine(rect.top() + y) + rect.left();

        quint32 hash = 0;

        for (int x = 0; x < rect.width(); ++x)
        {
            hash += (*pBuf);
            ++pBuf;
        }

        result.append(hash);
    }

    return result;
}

AreaFingerPrints getAreaFingerPrintsSlow(QImage *image, const QRect &area, int templateWidth)
{
    QRect rect = area.intersected(image->rect());
    int resultWidth = qMax(0, rect.width() - templateWidth + 1);

    AreaFingerPrints result;
    result.reserve(resultWidth);

    QSize size(templateWidth, rect.height());

    int rightLimit = rect.left() + resultWidth;
    for (int x = rect.left(); x < rightLimit; ++x)
    {
        AreaFingerPrint column = getAreaFingerPrint(image, QRect(QPoint(x, rect.top()), size));
        result.append(column);
    }

    return result;
}

#define roundDownToMultipleOf(x, s) ((x) & ~((s)-1))

AreaFingerPrints getAreaFingerPrintsFast(QImage *image, const QRect &area, int templateWidth)
{
    DGUARDMETHODPROFILED;

    QRect rect = area.intersected(image->rect());
    int resultWidth = qMax(0, rect.width() - templateWidth + 1);

    AreaFingerPrints result(resultWidth, AreaFingerPrint(rect.height(), 0));

    int startIndex;
    quint32 *pBuf;

#define OPTIMIZE_CONDITIONAL_OPT
#define OPTIMIZE_UNROLL_LOOPS
//#define OPTIMIZE_LARGER_INT
//#define OPTIMIZE_ARRAY_OFFSET

#ifndef OPTIMIZE_CONDITIONAL_OPT
    QVector<quint32> lineVector(rect.width(), 0);
#else
    QVector<quint32> lineVector(rect.width() + templateWidth, 0);
#endif
    quint32 *line = lineVector.data();
    //int lineSize = lineVector.size();

    AreaFingerPrint *resultRows = result.data();

    for (int y = 0; y < rect.height(); ++y)
    {
        pBuf = (quint32 *)image->scanLine(rect.top() + y) + rect.left();

#ifndef OPTIMIZE_CONDITIONAL_OPT
        for (int index = 0; index < lineVector.size(); ++index)
        {
            startIndex = qMax(0, index + 1 - templateWidth);

            for (int xw = startIndex; xw <= index; ++xw)
                line[xw] += (*pBuf);

            ++pBuf;
        }

        for (int index = 0; index < resultWidth; ++index)
            result[index][y] = line[index];
#else
#ifdef OPTIMIZE_UNROLL_LOOPS
        for (int index = templateWidth - 1; index < lineVector.size(); ++index)
        {
            quint32 pixelValue = (*pBuf);
            int xw = index + 1 - templateWidth;

#ifdef OPTIMIZE_LARGER_INT
            int lim = roundDownToMultipleOf(index, 16);
            quint64 pixelDoubleValue = (quint64)pixelValue << 32 | (quint64)pixelValue;
            for (; xw < lim; xw += 16)
            {
                quint64 *dline = (quint64 *)&line[xw];
                dline[0] += pixelDoubleValue;
                dline[1] += pixelDoubleValue;
                dline[2] += pixelDoubleValue;
                dline[3] += pixelDoubleValue;
                dline[4] += pixelDoubleValue;
                dline[5] += pixelDoubleValue;
                dline[6] += pixelDoubleValue;
                dline[7] += pixelDoubleValue;
            }
#else
            int lim = roundDownToMultipleOf(index, 16);
            for (; xw < lim; xw += 16)
            {
#ifdef OPTIMIZE_ARRAY_OFFSET
                quint32 *sline = &line[xw];
                sline[0] += pixelValue;
                sline[1] += pixelValue;
                sline[2] += pixelValue;
                sline[3] += pixelValue;
                sline[4] += pixelValue;
                sline[5] += pixelValue;
                sline[6] += pixelValue;
                sline[7] += pixelValue;
                sline[8] += pixelValue;
                sline[9] += pixelValue;
                sline[10] += pixelValue;
                sline[11] += pixelValue;
                sline[12] += pixelValue;
                sline[13] += pixelValue;
                sline[14] += pixelValue;
                sline[15] += pixelValue;
#else
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
#endif
            }
#endif

            for (; xw <= index; ++xw)
                line[xw] += pixelValue;

            ++pBuf;
        }
#else
        for (int index = templateWidth - 1; index < lineVector.size(); ++index)
        {
            for (int xw = index + 1 - templateWidth; xw <= index; ++xw)
                line[xw] += (*pBuf);

            ++pBuf;
        }
#endif

        for (int index = 0; index < resultWidth; ++index)
        {
            AreaFingerPrint &fp = resultRows[index];
            fp[y] = line[templateWidth - 1 + index];
        }
#endif

        lineVector.fill(0);
    }

    return result;
}

bool fingerPrintsEqual(const AreaFingerPrints &prints1, const AreaFingerPrints &prints2)
{
    if (prints1.size() != prints2.size())
        return false;

    for (int y = 0; y < prints1.size(); ++y)
    {
        if (prints1.at(y).size() != prints2.at(y).size())
            return false;

        if (memcmp(prints1[y].data(), prints2[y].data(), prints1[y].size() * sizeof(quint32)) != 0)
            return false;
    }

    return true;
}

int indexOfFingerPrintInFingerPrint(const AreaFingerPrint &haystack, const AreaFingerPrint &needle)
{
    int haystackSize = haystack.size();
    int needleSize = needle.size();

    if (needleSize > haystackSize)
        return -1;

    const quint32 *y = haystack.constData();
    const quint32 *x = needle.constData();

    quint32 j, k, l;

    if (x[0] == x[1])
    {
        k = 2;
        l = 1;
    }
    else
    {
        k = 1;
        l = 2;
    }

    j = 0;
    while (j <= haystackSize - needleSize)
    {
        if (x[1] != y[j + 1])
        {
            j += k;
        }
        else
        {
            if (!memcmp(x + 2, y + j + 2, (needleSize - 2) * sizeof(quint32)) && x[0] == y[j])
                return j;

            j += l;
        }
    }

    return -1;
}

bool findFingerPrintPosition(const AreaFingerPrints &prints, const AreaFingerPrint &print, QPoint &result)
{
    //DGUARDMETHODTIMED;

    for (int line = 0; line < prints.size(); ++line)
    {
        int index = indexOfFingerPrintInFingerPrint(prints[line], print);

        if (index > -1)
        {
            result = QPoint(line, index);
            return true;
        }
    }

    return false;
}

/* MoveAnalyzer */

MoveAnalyzer::MoveAnalyzer(QImage *imageBefore, QImage *imageAfter, const QRect &hashArea, int templateWidth) :
    imageBefore_(imageBefore),
    imageAfter_(imageAfter),
    hashArea_(hashArea),
    templateWidth_(templateWidth)
{
    hashArea_ = imageBefore_->rect();
    searchAreaFingerPrints_ = getAreaFingerPrintsFast(imageBefore_, hashArea_, templateWidth);
}

QRect MoveAnalyzer::findMovedRect(const QRect &searchArea, const QRect &templateRect)
{
    //DGUARDMETHODTIMED;
    //return findMovedRectNaive(searchArea, templateRect);

    AreaFingerPrint templateFingerPrint = getAreaFingerPrint(imageAfter_, templateRect);

    QPoint result;

    if (findFingerPrintPosition(searchAreaFingerPrints_, templateFingerPrint, result))
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
