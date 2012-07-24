#include "graphicssceneobserver.h"

#include <QMetaType>

#undef DEBUG
#include "debug.h"

GraphicsSceneObserver::GraphicsSceneObserver(QObject *parent) :
    QObject(parent),
    enabled_(false),
    scene_(NULL),
    sceneChangedHandler_(NULL)
{
}

GraphicsSceneObserver::~GraphicsSceneObserver()
{
    setEnabled(false);
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

            //disconnect(scene_, SIGNAL(changed(QList<QRectF>)));
            disconnect(scene_, SIGNAL(sceneRectChanged(QRectF)));

            sceneDetached();
        }

        enabled_ = value;

        if (enabled_)
        {
            sceneAttached();

            //connect(scene_, SIGNAL(changed(QList<QRectF>)), this, SLOT(sceneChanged(QList<QRectF>)));

            if (!sceneChangedHandler_)
            {
                sceneChangedHandler_ = new SynchronizedSceneChangedHandler(scene_);
                connect(sceneChangedHandler_, SIGNAL(newUpdateAvailable(QList<QRectF>)), this, SLOT(sceneChanged(QList<QRectF>)));
            }

            connect(scene_, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(sceneSceneRectChanged(QRectF)));
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

void GraphicsSceneObserver::sceneSceneRectChanged(QRectF newRect)
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
