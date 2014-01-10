#ifndef GRAPHICSSCENEBUFFERRENDERER_H
#define GRAPHICSSCENEBUFFERRENDERER_H

#include "viridity_global.h"

#include <QImage>
#include <QPainter>
#include <QRegion>

#include "graphicssceneobserver.h"
#include "imagecomparer.h"

class VIRIDITY_EXPORT GraphicsSceneBufferRenderer : public GraphicsSceneObserver
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
    ImageComparer *comparer_;

    void initComparer();
    void swapWorkBuffer();
    void emitUpdatesAvailable();
};

#endif // GRAPHICSSCENEBUFFERRENDERER_H