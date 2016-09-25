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
        qRegisterMetaType< QVector<QRect> >("QVector<QRect>");

        if (scene_->thread() != QThread::currentThread())
            this->moveToThread(scene_->thread()); // Move to scene's thread so we can invoke renderInSceneThreadContext in proper thread context...
    }

    void render(QPainter *painter, const QRect &rect)
    {
        QMetaObject::invokeMethod(
            this, "renderInSceneThreadContext",
            scene_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
            Q_ARG(QPainter *, painter),
            Q_ARG(const QRect &, rect)
        );
    }

    void render(QPainter *painter, const QVector<QRect> &rects)
    {
        QMetaObject::invokeMethod(
            this, "renderInSceneThreadContext",
            scene_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
            Q_ARG(QPainter *, painter),
            Q_ARG(const QVector<QRect> &, rects)
        );
    }

private slots:
    void renderInSceneThreadContext(QPainter *painter, const QRect &rect)
    {
        scene_->render(painter, rect, rect, Qt::IgnoreAspectRatio);
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
