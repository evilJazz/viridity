#include "graphicsscenebufferrenderer.h"

#undef DEBUG
#include "debug.h"

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

QRect fastFindChangedRect32(QImage *buffer1, QImage *buffer2, const QRect &searchArea)
{
    DGUARDMETHODTIMED;
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

QList<QRect> findUpdateRects(QImage *buffer1, QImage *buffer2, const QRect &searchArea)
{
    DGUARDMETHODTIMED;

    const int cBlockGranularityX = 128;
    const int cBlockGranularityY = 128;

    QList<QRect> blocks;
    int leftX = searchArea.width() % cBlockGranularityX;
    int leftY = searchArea.height() % cBlockGranularityY;

    int y = searchArea.top();
    int x;

    int bottomLimit = searchArea.bottom() + 1 - cBlockGranularityY + 1;
    int rightLimit = searchArea.right() + 1 - cBlockGranularityX + 1;

    for (y = searchArea.top(); y < bottomLimit; y += cBlockGranularityY)
    {
        for (x = searchArea.left(); x < rightLimit; x += cBlockGranularityX)
            blocks += QRect(x, y, cBlockGranularityX, cBlockGranularityY);

        if (leftX > 0)
            blocks += QRect(x, y, leftX, cBlockGranularityY);
    }

    if (leftY > 0)
    {
        for (x = searchArea.left(); x < rightLimit; x += cBlockGranularityX)
            blocks += QRect(x, y, cBlockGranularityX, leftY);

        if (leftX > 0)
            blocks += QRect(x, y, leftX, leftY);
    }

    QList<QRect> result;

    foreach (const QRect &rect, blocks)
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
        buffer1_ = QImage(width, height, QImage::Format_ARGB32_Premultiplied);

    if (width != buffer2_.width() || height != buffer2_.height())
        buffer2_ = QImage(width, height, QImage::Format_ARGB32_Premultiplied);

    fullUpdate();
}

void GraphicsSceneBufferRenderer::sceneSceneRectChanged(const QRectF &newRect)
{
    sceneAttached();
}

void GraphicsSceneBufferRenderer::sceneChanged(const QList<QRectF> &rects)
{
    DGUARDMETHODTIMED;

    QString rectString;

    foreach (const QRectF &rect, rects)
    {
        QRect newRect = rect.toAlignedRect();
        newRect.adjust(-2, -2, 2, 2); // similar to void QGraphicsView::updateScene(const QList<QRectF> &rects)
        region_ += newRect.intersected(workBuffer_->rect());
        DOP(rectString += QString().sprintf(" %.2f,%.2f+%.2fx%.2f", newRect.left(), newRect.top(), newRect.width(), newRect.height()));
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

