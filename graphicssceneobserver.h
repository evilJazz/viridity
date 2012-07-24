#ifndef GRAPHICSSCENEOBSERVER_H
#define GRAPHICSSCENEOBSERVER_H

#include <QObject>
#include <QList>
#include <QGraphicsScene>
#include <QThread>
#include <QMetaType>

class SynchronizedSceneChangedHandler : public QObject
{
    Q_OBJECT
public:
    SynchronizedSceneChangedHandler(QGraphicsScene *scene) :
        scene_(scene)
    {
        qRegisterMetaType< QList<QRectF> >("QList<QRectF>");

        if (scene_->thread() != QThread::currentThread())
            this->moveToThread(scene_->thread());

        connect(scene_, SIGNAL(changed(QList<QRectF>)), this, SLOT(sceneChanged(QList<QRectF>)));
    }

    virtual ~SynchronizedSceneChangedHandler()
    {
        disconnect(scene_, SIGNAL(changed(QList<QRectF>)));
    }

signals:
    void newUpdateAvailable(QList<QRectF> rects);

private slots:
    void sceneChanged(QList<QRectF> rects)
    {
        QList<QRectF> newRects = rects;
        newRects.detach(); // IMPORTANT, we need a deep copy for the thread...

        emit newUpdateAvailable(newRects);
    }

private:
    QGraphicsScene *scene_;
};

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
    virtual void sceneChanged(QList<QRectF> rects);
    virtual void sceneSceneRectChanged(QRectF newRect);
    virtual void sceneDetaching();
    virtual void sceneDetached();

protected:
    bool enabled_;
    QGraphicsScene *scene_;
    SynchronizedSceneChangedHandler *sceneChangedHandler_;
};

#endif // GRAPHICSSCENEOBSERVER_H
