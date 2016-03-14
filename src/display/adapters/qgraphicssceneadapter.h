#ifndef QGRAPHICSSCENEADAPTER_H
#define QGRAPHICSSCENEADAPTER_H

#include <graphicssceneadapter.h>

class QGraphicsScene;
class SynchronizedSceneChangedHandler;

class QGraphicsSceneAdapter : public GraphicsSceneAdapter
{
    Q_OBJECT
public:
    QGraphicsSceneAdapter(QGraphicsScene *scene);

    int width() const;
    int height() const;

    void handleKeyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers, const QString& text = QString());
    void handleMouseEnterEvent(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleMouseExitEvent(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleMouseEvent(QEvent::Type type, const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleMouseWheelEvent(const QPoint delta, const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleContextMenuEvent(const QPointF &scenePos, Qt::KeyboardModifiers modifiers, QContextMenuEvent::Reason reason);

    void render(QPainter *painter, const QRectF &target = QRectF(), const QRectF &source = QRectF(), Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio);
    void render(QPainter *painter, const QVector<QRect> &rects);

private:
    QGraphicsScene *scene_;
    SynchronizedSceneChangedHandler *sceneChangedHandler_;

    bool buttonDown_;
    Qt::MouseButton lastButton_;

    QPointF lastButtonDownScenePos_;
    QPoint lastButtonDownScreenPos_;

    QPointF lastScenePos_;
    QPoint lastScreenPos_;

    void postEvent(QEvent *event, bool spontaneous = false);
    void postEvent(QEvent::Type eventType, bool spontaneous = false);
};

#endif // QGRAPHICSSCENEADAPTER_H
