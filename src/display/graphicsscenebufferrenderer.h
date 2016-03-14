#ifndef GRAPHICSSCENEBUFFERRENDERER_H
#define GRAPHICSSCENEBUFFERRENDERER_H

#include "viridity_global.h"

#include <QImage>
#include <QPainter>
#include <QRegion>
#include <QMutex>

#include "graphicssceneadapter.h"
#include "comparer/tiles.h"
#include "comparer/imagecomparer.h"

class GraphicsSceneBufferRenderer;

class GraphicsSceneBufferRendererLocker
{
public:
    GraphicsSceneBufferRendererLocker(GraphicsSceneBufferRenderer *renderer);
private:
    QMutexLocker m_;
};

class VIRIDITY_EXPORT GraphicsSceneBufferRenderer : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneBufferRenderer(QObject *parent = 0);
    virtual ~GraphicsSceneBufferRenderer();

    void setTargetGraphicsSceneAdapter(GraphicsSceneAdapter *adapter);
    GraphicsSceneAdapter *targetGraphicsSceneAdapter() const { return adapter_; }

    void setMinimizeDamageRegion(bool value);
    bool minimizeDamageRegion() { return minimizeDamageRegion_; }

    const ComparerSettings &settings() const;
    void setSettings(const ComparerSettings &settings);

    UpdateOperationList updateBuffer();

    const QImage &buffer() const { return *workBuffer_; }
    bool updatesAvailable() const { return updatesAvailable_; }

    void pushFullFrame(const QImage& image); // Only works if we have a NULL adapter.

public slots:
    void fullUpdate();
    void setSize(int width, int height);

signals:
    void damagedRegionAvailable();

protected slots:
    void sceneAttached();
    void sceneChanged(QList<QRectF> rects);
    void sceneDetaching();
    void sceneDetached();
    void sceneDestroyed();

protected:
    GraphicsSceneAdapter *adapter_;

    bool minimizeDamageRegion_;
    bool updatesAvailable_;

    friend class GraphicsSceneBufferRendererLocker;
    mutable QMutex bufferAndRegionMutex_;

    QImage *workBuffer_;
    QImage *otherBuffer_;
    QImage buffer1_;
    QImage buffer2_;
    TiledRegion damageRegion_;

    ImageComparer *comparer_;
    ComparerSettings settings_;

    QImage pushedFullFrame_;

    void setSizeFromScene();

    void initComparer();
    void swapWorkBuffer();
    void emitUpdatesAvailable();

    QVector<QRect> paintUpdatesToBuffer(QPainter &p);
};

#endif // GRAPHICSSCENEBUFFERRENDERER_H
