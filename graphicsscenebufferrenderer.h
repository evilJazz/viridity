#ifndef GRAPHICSSCENEBUFFERRENDERER_H
#define GRAPHICSSCENEBUFFERRENDERER_H

#include <QImage>
#include <QPainter>
#include <QRegion>

#include "graphicssceneobserver.h"

class GraphicsSceneBufferRenderer : public GraphicsSceneObserver
{
    Q_OBJECT
public:
    explicit GraphicsSceneBufferRenderer(QObject *parent = 0);
    virtual ~GraphicsSceneBufferRenderer();

    void setMinimizeDamageRegion(bool value);
    bool minimizeDamageRegion() { return minimizeDamageRegion_; }

    QRegion updateBuffer();
    QImage buffer() const { return *workBuffer_; }
    bool updatesAvailable() const { return updatesAvailable_; }

public slots:
    void fullUpdate();

signals:
    void damagedRegionAvailable();

protected slots:
    void sceneAttached();
    void sceneSceneRectChanged(const QRectF &newRect);
    void sceneChanged(const QList<QRectF> &rects);
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
