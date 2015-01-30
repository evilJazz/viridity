#include "imagecomparer.h"
#include "moveanalyzer.h"
#include "tiledregion.h"

#include <QtConcurrentMap>

//#undef DEBUG
#include "KCL/debug.h"

//#define USE_MOVE_ANALYZER
#define USE_MOVE_ANALYZER_FINEGRAINED
#define USE_FILL_ANALYZER
//#define USE_MULTITHREADING

inline uint qHash(const QPoint& p)
{
    return qHash(QString().sprintf("%dx%d", p.x(), p.y()));
}

inline uint qHash(const QColor& c)
{
    return qHash((int)c.rgba());
}

/*
QRect findChangedRect(QImage *buffer1, QImage *buffer2, const QRect &searchArea)
{
    DGUARDMETHODTIMED;
    QRect roi = searchArea.intersected(buffer1->rect()).intersected(buffer2->rect());

    QRect result(roi.bottomRight(), roi.topLeft());

    for (int y = roi.top(); y < roi.bottom() + 2; ++y)
    {
        for (int x = roi.left(); x < roi.right() + 2; ++x)
        {
            if (buffer1->pixel(x, y) != buffer2->pixel(x, y))
            {
                result.setLeft(qMin(result.left(), x));
                result.setTop(qMin(result.top(), y));
                result.setRight(qMax(result.right(), x));
                result.setBottom(qMax(result.bottom(), y));
            }
        }
    }

    if (!result.isValid())
        return QRect();
    else
        return result;
}
*/

QRect fastFindChangedRect32(QImage *buffer1, QImage *buffer2, const QRect &searchArea)
{
    //DGUARDMETHODTIMED;
    QRect roi = searchArea.intersected(buffer1->rect()).intersected(buffer2->rect());

    QRect result(roi.bottomRight(), roi.topLeft());

    QRgb *pBuf1, *pBuf2;

    int roiBottom = roi.top() + roi.height();
    int roiRight = roi.left() + roi.width();

    for (int y = roi.top(); y < roiBottom; ++y)
    {
        pBuf1 = (QRgb *)buffer1->scanLine(y) + roi.left();
        pBuf2 = (QRgb *)buffer2->scanLine(y) + roi.left();

        for (int x = roi.left(); x < roiRight; ++x)
        {
            if (*pBuf1 != *pBuf2)
            {
                result.setLeft(qMin(result.left(), x));
                result.setTop(qMin(result.top(), y));
                result.setRight(qMax(result.right(), x));
                result.setBottom(qMax(result.bottom(), y));
            }

            ++pBuf1;
            ++pBuf2;
        }
    }

    if (!result.isValid())
        return QRect();
    else
        return result;
}

QColor getSolidRectColor(QImage *buffer, const QRect &area)
{
    QRect roi = area.intersected(buffer->rect());

    int roiBottom = roi.top() + roi.height();
    int roiRight = roi.left() + roi.width();

    QRgb solidColor = buffer->pixel(roi.topLeft());
    bool isSolidColor = true;
    int y = roi.top();

    QRgb *pBuf;

    while (isSolidColor && y < roiBottom)
    {
        pBuf = (QRgb *)buffer->scanLine(y) + roi.left();
        ++y;

        for (int x = roi.left(); x < roiRight; ++x)
        {
            if (*pBuf != solidColor)
            {
                isSolidColor = false;
                break;
            }

            ++pBuf;
        }
    }

    if (isSolidColor)
        return QColor::fromRgba(solidColor);
    else
        return QColor();
}

QList<QRect> splitRectIntoTiles(const QRect &rect, int tileWidth, int tileHeight)
{
    //DGUARDMETHODTIMED;

    QList<QRect> tiles;
    int leftX = rect.width() % tileWidth;
    int leftY = rect.height() % tileHeight;

    int y = rect.top();
    int x;

    int bottomLimit = rect.bottom() + 1 - tileHeight + 1;
    int rightLimit = rect.right() + 1 - tileWidth + 1;

    for (y = rect.top(); y < bottomLimit; y += tileHeight)
    {
        for (x = rect.left(); x < rightLimit; x += tileWidth)
            tiles += QRect(x, y, tileWidth, tileHeight);

        if (leftX > 0)
            tiles += QRect(x, y, leftX, tileHeight);
    }

    if (leftY > 0)
    {
        for (x = rect.left(); x < rightLimit; x += tileWidth)
            tiles += QRect(x, y, tileWidth, leftY);

        if (leftX > 0)
            tiles += QRect(x, y, leftX, leftY);
    }

    return tiles;
}

