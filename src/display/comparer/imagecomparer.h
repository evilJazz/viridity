#ifndef IMAGECOMPARER_H
#define IMAGECOMPARER_H

#include "viridity_global.h"

#include <QRegion>
#include <QVector>
#include <QList>
#include <QHash>
#include <QHashIterator>
#include <QRect>
#include <QColor>
#include <QImage>

enum UpdateOperationType { uotUpdate, uotMove, uotFill, uotNoOp };

struct UpdateOperation
{
    UpdateOperation() : type(uotNoOp), data(NULL) {}
    UpdateOperationType type;
    QRect srcRect;
    QPoint dstPoint;
    QColor fillColor;
    void *data;
};

typedef QList<UpdateOperation> UpdateOperationList;

QList<QRect> splitRectIntoTiles(const QRect &rect, int tileWidth, int tileHeight);


template<typename T> inline bool contentMatches(const T *pBuf1, const T *pBuf2, const int stride, const int bytes, const int height)
{
    for (int y = 0; y < height; ++y)
    {
        if (*pBuf1 != *pBuf2 ||
            memcmp(pBuf1, pBuf2, bytes) != 0)
            return false;

        pBuf1 += stride;
        pBuf2 += stride;
    }

    return true;
}

template<typename T> inline bool contentMatches(QImage *buffer1, QImage *buffer2, const int x1, const int y1, const int x2, const int y2, const int width, const int height)
{
    const int bytes = width * sizeof(T);
    register const T *pBuf1, *pBuf2;

    pBuf1 = (T *)buffer1->constScanLine(y1) + x1;
    pBuf2 = (T *)buffer2->constScanLine(y2) + x2;

    register const int stride = buffer1->bytesPerLine() / sizeof(T);

    return contentMatches<T>(pBuf1, pBuf2, stride, bytes, height);
}

template<typename T> inline bool contentMatches(QImage *buffer1, QImage *buffer2, const QPoint &pointBuffer1, const QRect &rectBuffer2)
{
    QRect rect1 = QRect(pointBuffer1, rectBuffer2.size()).intersected(buffer1->rect());
    QRect rect2 = rectBuffer2.intersected(buffer2->rect());

    if (rect1.width() != rect2.width() ||
        rect1.height() != rect2.height())
        return false;

    return contentMatches<T>(buffer1, buffer2, rect1.left(), rect1.top(), rect2.left(), rect2.top(), rect1.width(), rect1.height());
}

class MoveAnalyzer;

class VIRIDITY_EXPORT ImageComparer
{
public:
    ImageComparer(QImage *imageBefore, QImage *imageAfter, int tileWidth = 32);
    virtual ~ImageComparer();

    int tileSize() const { return tileWidth_; }

    QVector<QRect> findDifferences();

    UpdateOperationList findUpdateOperations(const QRect &searchArea, QVector<QRect> *additionalSearchAreas = NULL);
    void swap();

    static UpdateOperationList optimizeUpdateOperations(const UpdateOperationList &ops);

protected:
    friend struct MapProcessRect;
    bool processRect(const QRect &rect, UpdateOperation &op, QVector<QRect> *additionalSearchAreas = NULL);

private:
    friend class ImageComparerTest;
    QImage *imageBefore_;
    QImage *imageAfter_;

    MoveAnalyzer *moveAnalyzer_;
    int tileWidth_;
};

#endif // IMAGECOMPARER_H
