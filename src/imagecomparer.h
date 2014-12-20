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
#include <QMutex>

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
QList<QRect> findUpdateRects(QImage *buffer1, QImage *buffer2, const QRect &searchArea);

bool contentMatches(QImage *buffer1, QImage *buffer2, const QPoint &point, const QRect &rect);

QRect findMovedRect(QImage *imageBefore, QImage *imageAfter, const QRect &searchArea, const QRect &templateRect);
UpdateOperationList optimizeVectorizedOperations(UpdateOperationType type, const VectorHashUpdateOperationList &moveOps);
UpdateOperationList optimizeUpdateOperations(const UpdateOperationList &ops);

class MoveAnalyzer;

class VIRIDITY_EXPORT ImageComparer
{
public:
    ImageComparer(QImage *imageBefore, QImage *imageAfter);
    virtual ~ImageComparer();

    UpdateOperationList findUpdateOperations(const QRect &searchArea);
    void swap();

    int tileSize() const { return tileWidth_; }

protected:
    friend struct MapProcessRect;
    bool processRect(const QRect &rect, UpdateOperation &op);

private:
    QImage *imageBefore_;
    QImage *imageAfter_;

    QMutex mutex_;
    QList<QPoint> lastSuccessfulMoveVectors_;
    MoveAnalyzer *moveAnalyzer_;
    int movedRectSearchMisses_;
    bool movedRectSearchEnabled_;
    int tileWidth_;

    QList<QRect> damagedAreas_;
};

#endif // IMAGECOMPARER_H
