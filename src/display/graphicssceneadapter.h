#ifndef GRAPHICSSCENEADAPTER_H
#define GRAPHICSSCENEADAPTER_H

#include <QObject>
#include <QRectF>
#include <QEvent>
#include <QContextMenuEvent>

class QPainter;

/* AbstractGraphicsSceneAdapter */

class AbstractGraphicsSceneAdapter : public QObject
{
    Q_OBJECT
public:
    AbstractGraphicsSceneAdapter(QObject *parent) : QObject(parent) {  qRegisterMetaType< QList<QRectF> >("QList<QRectF>"); }
    virtual ~AbstractGraphicsSceneAdapter() {}

    virtual int width() const = 0;
    virtual int height() const = 0;

    virtual void setSize(int width, int height, qreal ratio) {}

    virtual void handleKeyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers, const QString& text = QString()) = 0;
    virtual void handleMouseEnterEvent(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers) = 0;
    virtual void handleMouseExitEvent(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers) = 0;
    virtual void handleMouseEvent(QEvent::Type type, const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers) = 0;
    virtual void handleMouseWheelEvent(const QPoint delta, const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers) = 0;
    virtual void handleContextMenuEvent(const QPointF &scenePos, Qt::KeyboardModifiers modifiers, QContextMenuEvent::Reason reason) = 0;

    virtual void render(QPainter *painter, const QRect &rect) = 0;
    virtual void render(QPainter *painter, const QVector<QRect> &rects) = 0;

signals:
    void sceneChanged(QList<QRectF> rects);
};

#endif // GRAPHICSSCENEADAPTER_H
