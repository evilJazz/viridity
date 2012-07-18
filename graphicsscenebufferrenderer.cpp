#include "graphicsscenebufferrenderer.h"

#undef DEBUG
#include "debug.h"

#include <QVector2D>

struct MoveOperation
{
    QRect srcRect;
    QPoint dstPoint;
};

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
                DPRINTF("Minimized rect:  %d, %d + %d x %d   ->   %d, %d + %d x %d",
                        rect.left(), rect.top(), rect.width(), rect.height(),
                        minRect.left(), minRect.top(), minRect.width(), minRect.height()
                        );

            result += minRect;
        }
        else
            DPRINTF("Removed rect:  %d, %d + %d x %d   ->   %d, %d + %d x %d",
                    rect.left(), rect.top(), rect.width(), rect.height(),
                    minRect.left(), minRect.top(), minRect.width(), minRect.height()
                    );
    }

    return result;
}

inline bool contentMatches(QImage *buffer1, QImage *buffer2, const QPoint &point, const QRect &rect)
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

QRect findMovedRect(QImage *imageBefore, QImage *imageAfter, const QRect &searchArea, const QRect &templateRect)
{
    DGUARDMETHODTIMED;
    QRect roi = searchArea.intersected(imageBefore->rect()).intersected(imageAfter->rect());

    int roiBottom = roi.top() + roi.height() - templateRect.height() + 1;
    int roiRight = roi.left() + roi.width() - templateRect.width() + 1;

    QPoint srcPoint;

    for (int y = roi.top(); y < roiBottom; ++y)
    {
        srcPoint.setY(y);

        for (int x = roi.left(); x < roiRight; ++x)
        {
            srcPoint.setX(x);

            if (contentMatches(imageBefore, imageAfter, srcPoint, templateRect))
                return QRect(srcPoint, templateRect.size());
        }
    }

    return QRect();
}

QList<UpdateOperation> findUpdateOperations(QImage *imageBefore, QImage *imageAfter, const QRect &searchArea)
{
    DGUARDMETHODTIMED;

    QList<QRect> tiles = splitRectIntoTiles(searchArea, 20, 20);

    QList<UpdateOperation> result;
    UpdateOperation op;

    QList<QPoint> lastSuccessfulMoveVectors;
    int movedRectSearchMisses = 0;
    bool movedRectSearchEnabled = true;

    foreach (const QRect &rect, tiles)
    {
        QRect minRect = fastFindChangedRect32(imageBefore, imageAfter, rect);

        if (!minRect.isEmpty())
        {
            /*
            if (minRect != rect)
                DPRINTF("Minimized rect:  %d, %d + %d x %d   ->   %d, %d + %d x %d",
                        rect.left(), rect.top(), rect.width(), rect.height(),
                        minRect.left(), minRect.top(), minRect.width(), minRect.height()
                        );
            */

            //*

            QRect srcRect;

            if (movedRectSearchEnabled)
            {
                QRect movedRectSearchArea;
                if (lastSuccessfulMoveVectors.count() == 0)
                    movedRectSearchArea = minRect.adjusted(-100, -100, 100, 100);
                else
                {
                    foreach (const QPoint &moveVector, lastSuccessfulMoveVectors)
                    {
                        movedRectSearchArea = minRect;
                        movedRectSearchArea.translate(-moveVector);
                        //movedRectSearchArea.adjust(-5, -5, 5, 5);
                        srcRect = findMovedRect(imageBefore, imageAfter, movedRectSearchArea, minRect);

                        if (!srcRect.isEmpty())
                        {
                            DPRINTF("Found move area with existing vector %d x %d", moveVector.x(), moveVector.y());
                            break;
                        }
                    }

                    if (srcRect.isEmpty())
                        movedRectSearchArea = minRect.adjusted(-100, -100, 100, 100);
                }

                if (srcRect.isNull())
                    srcRect = findMovedRect(imageBefore, imageAfter, movedRectSearchArea, minRect);

                if (srcRect.isNull())
                {
                    ++movedRectSearchMisses;
                    if (movedRectSearchMisses == 10)
                        movedRectSearchEnabled = false;
                }
            }

            if (!srcRect.isEmpty())
            {
                QPoint currentMoveVector = minRect.topLeft() - srcRect.topLeft();

                int index = lastSuccessfulMoveVectors.indexOf(currentMoveVector);

                if (index == -1)
                    lastSuccessfulMoveVectors.prepend(currentMoveVector);
                else
                    lastSuccessfulMoveVectors.move(index, 0);

                op.type = uotMove;
                op.srcRect = srcRect;
                op.dstPoint = minRect.topLeft();

                DPRINTF("Move  %d, %d + %d x %d  ->  %d, %d + %d x %d  (vec %d %d)",
                        srcRect.left(), srcRect.top(), minRect.width(), minRect.height(),
                        minRect.left(), minRect.top(), minRect.width(), minRect.height(),
                        currentMoveVector.x(), currentMoveVector.y()
                        );

                result.append(op);
            }
            else
            //*/
            {
                DPRINTF("No move operation found for: %d, %d + %d x %d",
                        minRect.left(), minRect.top(), minRect.width(), minRect.height()
                        );

                op.type = uotUpdate;
                op.srcRect = minRect;
                op.dstPoint = minRect.topLeft();

                result.append(op);
            }
        }
        /*
        else
            DPRINTF("Removed rect:  %d, %d + %d x %d   ->   %d, %d + %d x %d",
                    rect.left(), rect.top(), rect.width(), rect.height(),
                    minRect.left(), minRect.top(), minRect.width(), minRect.height()
                    );
        */
    }

    return result;
}

