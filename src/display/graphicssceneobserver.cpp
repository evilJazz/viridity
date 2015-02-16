#include "graphicssceneobserver.h"

#include <QMetaType>

#include "private/synchronizedscenechangedhandler.h"

//#undef DEBUG
#include "KCL/debug.h"

GraphicsSceneObserver::GraphicsSceneObserver(QObject *parent) :
    QObject(parent),
    enabled_(false),
    scene_(NULL),
    sceneChangedHandler_(NULL)
{
    DGUARDMETHODTIMED;
}

GraphicsSceneObserver::~GraphicsSceneObserver()
{
    DGUARDMETHODTIMED;
}

void GraphicsSceneObserver::setTargetGraphicsScene(QGraphicsScene *scene)
{
    if (scene != scene_)
    {
        bool wasEnabled = enabled();
        setEnabled(false);

        scene_ = scene;

        setEnabled(wasEnabled);
    }
}

void GraphicsSceneObserver::setEnabled(bool value)
{
    if (value != enabled_)
    {
        if (enabled_ && scene_)
        {
            sceneDetaching();

            if (sceneChangedHandler_)
            {
                disconnect(sceneChangedHandler_, SIGNAL(newUpdateAvailable(QList<QRectF>)));
                delete sceneChangedHandler_;
                sceneChangedHandler_ = NULL;
            }

            disconnect(scene_, SIGNAL(destroyed()));

            sceneDetached();
        }

        enabled_ = value;

        if (enabled_ && scene_)
        {
            sceneAttached();

            //connect(scene_, SIGNAL(changed(QList<QRectF>)), this, SLOT(sceneChanged(QList<QRectF>)));

            if (!sceneChangedHandler_)
            {
                sceneChangedHandler_ = new SynchronizedSceneChangedHandler(scene_);
                connect(sceneChangedHandler_, SIGNAL(newUpdateAvailable(QList<QRectF>)), this, SLOT(sceneChanged(QList<QRectF>)));
            }

            connect(scene_, SIGNAL(destroyed()), this, SLOT(sceneDestroyed()));
        }
    }
}

void GraphicsSceneObserver::sceneAttached()
{
    DGUARDMETHODTIMED;
}

void GraphicsSceneObserver::sceneChanged(QList<QRectF> rects)
{
    DGUARDMETHODTIMED;
}

void GraphicsSceneObserver::sceneDetaching()
{
    DGUARDMETHODTIMED;
}

void GraphicsSceneObserver::sceneDetached()
{
    DGUARDMETHODTIMED;
}

void GraphicsSceneObserver::sceneDestroyed()
{
    DGUARDMETHODTIMED;
    scene_ = NULL;
    sceneDetached();
    setEnabled(false);
}
