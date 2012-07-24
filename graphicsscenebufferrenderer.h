#ifndef GRAPHICSSCENEBUFFERRENDERER_H
#define GRAPHICSSCENEBUFFERRENDERER_H

#include <QImage>
#include <QPainter>
#include <QRegion>
#include <QThread>

#include "graphicssceneobserver.h"
#include "imagecomparer.h"

class SynchronizedSceneRenderer : public QObject
{
    Q_OBJECT
public:
    SynchronizedSceneRenderer(QGraphicsScene *scene) :
        scene_(scene)
    {
        if (scene_->thread() != QThread::currentThread())
            this->moveToThread(scene_->thread());
    }

    void render(QPainter *painter, const QRectF &target = QRectF(), const QRectF &source = QRectF(), Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio)
    {
        if (scene_->thread() != QThread::currentThread())
            metaObject()->invokeMethod(
                this, "otherThreadRender", Qt::BlockingQueuedConnection,
                Q_ARG(QPainter *, painter),
                Q_ARG(const QRectF &, target),
                Q_ARG(const QRectF &, source),
                Q_ARG(Qt::AspectRatioMode, aspectRatioMode)
            );
        else
            otherThreadRender(painter, target, source, aspectRatioMode);
    }

private slots:
    void otherThreadRender(QPainter *painter, const QRectF &target = QRectF(), const QRectF &source = QRectF(), Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio)
    {
        scene_->render(painter, target, source, aspectRatioMode);
    }

private:
    QGraphicsScene *scene_;
};

class GraphicsSceneBufferRenderer : public GraphicsSceneObserver
{
    Q_OBJECT
public:
    explicit GraphicsSceneBufferRenderer(QObject *parent = 0);
    virtual ~GraphicsSceneBufferRenderer();

    void setMinimizeDamageRegion(bool value);
    bool minimizeDamageRegion() { return minimizeDamageRegion_; }

    QRegion updateBuffer();
    UpdateOperationList updateBufferExt();
    QImage buffer() const { return *workBuffer_; }
    bool updatesAvailable() const { return updatesAvailable_; }

public slots:
    void fullUpdate();

signals:
    void damagedRegionAvailable();

protected slots:
    void sceneAttached();
    void sceneSceneRectChanged(QRectF newRect);
    void sceneChanged(QList<QRectF> rects);
    void sceneDetached();

protected:
    bool minimizeDamageRegion_;
    bool updatesAvailable_;
    QImage *workBuffer_;
    QImage *otherBuffer_;
    QImage buffer1_;
    QImage buffer2_;
    QRegion region_;

    void swapWorkBuffer();
    void emitUpdatesAvailable();
};

#endif // GRAPHICSSCENEBUFFERRENDERER_H