/*
QList<QRect> findUpdateRects(QImage *buffer1, QImage *buffer2, const QRect &searchArea)
{
    QList<QRect> tiles = splitRectIntoTiles(searchArea, 100, 100);

    QList<QRect> result;

    foreach (const QRect &rect, tiles)
    {
        QRect minRect = fastFindChangedRect32(buffer1, buffer2, rect);

        if (!minRect.isEmpty())
        {
            if (minRect != rect)
            {
                DPRINTF("Minimized rect:  %d, %d + %d x %d   ->   %d, %d + %d x %d",
                        rect.left(), rect.top(), rect.width(), rect.height(),
                        minRect.left(), minRect.top(), minRect.width(), minRect.height()
                        );
            }

            result += minRect;
        }
        else
        {
            DPRINTF("Removed rect:  %d, %d + %d x %d   ->   %d, %d + %d x %d",
                    rect.left(), rect.top(), rect.width(), rect.height(),
                    minRect.left(), minRect.top(), minRect.width(), minRect.height()
                    );
        }
    }

    return result;
}
*/

UpdateOperationList optimizeVectorizedOperations(UpdateOperationType type, const VectorHashUpdateOperationList &moveOps)
{
    UpdateOperationList newOps;

    VectorHashUpdateOperationListIterator i(moveOps);
    while (i.hasNext())
    {
        i.next();
        const QPoint &vector = i.key();
        const UpdateOperationList &ops = i.value();

        QRegion region;
        foreach (const UpdateOperation &op, ops)
            region += op.srcRect;

        QVector<QRect> rects = TiledRegion::verticallyUniteRects(region.rects());

        foreach (const QRect &rect, rects)
        {
            UpdateOperation op;
            op.type = type;
            op.srcRect = rect;
            op.dstPoint = rect.topLeft() - vector;
            newOps.append(op);
        }
    }

    return newOps;
}

UpdateOperationList optimizeFillOperations(const ColorHashUpdateOperationList &fillOps)
{
    UpdateOperationList newOps;

    ColorHashUpdateOperationListIterator i(fillOps);
    while (i.hasNext())
    {
        i.next();

        const QColor &color = i.key();
        const UpdateOperationList &ops = i.value();

        QRegion region;
        foreach (const UpdateOperation &op, ops)
            region += op.srcRect;

        QVector<QRect> rects = TiledRegion::verticallyUniteRects(region.rects());

        foreach (const QRect &rect, rects)
        {
            UpdateOperation op;
            op.type = uotFill;
            op.srcRect = rect;
            op.dstPoint = rect.topLeft();
            op.fillColor = color;
            newOps.append(op);
        }
    }

    return newOps;
}

