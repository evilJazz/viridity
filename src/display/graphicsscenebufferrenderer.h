/****************************************************************************
**
** Copyright (C) 2012-2016 Andre Beckedorf, Meteora Softworks
** Contact: info@meteorasoftworks.com
**
** This file is part of Viridity
**
** $VIRIDITY_BEGIN_LICENSE:COMMERCIAL_AGPL$
**
** This library is licensed under either a separately available commercial
** license or the GNU Affero General Public License Version 3.0,
** published 19 November 2007.
**
** See https://www.gnu.org/licenses/agpl-3.0.html or LICENSE-agpl-3.0.txt for
** details.
**
** If you wish to use and distribute the Viridity library in your commercial
** product without making your sourcecode available to the public, please
** contact us for a commercial license at info@meteorasoftworks.com
**
** $VIRIDITY_END_LICENSE$
**
****************************************************************************/

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

    void setTargetGraphicsSceneAdapter(AbstractGraphicsSceneAdapter *adapter);
    AbstractGraphicsSceneAdapter *targetGraphicsSceneAdapter() const { return adapter_; }

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
    AbstractGraphicsSceneAdapter *adapter_;

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
