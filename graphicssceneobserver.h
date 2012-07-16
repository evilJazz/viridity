#ifndef GRAPHICSSCENEOBSERVER_H
#define GRAPHICSSCENEOBSERVER_H

#include <QObject>
#include <QGraphicsScene>

class GraphicsSceneObserver : public QObject
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
    virtual void sceneChanged(const QList<QRectF> &rects);
    virtual void sceneSceneRectChanged(const QRectF &newRect);
    virtual void sceneDetaching();
    virtual void sceneDetached();

protected:
    bool enabled_;
    QGraphicsScene *scene_;
};

#endif // GRAPHICSSCENEOBSERVER_H