UpdateOperationList optimizeUpdateOperations(const UpdateOperationList &ops)
{
    //DGUARDMETHODTIMED;

    VectorHashUpdateOperationList vectorHashedUpdateOps;
    VectorHashUpdateOperationList vectorHashedMoveOps;
    ColorHashUpdateOperationList colorHashedFillOps;

    UpdateOperationList updateOps;
    UpdateOperationList moveOps;
    UpdateOperationList fillOps;

    UpdateOperationList otherOps;

    foreach (const UpdateOperation &op, ops)
    {
        if (op.type == uotUpdate)
        {
            QPoint moveVector(op.srcRect.topLeft() - op.dstPoint);
            vectorHashedUpdateOps[moveVector].append(op);
            updateOps.append(op);
        }
        else if (op.type == uotMove)
        {
            QPoint moveVector(op.srcRect.topLeft() - op.dstPoint);
            vectorHashedMoveOps[moveVector].append(op);
            moveOps.append(op);
        }
        else if (op.type == uotFill)
        {
            colorHashedFillOps[op.fillColor].append(op);
            fillOps.append(op);
        }
        else
            otherOps.append(op);
    }

    UpdateOperationList newUpdateOps = optimizeVectorizedOperations(uotUpdate, vectorHashedUpdateOps);
    UpdateOperationList newMoveOps = optimizeVectorizedOperations(uotMove, vectorHashedMoveOps);
    UpdateOperationList newFillOps = optimizeFillOperations(colorHashedFillOps);

    UpdateOperationList newOps;

    newOps += newUpdateOps.count() < updateOps.count() ? newUpdateOps : updateOps;
    newOps += newFillOps.count() < fillOps.count() ? newFillOps : fillOps;
    newOps += newMoveOps.count() < moveOps.count() ? newMoveOps : moveOps;
    newOps += otherOps;

    DPRINTF("ops: %d, newOps: %d (%d)  moveOps: %d, newMoveOps: %d (%d)  updateOps: %d, newUpdateOps: %d (%d)  fillOps: %d, newFillOps: %d (%d)",
        ops.count(), newOps.count(), ops.count() - newOps.count(),
        moveOps.count(), newMoveOps.count(), moveOps.count() - newMoveOps.count(),
        updateOps.count(), newUpdateOps.count(), updateOps.count() - newUpdateOps.count(),
        fillOps.count(), newFillOps.count(), fillOps.count() - newFillOps.count()
    );

    return newOps;
}

/* ImageComparer */

ImageComparer::ImageComparer(QImage *imageBefore, QImage *imageAfter) :
    imageBefore_(imageBefore),
    imageAfter_(imageAfter),
    moveAnalyzer_(NULL),
    tileWidth_(64)
{
#ifdef USE_MOVE_ANALYZER
#ifdef USE_MOVE_ANALYZER_FINEGRAINED
    moveAnalyzer_ = new MoveAnalyzer(imageBefore_, imageAfter_, imageBefore_->rect(), tileWidth_ / 2);
#else
    moveAnalyzer_ = new MoveAnalyzer(imageBefore_, imageAfter_, imageBefore_->rect(), tileWidth_);
#endif
#endif
}

ImageComparer::~ImageComparer()
{
#ifdef USE_MOVE_ANALYZER
    if (moveAnalyzer_)
        delete moveAnalyzer_;
#endif
}

QVector<QRect> ImageComparer::findDifferences()
{
    TiledRegion region(tileWidth_, tileWidth_);

    int width = imageBefore_->width() / tileWidth_;
    int height = imageBefore_->height() / tileWidth_;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            QRect rect = QRect(x * tileWidth_, y * tileWidth_, tileWidth_, tileWidth_);

            if (!contentMatches<QRgb>(imageBefore_, imageAfter_, rect.topLeft(), rect))
                region += rect;
        }
    }

    return region.rects();
}

#ifdef USE_MULTITHREADING
struct MapProcessRect
{
    MapProcessRect(ImageComparer *comparer, QVector<QRect> *additionalSearchAreas) :
        comparer(comparer),
        additionalSearchAreas(additionalSearchAreas)
    {
    }

    typedef UpdateOperation result_type;

    UpdateOperation operator()(const QRect &rect)
    {
        UpdateOperation op;
        if (!comparer->processRect(rect, op))
            op.type = uotNoOp;

        return op;
    }

    ImageComparer *comparer;
    QVector<QRect> *additionalSearchAreas;
};

struct ReduceProcessRect
{
    ReduceProcessRect(ImageComparer *comparer) :
        comparer(comparer)
    {
    }

    typedef UpdateOperationList result_type;

    void operator()(UpdateOperationList &list, const UpdateOperation &op)
    {
        if (op.type != uotNoOp)
            list.append(op);
    }

    ImageComparer *comparer;
};

void reduceProcessRectToList(UpdateOperationList &list, const UpdateOperation &op)
{
    if (op.type != uotNoOp)
        list.append(op);
}
#endif

