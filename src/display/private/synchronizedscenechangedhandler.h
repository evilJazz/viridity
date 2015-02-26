#ifndef SYNCHRONIZEDSCENECHANGEDHANDLER_H
#define SYNCHRONIZEDSCENECHANGEDHANDLER_H

#include <QObject>
#include <QGraphicsScene>
#include <QThread>
#include <QMetaType>

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

class SynchronizedSceneChangedHandler : public QObject
{
    Q_OBJECT
public:
    SynchronizedSceneChangedHandler(QGraphicsScene *scene) :
        scene_(scene)
    {
        DGUARDMETHODTIMED;
        qRegisterMetaType< QList<QRectF> >("QList<QRectF>");

        if (QThread::currentThread() != scene_->thread())
            this->moveToThread(scene_->thread());

        connect(scene_, SIGNAL(changed(QList<QRectF>)), this, SLOT(sceneChanged(QList<QRectF>)));
    }

    virtual ~SynchronizedSceneChangedHandler()
    {
        DGUARDMETHODTIMED;
    }

signals:
    void newUpdateAvailable(QList<QRectF> rects);

private slots:
    void sceneChanged(QList<QRectF> rects)
    {
        DGUARDMETHODTIMED;
        QList<QRectF> newRects = rects;
        newRects.detach(); // IMPORTANT, we need a deep copy for the thread...

        emit newUpdateAvailable(newRects);
    }

private:
    QGraphicsScene *scene_;
};

#endif // SYNCHRONIZEDSCENECHANGEDHANDLER_H