GraphicsSceneBufferRenderer::GraphicsSceneBufferRenderer(QObject *parent) :
    GraphicsSceneObserver(parent),
    updatesAvailable_(false),
    workBuffer_(&buffer1_),
    otherBuffer_(&buffer2_),
    minimizeDamageRegion_(true)
{
}

GraphicsSceneBufferRenderer::~GraphicsSceneBufferRenderer()
{
}

void GraphicsSceneBufferRenderer::setMinimizeDamageRegion(bool value)
{
    minimizeDamageRegion_ = value;
}

QList<UpdateOperation> GraphicsSceneBufferRenderer::updateBufferExt()
{
    DGUARDMETHODTIMED;

    QRegion updateRegion;
    if (minimizeDamageRegion_)
        updateRegion = QRegion();
    else
        updateRegion = region_;

    QVector<QRect> rects = region_.rects();
    QList<UpdateOperation> ops;

    swapWorkBuffer();
    QPainter p(workBuffer_);

    foreach (const QRect &rect, rects)
    {
        p.eraseRect(rect);
        scene_->render(&p, rect, rect, Qt::IgnoreAspectRatio);

        if (minimizeDamageRegion_)
        {
            QList<UpdateOperation> newOps = findUpdateOperations(otherBuffer_, workBuffer_, rect);

            foreach (const UpdateOperation &op, newOps)
            {
                if (op.type == uotUpdate)
                    updateRegion += QRect(op.dstPoint, op.srcRect.size());
                else
                    ops.append(op);
            }
        }
    }

    rects = updateRegion.rects();

    region_ = QRegion();
    updatesAvailable_ = false;

    foreach (const QRect &rect, rects)
    {
        UpdateOperation op;
        op.type = uotUpdate;
        op.dstPoint = rect.topLeft();
        op.srcRect = rect;
        ops.append(op);
    }

    return ops;
}

QRegion GraphicsSceneBufferRenderer::updateBuffer()
{
    DGUARDMETHODTIMED;

    QRegion result;
    if (minimizeDamageRegion_)
        result = QRegion();
    else
        result = region_;

    QVector<QRect> rects = region_.rects();

    swapWorkBuffer();
    QPainter p(workBuffer_);

    foreach (const QRect &rect, rects)
    {
        p.eraseRect(rect);
        scene_->render(&p, rect, rect, Qt::IgnoreAspectRatio);

        if (minimizeDamageRegion_)
        {
            QList<QRect> minRects = findUpdateRects(&buffer1_, &buffer2_, rect);
            foreach (const QRect &minRect, minRects)
                result += minRect;
        }
    }

    region_ = QRegion();
    updatesAvailable_ = false;

    return result;
}

void GraphicsSceneBufferRenderer::fullUpdate()
{
    // TODO: This method is inefficient. Optimize!!
    workBuffer_->fill(0);
    region_ += QRect(0, 0, workBuffer_->width(), workBuffer_->height());
    emitUpdatesAvailable();
}

void GraphicsSceneBufferRenderer::sceneAttached()
{
    int width = 1024; //scene_->width();
    int height = 768; //scene_->height();

    if (width != buffer1_.width() || height != buffer1_.height())
        buffer1_ = QImage(width, height, QImage::Format_ARGB32);

    if (width != buffer2_.width() || height != buffer2_.height())
        buffer2_ = QImage(width, height, QImage::Format_ARGB32);

    fullUpdate();
}

void GraphicsSceneBufferRenderer::sceneSceneRectChanged(const QRectF &newRect)
{
    sceneAttached();
}

static int updateNo = 0;
void GraphicsSceneBufferRenderer::sceneChanged(const QList<QRectF> &rects)
{
    DGUARDMETHODTIMED;
    DPRINTF("UpDaTe %d", updateNo++);

    QString rectString;

    foreach (const QRectF &rect, rects)
    {
        QRect newRect = rect.toAlignedRect();
        newRect.adjust(-2, -2, 2, 2); // similar to void QGraphicsView::updateScene(const QList<QRectF> &rects)
        region_ += newRect.intersected(workBuffer_->rect());
        DOP(rectString += QString().sprintf(" %4d,%4d+%4dx%4d", newRect.left(), newRect.top(), newRect.width(), newRect.height()));
    }

    DPRINTF("rects: %s", rectString.toUtf8().constData());

    emitUpdatesAvailable();
}

void GraphicsSceneBufferRenderer::sceneDetached()
{
    buffer1_ = QImage();
    buffer2_ = QImage();
    region_ = QRegion();
}

void GraphicsSceneBufferRenderer::swapWorkBuffer()
{
    DGUARDMETHODTIMED;

    if (workBuffer_ == &buffer1_)
    {
        workBuffer_ = &buffer2_;
        otherBuffer_ = &buffer1_;
    }
    else
    {
        workBuffer_ = &buffer1_;
        otherBuffer_ = &buffer2_;
    }

    // copy content over to new work buffer...
    *workBuffer_ = *otherBuffer_;
}

void GraphicsSceneBufferRenderer::emitUpdatesAvailable()
{
    updatesAvailable_ = true;
    emit damagedRegionAvailable();
}

