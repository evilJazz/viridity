#ifndef GRAPHICSSCENEBUFFERRENDERER_H
#define GRAPHICSSCENEBUFFERRENDERER_H

#include "viridity_global.h"

#include <QImage>
#include <QPainter>
#include <QRegion>
#include <QMutex>

#include "graphicssceneobserver.h"
#include "imagecomparer.h"
#include "tiledregion.h"

class VIRIDITY_EXPORT GraphicsSceneBufferRenderer : public GraphicsSceneObserver
{
    Q_OBJECT
public:
    explicit GraphicsSceneBufferRenderer(QObject *parent = 0);
    virtual ~GraphicsSceneBufferRenderer();

    void setMinimizeDamageRegion(bool value);
    bool minimizeDamageRegion() { return minimizeDamageRegion_; }

    //QRegion updateBuffer();
    UpdateOperationList updateBufferExt();
    QImage &buffer() const { return *workBuffer_; }
    bool updatesAvailable() const { return updatesAvailable_; }

    int tileSize() const { return comparer_ ? comparer_->tileSize() : 0; }

public slots:
    void fullUpdate();
    void setSize(int width, int height);

signals:
    void damagedRegionAvailable();

protected slots:
    void sceneAttached();
    void sceneChanged(QList<QRectF> rects);
    void sceneDetached();

protected:
    bool minimizeDamageRegion_;
    bool updatesAvailable_;

    QMutex bufferAndRegionMutex_;

    QImage *workBuffer_;
    QImage *otherBuffer_;
    QImage buffer1_;
    QImage buffer2_;
    TiledRegion damageRegion_;
    ImageComparer *comparer_;

    void setSizeFromScene();

    void initComparer();
    void swapWorkBuffer();
    void emitUpdatesAvailable();
};

#endif // GRAPHICSSCENEBUFFERRENDERER_H
