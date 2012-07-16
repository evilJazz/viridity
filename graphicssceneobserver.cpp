#include "graphicssceneobserver.h"

#undef DEBUG
#include "debug.h"

GraphicsSceneObserver::GraphicsSceneObserver(QObject *parent) :
    QObject(parent),
    enabled_(false),
    scene_(NULL)
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

            disconnect(scene_, SIGNAL(changed(QList<QRectF>)));
            disconnect(scene_, SIGNAL(sceneRectChanged(QRectF)));

            sceneDetached();
        }

        enabled_ = value;

        if (enabled_)
        {
            sceneAttached();

            connect(scene_, SIGNAL(changed(QList<QRectF>)), this, SLOT(sceneChanged(QList<QRectF>)));
            connect(scene_, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(sceneSceneRectChanged(QRectF)));
        }
    }
}

void GraphicsSceneObserver::sceneAttached()
{
    DGUARDMETHODTIMED;
}

void GraphicsSceneObserver::sceneChanged(const QList<QRectF> &rects)
{
    DGUARDMETHODTIMED;
}

void GraphicsSceneObserver::sceneSceneRectChanged(const QRectF &newRect)
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
