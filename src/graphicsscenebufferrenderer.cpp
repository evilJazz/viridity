#include "graphicsscenebufferrenderer.h"

#include <QThread>

#include "private/synchronizedscenerenderer.h"

#undef DEBUG
#include "private/debug.h"

/* GraphicsSceneBufferRenderer */

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

UpdateOperationList GraphicsSceneBufferRenderer::updateBufferExt()
{
    DGUARDMETHODTIMED;

    QVector<QRect> rects = region_.rects();

    swapWorkBuffer();
    QPainter p(workBuffer_);

    ImageComparer comparer(otherBuffer_, workBuffer_);
    SynchronizedSceneRenderer syncedSceneRenderer(scene_);

    foreach (const QRect &rect, rects)
    {
        p.eraseRect(rect);
        //scene_->render(&p, rect, rect, Qt::IgnoreAspectRatio);
        syncedSceneRenderer.render(&p, rect, rect, Qt::IgnoreAspectRatio);
    }

    UpdateOperationList ops;

    if (minimizeDamageRegion_)
    {
        foreach (const QRect &rect, rects)
            ops += comparer.findUpdateOperations(rect);

        ops = optimizeUpdateOperations(ops);
    }
    else
    {
        foreach (const QRect &rect, rects)
        {
            UpdateOperation op;
            op.type = uotUpdate;
            op.dstPoint = rect.topLeft();
            op.srcRect = rect;
            ops.append(op);
        }
    }

    region_ = QRegion();
    updatesAvailable_ = false;

    return ops;
}

QRegion GraphicsSceneBufferRenderer::updateBuffer()
{
    //DGUARDMETHODTIMED;

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

void GraphicsSceneBufferRenderer::sceneSceneRectChanged(QRectF newRect)
{
    sceneAttached();
}

static int updateNo = 0;
void GraphicsSceneBufferRenderer::sceneChanged(QList<QRectF> rects)
{
    //DGUARDMETHODTIMED;
    //DPRINTF("UpDaTe %d", updateNo++);

    QString rectString;

    foreach (const QRectF &rect, rects)
    {
        QRect newRect = rect.toAlignedRect();
        newRect.adjust(-2, -2, 2, 2); // similar to void QGraphicsView::updateScene(const QList<QRectF> &rects)
        region_ += newRect.intersected(workBuffer_->rect());
        DOP(rectString += QString().sprintf(" %4d,%4d+%4dx%4d", newRect.left(), newRect.top(), newRect.width(), newRect.height()));
    }

    //DPRINTF("rects: %s", rectString.toUtf8().constData());

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
    //DGUARDMETHODTIMED;

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

