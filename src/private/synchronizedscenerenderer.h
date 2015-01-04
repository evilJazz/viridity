#ifndef SYNCHRONIZEDSCENERENDERER_H
#define SYNCHRONIZEDSCENERENDERER_H

#include <QObject>
#include <QGraphicsScene>
#include <QPainter>
#include <QThread>
#include <QMetaType>

class SynchronizedSceneRenderer : public QObject
{
    Q_OBJECT
public:
    SynchronizedSceneRenderer(QGraphicsScene *scene) :
        scene_(scene)
    {
        qRegisterMetaType<QPainter *>("QPainter *");
        qRegisterMetaType<Qt::AspectRatioMode>("Qt::AspectRatioMode");
        qRegisterMetaType< QVector<QRect> >("QVector<QRect>");

        if (scene_->thread() != QThread::currentThread())
            this->moveToThread(scene_->thread()); // Move to scene's thread so we can invoke renderInSceneThreadContext in proper thread context...
    }

    void render(QPainter *painter, const QRectF &target = QRectF(), const QRectF &source = QRectF(), Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio)
    {
        metaObject()->invokeMethod(
            this, "renderInSceneThreadContext",
            scene_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
            Q_ARG(QPainter *, painter),
            Q_ARG(const QRectF &, target),
            Q_ARG(const QRectF &, source),
            Q_ARG(Qt::AspectRatioMode, aspectRatioMode)
        );
    }

    void render(QPainter *painter, const QVector<QRect> &rects)
    {
        metaObject()->invokeMethod(
            this, "renderInSceneThreadContext",
            scene_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
            Q_ARG(QPainter *, painter),
            Q_ARG(const QVector<QRect> &, rects)
        );
    }

private slots:
    void renderInSceneThreadContext(QPainter *painter, const QRectF &target = QRectF(), const QRectF &source = QRectF(), Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio)
    {
        scene_->render(painter, target, source, aspectRatioMode);
    }

    void renderInSceneThreadContext(QPainter *painter, const QVector<QRect> &rects)
    {
        foreach (const QRect &rect, rects)
            scene_->render(painter, rect, rect, Qt::IgnoreAspectRatio);
    }

private:
    QGraphicsScene *scene_;
};

#endif // SYNCHRONIZEDSCENERENDERER_H
