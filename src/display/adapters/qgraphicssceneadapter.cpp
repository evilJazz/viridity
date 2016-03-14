#include "qgraphicssceneadapter.h"

#include "private/synchronizedscenerenderer.h"
#include "private/synchronizedscenechangedhandler.h"

#include <QRectF>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QCoreApplication>
#include <QKeyEvent>

// Hacker class to get access to private QEvent::spont member. Not nice, but better than pointer hackery.
class QCoreApplicationPrivate
{
public:
    static void setEventSpontaneous(QEvent *event)
    {
        event->spont = true;
    }
};

/* QGraphicsSceneAdapter */

QGraphicsSceneAdapter::QGraphicsSceneAdapter(QGraphicsScene *scene) :
    GraphicsSceneAdapter(scene),
    scene_(scene),
    buttonDown_(false)
{
    sceneChangedHandler_ = new SynchronizedSceneChangedHandler(scene_);
    connect(sceneChangedHandler_, SIGNAL(newUpdateAvailable(QList<QRectF>)), this, SIGNAL(sceneChanged(QList<QRectF>)));
}

int QGraphicsSceneAdapter::width() const
{
    return scene_->width();
}

int QGraphicsSceneAdapter::height() const
{
    return scene_->height();
}

void QGraphicsSceneAdapter::postEvent(QEvent *event, bool spontaneous)
{
    if (spontaneous)
        QCoreApplicationPrivate::setEventSpontaneous(event);

    QCoreApplication::postEvent(scene_, event);
}

void QGraphicsSceneAdapter::postEvent(QEvent::Type eventType, bool spontaneous)
{
    QEvent *event = new QEvent(eventType);
    postEvent(event, spontaneous);
}

void QGraphicsSceneAdapter::handleKeyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers, const QString &text)
{
    QKeyEvent *ke = new QKeyEvent(type, key, modifiers, text);
    postEvent(ke, true);
}

void QGraphicsSceneAdapter::handleMouseEnterEvent(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    if (!scene_->isActive())
        postEvent(QEvent::WindowActivate);

    postEvent(QEvent::Enter, true);

    if (!scene_->hasFocus())
        scene_->setFocus();

    QInputMethodEvent *enterFocus = new QInputMethodEvent();
    postEvent(enterFocus);
}

void QGraphicsSceneAdapter::handleMouseExitEvent(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    //postEvent(QEvent::Leave, true);
    //scene_->clearFocus();
    //postEvent(QEvent::WindowDeactivate, false);
}

void QGraphicsSceneAdapter::handleMouseEvent(QEvent::Type type, const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    QPoint screenPos(scenePos.toPoint());

    // Translate mouse event to GraphicsSceneMouse* events...
    if (type == QEvent::MouseMove)
    {
        type = QEvent::GraphicsSceneMouseMove;

        if (buttonDown_)
            buttons = button = lastButton_;
        else
            buttons = button = Qt::NoButton;
    }
    else if (type == QEvent::MouseButtonPress)
    {
        type = QEvent::GraphicsSceneMousePress;
        lastButtonDownScenePos_ = scenePos;
        lastButtonDownScreenPos_ = screenPos;
        lastButton_ = button;
        buttonDown_ = true;
    }
    else if (type == QEvent::MouseButtonRelease)
    {
        type = QEvent::GraphicsSceneMouseRelease;
        buttonDown_ = false;
    }
    else if (type == QEvent::MouseButtonDblClick)
    {
        type = QEvent::GraphicsSceneMouseDoubleClick;
        buttonDown_ = false;
    }
    else
    {
        buttonDown_ = false;
        return;
    }

    QGraphicsSceneMouseEvent *me = new QGraphicsSceneMouseEvent(type);
    me->setButtonDownScenePos(button, lastButtonDownScenePos_);
    me->setButtonDownScreenPos(button, lastButtonDownScreenPos_);

    me->setScenePos(scenePos);
    me->setScreenPos(screenPos);

    me->setLastScenePos(lastScenePos_);
    me->setLastScreenPos(lastScreenPos_);

    lastScenePos_ = scenePos;
    lastScreenPos_ = screenPos;

    me->setButtons(buttons);
    me->setButton(button);
    me->setModifiers(modifiers);

    me->setAccepted(false);
    postEvent(me, true);
}

void QGraphicsSceneAdapter::handleMouseWheelEvent(const QPoint delta, const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    QPoint screenPos(scenePos.toPoint());

    QGraphicsSceneWheelEvent *we = new QGraphicsSceneWheelEvent(QEvent::GraphicsSceneWheel);
    we->setWidget(NULL);
    we->setScenePos(scenePos);
    we->setScreenPos(screenPos);
    we->setButtons(buttons);
    we->setModifiers(modifiers);

    if (delta.y() != 0)
    {
        we->setDelta(delta.y() * 10);
        we->setOrientation(Qt::Vertical);
    }
    else
    {
        we->setDelta(delta.x() * 10);
        we->setOrientation(Qt::Horizontal);
    }

    we->setAccepted(false);
    postEvent(we, true);
}

void QGraphicsSceneAdapter::handleContextMenuEvent(const QPointF &scenePos, Qt::KeyboardModifiers modifiers, QContextMenuEvent::Reason reason)
{
    QPoint screenPos(scenePos.toPoint());

    QGraphicsSceneContextMenuEvent *cme = new QGraphicsSceneContextMenuEvent(QEvent::GraphicsSceneContextMenu);

    cme->setWidget(NULL);
    cme->setScenePos(scenePos);
    cme->setScreenPos(screenPos);
    cme->setModifiers(modifiers);
    cme->setReason(static_cast<QGraphicsSceneContextMenuEvent::Reason>(reason));

    cme->setAccepted(false);
    postEvent(cme, true);
}

void QGraphicsSceneAdapter::render(QPainter *painter, const QRectF &target, const QRectF &source, Qt::AspectRatioMode aspectRatioMode)
{
    SynchronizedSceneRenderer syncedSceneRenderer(scene_);
    syncedSceneRenderer.render(painter, target, source, aspectRatioMode);
}

void QGraphicsSceneAdapter::render(QPainter *painter, const QVector<QRect> &rects)
{
    SynchronizedSceneRenderer syncedSceneRenderer(scene_);
    syncedSceneRenderer.render(painter, rects);
}
