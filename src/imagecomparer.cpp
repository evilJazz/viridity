#include "imagecomparer.h"
#include "moveanalyzer.h"

#include <QtConcurrentMap>

#undef DEBUG
#include "private/debug.h"

#define USE_MOVE_ANALYZER
//#define USE_FILL_ANALYZER
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

bool contentMatches(QImage *buffer1, QImage *buffer2, const QPoint &point, const QRect &rect)
{
    QRect rect1 = QRect(point, rect.size()).intersected(buffer1->rect());
    QRect rect2 = rect.intersected(buffer2->rect());

    if (rect1.size() != rect.size() || rect2.size() != rect.size())
        return false;

    QRgb *pBuf1, *pBuf2;

    for (int y = 0; y < rect1.height(); ++y)
    {
        pBuf1 = (QRgb *)buffer1->scanLine(rect1.top() + y) + rect1.left();
        pBuf2 = (QRgb *)buffer2->scanLine(rect2.top() + y) + rect2.left();

        for (int x = 0; x < rect1.width(); ++x)
        {
            if (*pBuf1 != *pBuf2)
                return false;

            ++pBuf1;
            ++pBuf2;
        }
    }

    return true;
}

inline bool fastContentMatches(QImage *buffer1, QImage *buffer2, const QPoint &point, const QRect &rect)
{
    QRect rect1 = QRect(point, rect.size()).intersected(buffer1->rect());
    QRect rect2 = rect.intersected(buffer2->rect());

    if (rect1.size() != rect.size() || rect2.size() != rect.size())
        return false;

    QRgb *pBuf1, *pBuf2;

    const int stepWidth = 5;

    for (int s = 0; s < stepWidth; ++s)
    {
        for (int y = s; y < rect1.height(); y += stepWidth)
        {
            pBuf1 = (QRgb *)buffer1->scanLine(rect1.top() + y) + rect1.left();
            pBuf2 = (QRgb *)buffer2->scanLine(rect2.top() + y) + rect2.left();

            for (int x = s; x < rect1.width(); x += stepWidth)
            {
                if (*pBuf1 != *pBuf2)
                    return false;

                pBuf1 += stepWidth;
                pBuf2 += stepWidth;
            }
        }
    }

    return true;
}

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

        QVector<QRect> rects = region.rects();

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

        QVector<QRect> rects = region.rects();

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
    mutex_(QMutex::Recursive),
    moveAnalyzer_(NULL),
    movedRectSearchMisses_(0),
    movedRectSearchEnabled_(true),
    tileWidth_(20)
{
#ifdef USE_MOVE_ANALYZER
    moveAnalyzer_ = new MoveAnalyzer(imageBefore_, imageAfter_, imageBefore_->rect(), tileWidth_);
#endif
}

ImageComparer::~ImageComparer()
{
#ifdef USE_MOVE_ANALYZER
    if (moveAnalyzer_)
        delete moveAnalyzer_;
#endif
}

#ifdef USE_MULTITHREADING
struct MapProcessRect
{
    MapProcessRect(ImageComparer *comparer) :
        comparer(comparer)
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

UpdateOperationList ImageComparer::findUpdateOperations(const QRect &searchArea)
{
    DGUARDMETHODTIMED;
    //qDebug("searchArea: %d, %d + %d x %d", searchArea.left(), searchArea.top(), searchArea.width(), searchArea.height());

    UpdateOperationList result;
    UpdateOperation op;

#ifdef USE_MOVE_ANALYZER
    movedRectSearchMisses_ = 0;
    movedRectSearchEnabled_ = true;
#endif

    QList<QRect> tiles = splitRectIntoTiles(searchArea, tileWidth_, tileWidth_);

#ifdef USE_MULTITHREADING
    result = QtConcurrent::blockingMappedReduced(tiles, MapProcessRect(this), &reduceProcessRectToList, QtConcurrent::UnorderedReduce);
#else
    foreach (const QRect &rect, tiles)
    {
        if (processRect(rect, op))
            result.append(op);
    }
#endif

    return result;
}

void ImageComparer::swap()
{
    QImage *temp = imageBefore_;
    imageBefore_ = imageAfter_;
    imageAfter_ = temp;

#ifdef USE_MOVE_ANALYZER
    QRegion region;
    foreach (const QRect &rect, damagedAreas_)
        region += rect;

    damagedAreas_.clear();

    while (lastSuccessfulMoveVectors_.count() > 10)
        lastSuccessfulMoveVectors_.removeLast();

    moveAnalyzer_->swap();

    const QVector<QRect> rects = region.rects();
    foreach (const QRect &rect, rects)
        moveAnalyzer_->updateArea(rect);
#endif
}

bool ImageComparer::processRect(const QRect &rect, UpdateOperation &op)
{
    QRect minRect = fastFindChangedRect32(imageBefore_, imageAfter_, rect);
    if (minRect.isEmpty())
        return false;

#ifdef USE_MOVE_ANALYZER
    QMutexLocker l(&mutex_);
    damagedAreas_.append(rect); // used for updating MoveAnalyzer instance in swap()
    l.unlock();

    if (movedRectSearchEnabled_ && rect.width() == tileWidth_)
    {
        QRect movedSrcRect;
        QRect movedRectSearchArea;
        if (lastSuccessfulMoveVectors_.count() == 0)
            movedRectSearchArea = rect.adjusted(-100, -100, 100, 100);
        else
        {
            QMutexLocker l(&mutex_);
            QList<QPoint> list = lastSuccessfulMoveVectors_;
            list.detach();
            l.unlock();

            foreach (const QPoint &moveVector, list)
            {
                movedRectSearchArea = rect;
                movedRectSearchArea.translate(-moveVector);
                //movedRectSearchArea.adjust(-5, -5, 5, 5);
                movedSrcRect = moveAnalyzer_->findMovedRect(movedRectSearchArea, rect);

                if (!movedSrcRect.isEmpty())
                {
                    DPRINTF("Found move area with existing vector %d x %d", moveVector.x(), moveVector.y());
                    break;
                }
            }

            if (movedSrcRect.isEmpty())
                movedRectSearchArea = rect.adjusted(-100, -100, 100, 100);
        }

        if (movedSrcRect.isNull())
            movedSrcRect = moveAnalyzer_->findMovedRect(movedRectSearchArea, rect);

        if (movedSrcRect.isNull())
        {
            QMutexLocker l(&mutex_);
            ++movedRectSearchMisses_;
            //if (movedRectSearchMisses == 10)
            //    movedRectSearchEnabled = false;
        }

        if (!movedSrcRect.isEmpty())
        {
            QPoint currentMoveVector = rect.topLeft() - movedSrcRect.topLeft();

            QMutexLocker l(&mutex_);

            int index = lastSuccessfulMoveVectors_.indexOf(currentMoveVector);

            if (index == -1)
                lastSuccessfulMoveVectors_.prepend(currentMoveVector);
            else
                lastSuccessfulMoveVectors_.move(index, 0);

            l.unlock();

            op.type = uotMove;
            op.srcRect = movedSrcRect;
            op.dstPoint = rect.topLeft();

            DPRINTF("Move  %d, %d + %d x %d  ->  %d, %d + %d x %d  (vec %d %d)",
                    movedSrcRect.left(), movedSrcRect.top(), rect.width(), rect.height(),
                    rect.left(), rect.top(), rect.width(), rect.height(),
                    currentMoveVector.x(), currentMoveVector.y()
                    );

            return true;
        }
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
