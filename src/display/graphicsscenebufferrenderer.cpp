#include "graphicsscenebufferrenderer.h"

#include <QThread>

#define USE_SCENE_DAMAGEREGION

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

/* GraphicsSceneBufferRendererLocker */

GraphicsSceneBufferRendererLocker::GraphicsSceneBufferRendererLocker(GraphicsSceneBufferRenderer *renderer) :
    m_(&renderer->bufferAndRegionMutex_)
{
}

/* GraphicsSceneBufferRenderer */

GraphicsSceneBufferRenderer::GraphicsSceneBufferRenderer(QObject *parent) :
    QObject(parent),
    adapter_(NULL),
    minimizeDamageRegion_(true),
    updatesAvailable_(false),
    bufferAndRegionMutex_(QMutex::Recursive),
    workBuffer_(&buffer1_),
    otherBuffer_(&buffer2_),
    comparer_(NULL)
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

void GraphicsSceneBufferRenderer::setTargetGraphicsSceneAdapter(GraphicsSceneAdapter *adapter)
{
    if (adapter != adapter_)
    {
        if (adapter_)
        {
            sceneDetaching();

            disconnect(adapter_, SIGNAL(sceneChanged(QList<QRectF>)));
            disconnect(adapter_, SIGNAL(destroyed()));

            sceneDetached();
        }

        adapter_ = adapter;

        if (adapter_)
        {
            sceneAttached();

            connect(adapter_, SIGNAL(destroyed()), this, SLOT(sceneDestroyed()));
            connect(adapter_, SIGNAL(sceneChanged(QList<QRectF>)), this, SLOT(sceneChanged(QList<QRectF>)));
        }
    }
}

void GraphicsSceneBufferRenderer::initComparer()
{
    QMutexLocker m(&bufferAndRegionMutex_);

    if (comparer_)
        delete comparer_;

    comparer_ = new ImageComparer(otherBuffer_, workBuffer_);
    comparer_->setSettings(settings_);
}

void GraphicsSceneBufferRenderer::setMinimizeDamageRegion(bool value)
{
    minimizeDamageRegion_ = value;
}

const ComparerSettings &GraphicsSceneBufferRenderer::settings() const
{
    return settings_;
}

void GraphicsSceneBufferRenderer::setSettings(const ComparerSettings &settings)
{
    settings_ = settings;

    if (comparer_)
        comparer_->setSettings(settings_);
}

QVector<QRect> GraphicsSceneBufferRenderer::paintUpdatesToBuffer(QPainter &p)
{
    DTIMERINIT(paintTimer);
    // Properly set up to make eraseRect work the way we expect it to work,
    // ie. replace with the defined transparent brush instead of merging/blending
    p.setBackground(QBrush(QColor(255, 255, 255, 0)));
    p.setCompositionMode(QPainter::CompositionMode_Source);

    QVector<QRect> rects;

    if (adapter_)
    {
#ifdef USE_SCENE_DAMAGEREGION
        rects = damageRegion_.rects();
        foreach (const QRect &rect, rects)
            p.eraseRect(rect);

        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        adapter_->render(&p, rects);
#else
        p.eraseRect(workBuffer_->rect());

        p.setCompositionMode(QPainter::CompositionMode_SourceOver);

        adapter_->render(&p, workBuffer_->rect(), workBuffer_->rect(), Qt::IgnoreAspectRatio);
#endif

        rects = comparer_->findDifferences();
    }
    else
    {
        if (!pushedFullFrame_.isNull())
        {
            p.eraseRect(workBuffer_->rect());
            p.drawImage(0, 0, pushedFullFrame_);
            rects = comparer_->findDifferences();
        }
    }

    DTIMERPRINT(paintTimer, "Damage region paint");

    return rects;
}

UpdateOperationList GraphicsSceneBufferRenderer::updateBuffer()
{
    DGUARDMETHODTIMED;
    QMutexLocker m(&bufferAndRegionMutex_);

    swapWorkBuffer();

    QPainter p(workBuffer_);
    QVector<QRect> rects = paintUpdatesToBuffer(p);

    UpdateOperationList ops;

    if (minimizeDamageRegion_)
    {
        DTIMERINIT(optTimer);

        foreach (const QRect &rect, rects)
            ops += comparer_->findUpdateOperations(rect, &rects);

        ops = ImageComparerOpTools::optimizeUpdateOperations(ops);

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

void GraphicsSceneBufferRenderer::pushFullFrame(const QImage &image)
{
    if (!adapter_)
    {
        pushedFullFrame_ = image;
        emitUpdatesAvailable();
    }
}

void GraphicsSceneBufferRenderer::fullUpdate()
{
    // TODO: This method is inefficient. Optimize!!
    QMutexLocker m(&bufferAndRegionMutex_);
    otherBuffer_->fill(0xFFFFFFFF);
    workBuffer_->fill(0);
    damageRegion_ += QRect(0, 0, workBuffer_->width(), workBuffer_->height());
    emitUpdatesAvailable();
}

void GraphicsSceneBufferRenderer::setSize(int width, int height)
{
    DGUARDMETHODFTIMED("GraphicsSceneBufferRenderer: %p", this);
    QMutexLocker m(&bufferAndRegionMutex_);

    if (width == 0 || height == 0)
        return;

    if (buffer1_.width() != width || buffer1_.height() != height)
    {
        if (width != buffer1_.width() || height != buffer1_.height())
        {
            if (buffer1_.isNull())
            {
                buffer1_ = QImage(width, height, QImage::Format_ARGB32);
                buffer1_.fill(0);
            }
            else
                buffer1_ = buffer1_.copy(0, 0, width, height);
        }

        if (width != buffer2_.width() || height != buffer2_.height())
        {
            if (buffer2_.isNull())
            {
                buffer2_ = QImage(width, height, QImage::Format_ARGB32);
                buffer2_.fill(0);
            }
            else
                buffer2_ = buffer2_.copy(0, 0, width, height);
        }

        initComparer();
        damageRegion_.clear();
        damageRegion_ += QRect(0, 0, workBuffer_->width(), workBuffer_->height());
        emitUpdatesAvailable();
    }
}

void GraphicsSceneBufferRenderer::setSizeFromScene()
{
    if (adapter_)
    {
        int width = adapter_->width();
        int height = adapter_->height();
        setSize(width, height);
    }
}

void GraphicsSceneBufferRenderer::sceneAttached()
{
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

void GraphicsSceneBufferRenderer::sceneDetaching()
{

}

void GraphicsSceneBufferRenderer::sceneDetached()
{
    QMutexLocker m(&bufferAndRegionMutex_);
    buffer1_ = QImage();
    buffer2_ = QImage();
    damageRegion_.clear();
}

void GraphicsSceneBufferRenderer::sceneDestroyed()
{
    DGUARDMETHODTIMED;
    adapter_ = NULL;
    sceneDetached();
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
