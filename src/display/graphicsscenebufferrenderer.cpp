#include "graphicsscenebufferrenderer.h"

#include <QThread>

#include "private/synchronizedscenerenderer.h"

#define USE_SCENE_DAMAGEREGION

//#undef DEBUG
//#define DIAGNOSTIC_OUTPUT
#include "KCL/debug.h"

/* GraphicsSceneBufferRenderer */

GraphicsSceneBufferRenderer::GraphicsSceneBufferRenderer(QObject *parent) :
    GraphicsSceneObserver(parent),
    minimizeDamageRegion_(true),
    updatesAvailable_(false),
    workBuffer_(&buffer1_),
    otherBuffer_(&buffer2_),
    comparer_(NULL),
    bufferAndRegionMutex_(QMutex::Recursive)
{
    DGUARDMETHODTIMED;

    initComparer();
}

GraphicsSceneBufferRenderer::~GraphicsSceneBufferRenderer()
{
    DGUARDMETHODTIMED;

    if (comparer_)
        delete comparer_;
}

void GraphicsSceneBufferRenderer::initComparer()
{
    QMutexLocker m(&bufferAndRegionMutex_);

    if (comparer_)
        delete comparer_;

    comparer_ = new ImageComparer(otherBuffer_, workBuffer_);
}

void GraphicsSceneBufferRenderer::setMinimizeDamageRegion(bool value)
{
    minimizeDamageRegion_ = value;
}

UpdateOperationList GraphicsSceneBufferRenderer::updateBufferExt()
{
    DGUARDMETHODTIMED;
    QMutexLocker m(&bufferAndRegionMutex_);

    swapWorkBuffer();

    DTIMERINIT(paintTimer);
    QPainter p(workBuffer_);

    // Properly set up to make eraseRect work the way we expect it to work,
    // ie. replace with the defined transparent brush instead of merging/blending
    p.setBackground(QBrush(QColor(255, 255, 255, 0)));
    p.setCompositionMode(QPainter::CompositionMode_Source);

#ifdef USE_SCENE_DAMAGEREGION
    QVector<QRect> rects = damageRegion_.rects();
    foreach (const QRect &rect, rects)
        p.eraseRect(rect);

    SynchronizedSceneRenderer syncedSceneRenderer(scene_);
    syncedSceneRenderer.render(&p, rects);
#else
    p.eraseRect(workBuffer_->rect());

    SynchronizedSceneRenderer syncedSceneRenderer(scene_);
    syncedSceneRenderer.render(&p, workBuffer_->rect(), workBuffer_->rect(), Qt::IgnoreAspectRatio);

    QVector<QRect> rects = comparer_->findDifferences();
#endif
    DTIMERPRINT(paintTimer, "Damage region paint");

    UpdateOperationList ops;

    if (minimizeDamageRegion_)
    {
        DTIMERINIT(optTimer);

        foreach (const QRect &rect, rects)
            ops += comparer_->findUpdateOperations(rect, &rects);

        ops = optimizeUpdateOperations(ops);

        DTIMERPRINT(optTimer, "Damage region optimization");
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

    damageRegion_.clear();
    updatesAvailable_ = false;

    return ops;
}

/*
QRegion GraphicsSceneBufferRenderer::updateBuffer()
{
    DGUARDMETHODTIMED;
    QMutexLocker m(&bufferAndRegionMutex_);

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

    region_ = TiledRegion();
    updatesAvailable_ = false;

    return result;
}
*/

void GraphicsSceneBufferRenderer::fullUpdate()
{
    // TODO: This method is inefficient. Optimize!!
    QMutexLocker m(&bufferAndRegionMutex_);
    otherBuffer_->fill(-1);
    workBuffer_->fill(0);
    damageRegion_ += QRect(0, 0, workBuffer_->width(), workBuffer_->height());
    emitUpdatesAvailable();
}

void GraphicsSceneBufferRenderer::setSizeFromScene()
{
    DGUARDMETHODFTIMED("GraphicsSceneBufferRenderer: %p", this);
    QMutexLocker m(&bufferAndRegionMutex_);

    int width = scene_->width();
    int height = scene_->height();

    if (buffer1_.width() != width || buffer1_.height() != height)
    {
        if (width != buffer1_.width() || height != buffer1_.height())
            buffer1_ = QImage(width, height, QImage::Format_ARGB32);

        if (width != buffer2_.width() || height != buffer2_.height())
            buffer2_ = QImage(width, height, QImage::Format_ARGB32);

        workBuffer_ = &buffer1_;
        otherBuffer_ = &buffer2_;

        initComparer();
        damageRegion_.clear();
        fullUpdate();
    }
}

void GraphicsSceneBufferRenderer::sceneAttached()
{
    setSizeFromScene();
}

void GraphicsSceneBufferRenderer::sceneSceneRectChanged(QRectF newRect)
{
    //qDebug("Scene rect changed!");
    setSizeFromScene();
}

void GraphicsSceneBufferRenderer::sceneChanged(QList<QRectF> rects)
{
    //DGUARDMETHODTIMED;
#ifdef USE_SCENE_DAMAGEREGION
    QMutexLocker m(&bufferAndRegionMutex_);

    QString rectString;

    foreach (const QRectF &rect, rects)
    {
        QRect newRect = rect.toAlignedRect();
        newRect.adjust(-2, -2, 2, 2); // similar to void QGraphicsView::updateScene(const QList<QRectF> &rects)
        damageRegion_ += newRect.intersected(workBuffer_->rect());
        //DOP(rectString += QString().sprintf(" %4d,%4d+%4dx%4d", newRect.left(), newRect.top(), newRect.width(), newRect.height()));
    }

    //DPRINTF("rects: %s", rectString.toUtf8().constData());
#endif
    emitUpdatesAvailable();
}

void GraphicsSceneBufferRenderer::sceneDetached()
{
    QMutexLocker m(&bufferAndRegionMutex_);
    buffer1_ = QImage();
    buffer2_ = QImage();
    damageRegion_.clear();
}

void GraphicsSceneBufferRenderer::swapWorkBuffer()
{
    //DGUARDMETHODTIMED;
    QMutexLocker m(&bufferAndRegionMutex_);

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

    comparer_->swap();

    // copy content over to new work buffer...
    *workBuffer_ = *otherBuffer_;
}

void GraphicsSceneBufferRenderer::emitUpdatesAvailable()
{
    updatesAvailable_ = true;
    emit damagedRegionAvailable();
}
