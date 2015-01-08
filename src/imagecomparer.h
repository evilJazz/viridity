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
    UpdateOperationType type;
    QRect srcRect;
    QPoint dstPoint;
    QColor fillColor;
};

typedef QList<UpdateOperation> UpdateOperationList;

typedef QHash<QPoint, UpdateOperationList> VectorHashUpdateOperationList;
typedef QHashIterator<QPoint, UpdateOperationList> VectorHashUpdateOperationListIterator;

typedef QHash<QColor, UpdateOperationList> ColorHashUpdateOperationList;
typedef QHashIterator<QColor, UpdateOperationList> ColorHashUpdateOperationListIterator;

QList<QRect> splitRectIntoTiles(const QRect &rect, int tileWidth, int tileHeight);

template<typename T> inline bool contentMatches(QImage *buffer1, QImage *buffer2, const QPoint &point, const QRect &rect)
{
    QRect rect1 = QRect(point, rect.size()).intersected(buffer1->rect());
    QRect rect2 = rect.intersected(buffer2->rect());

    if (rect1.size() != rect.size() || rect2.size() != rect.size())
        return false;

    int y;
    int height = rect1.height();
    int x;
    int width = rect1.width();
    register T *pBuf1, *pBuf2;

    //pBuf1 = (T *)buffer1->scanLine(rect1.top()) + rect1.left();
    //pBuf2 = (T *)buffer2->scanLine(rect2.top()) + rect2.left();

    //register int stride1 = buffer1->bytesPerLine() / sizeof(T);
    //register int stride2 = buffer2->bytesPerLine() / sizeof(T);

    for (y = 0; y < height; ++y)
    {
        pBuf1 = (T *)buffer1->scanLine(rect1.top() + y) + rect1.left();
        pBuf2 = (T *)buffer2->scanLine(rect2.top() + y) + rect2.left();

        /*
        if (memcmp(pBuf1, pBuf2, width * sizeof(T)) != 0)
            return false;
        //*/

        //*
        for (x = 0; x < width; ++x)
        {
            if (*pBuf1 != *pBuf2)
                return false;

            ++pBuf1;
            ++pBuf2;
        }
        //*/

        //pBuf1 += stride1;
        //pBuf2 += stride1;
    }

    return true;
}

UpdateOperationList optimizeVectorizedOperations(UpdateOperationType type, const VectorHashUpdateOperationList &moveOps);
UpdateOperationList optimizeUpdateOperations(const UpdateOperationList &ops);

class MoveAnalyzer;

class VIRIDITY_EXPORT ImageComparer
{
public:
    ImageComparer(QImage *imageBefore, QImage *imageAfter);
    virtual ~ImageComparer();

    QVector<QRect> findDifferences();

    UpdateOperationList findUpdateOperations(const QRect &searchArea, QVector<QRect> *additionalSearchAreas = NULL);
    void swap();

    int tileSize() const { return tileWidth_; }

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