UpdateOperationList ImageComparer::findUpdateOperations(const QRect &searchArea, QVector<QRect> *additionalSearchAreas)
{
    DGUARDMETHODTIMED;
    //qDebug("searchArea: %d, %d + %d x %d", searchArea.left(), searchArea.top(), searchArea.width(), searchArea.height());

    UpdateOperationList result;

    QRect minSearchRect = fastFindChangedRect32(imageBefore_, imageAfter_, searchArea);
    if (minSearchRect.isEmpty())
        return result;

    UpdateOperation op;

#if defined(USE_MOVE_ANALYZER) && defined(USE_MOVE_ANALYZER_FINEGRAINED)
    QRegion leftovers;
    MoveOperationList moves = moveAnalyzer_->findMoveOperations(minSearchRect, leftovers, additionalSearchAreas);

    foreach (const MoveOperation &move, moves)
    {
        op.type = uotMove;
        op.srcRect = move.srcRect;
        op.dstPoint = move.dstPoint;
        result.append(op);
    }

    QVector<QRect> searchRects = TiledRegion::verticallyUniteRects(leftovers.rects());

    QList<QRect> tiles;
    foreach (const QRect &rect, searchRects)
        tiles += splitRectIntoTiles(rect, tileWidth_, tileWidth_);
#else

#ifdef USE_MOVE_ANALYZER
    moveAnalyzer_->startNewSearch();
#endif

    QList<QRect> tiles = splitRectIntoTiles(minSearchRect, tileWidth_, tileWidth_);
#endif

#ifdef USE_MULTITHREADING
    result = QtConcurrent::blockingMappedReduced(tiles, MapProcessRect(this, additionalSearchAreas), &reduceProcessRectToList, QtConcurrent::UnorderedReduce);
#else
    foreach (const QRect &rect, tiles)
    {
        if (processRect(rect, op, additionalSearchAreas))
            result.append(op);
    }
#endif

    return result;
}

void ImageComparer::swap()
{
    DGUARDMETHODTIMED;

    QImage *temp = imageBefore_;
    imageBefore_ = imageAfter_;
    imageAfter_ = temp;

#ifdef USE_MOVE_ANALYZER
    moveAnalyzer_->swap();
#endif
}

bool ImageComparer::processRect(const QRect &rect, UpdateOperation &op, QVector<QRect> *additionalSearchAreas)
{
    QRect minRect = fastFindChangedRect32(imageBefore_, imageAfter_, rect);
    if (minRect.isEmpty())
        return false;

#if defined(USE_MOVE_ANALYZER) && !defined(USE_MOVE_ANALYZER_FINEGRAINED)
    QRect movedSrcRect = moveAnalyzer_->processRect(rect);
    if (!movedSrcRect.isEmpty())
    {
        op.type = uotMove;
        op.srcRect = movedSrcRect;
        op.dstPoint = rect.topLeft();

        DPRINTF("Move  %d, %d + %d x %d  ->  %d, %d + %d x %d",
                movedSrcRect.left(), movedSrcRect.top(), rect.width(), rect.height(),
                rect.left(), rect.top(), rect.width(), rect.height()
                );

        return true;
    }
#endif

#ifdef USE_FILL_ANALYZER
    QColor solidColor = getSolidRectColor(imageAfter_, minRect);
    if (solidColor.isValid())
    {
        op.type = uotFill;
        op.srcRect = minRect;
        op.dstPoint = minRect.topLeft();
        op.fillColor = solidColor;

        return true;
    }
#endif

    /*
        if (minRect != rect)
            DPRINTF("Minimized rect:  %d, %d + %d x %d   ->   %d, %d + %d x %d",
                rect.left(), rect.top(), rect.width(), rect.height(),
                minRect.left(), minRect.top(), minRect.width(), minRect.height()
            );
    */

    DPRINTF("No move operation found for: %d, %d + %d x %d",
            minRect.left(), minRect.top(), minRect.width(), minRect.height()
            );

    op.type = uotUpdate;
    op.srcRect = minRect;
    op.dstPoint = minRect.topLeft();

    return true;
}
