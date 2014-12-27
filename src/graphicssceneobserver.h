#ifndef GRAPHICSSCENEOBSERVER_H
#define GRAPHICSSCENEOBSERVER_H

#include "viridity_global.h"

#include <QObject>
#include <QList>
#include <QRectF>

class QGraphicsScene;
class SynchronizedSceneChangedHandler;

class VIRIDITY_EXPORT GraphicsSceneObserver : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneObserver(QObject *parent = 0);
    virtual ~GraphicsSceneObserver();

    void setTargetGraphicsScene(QGraphicsScene *scene);
    QGraphicsScene *targetGraphicsScene() const { return scene_; }

    void setEnabled(bool value);
    bool enabled() const { return enabled_; }

protected slots:
    virtual void sceneAttached();
    virtual void sceneChanged(QList<QRectF> rects);
    virtual void sceneSceneRectChanged(QRectF newRect);
    virtual void sceneDetaching();
    virtual void sceneDetached();
    virtual void sceneDestroyed();

protected:
    bool enabled_;
    QGraphicsScene *scene_;
    SynchronizedSceneChangedHandler *sceneChangedHandler_;
};

#endif // GRAPHICSSCENEOBSERVER_H
