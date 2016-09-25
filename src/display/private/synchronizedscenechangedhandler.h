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
    SynchronizedSceneChangedHandler(QGraphicsScene *scene, QObject *parent) :
        QObject(parent),
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
