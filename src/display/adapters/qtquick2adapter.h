#ifndef QTQUICK2ADAPTER_H
#define QTQUICK2ADAPTER_H

#include "graphicssceneadapter.h"

#include <QImage>

#include <QEvent>

class QQuickItem;
class QOpenGLContext;
class QOffscreenSurface;
class QOpenGLFramebufferObject;
class QQuickRenderControl;
class QQuickWindow;

class QtQuick2Adapter : public GraphicsSceneAdapter
{
    Q_OBJECT
public:
    QtQuick2Adapter(QQuickItem *rootItem);
    virtual ~QtQuick2Adapter();

    int width() const;
    int height() const;

    void setSize(int width, int height, qreal ratio);

    void handleKeyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers, const QString& text = QString());
    void handleMouseEnterEvent(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleMouseExitEvent(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleMouseEvent(QEvent::Type type, const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleMouseWheelEvent(const QPoint delta, const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleContextMenuEvent(const QPointF &scenePos, Qt::KeyboardModifiers modifiers, QContextMenuEvent::Reason reason);

    void render(QPainter *painter, const QRectF &target = QRectF(), const QRectF &source = QRectF(), Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio);
    void render(QPainter *painter, const QVector<QRect> &rects);

private slots:
    void createFbo();
    void destroyFbo();
    void handleSceneChanged();
    void updateBuffer();

private:
    QQuickItem *rootItem_;
    QOpenGLContext *context_;
    QOffscreenSurface *offscreenSurface_;
    QOpenGLFramebufferObject *fbo_;
    QQuickRenderControl *renderControl_;
    QQuickWindow *quickWindow_;

    QImage buffer_;
    bool updateRequired_;

    bool buttonDown_;
    Qt::MouseButton lastButton_;

    QPointF lastButtonDownScenePos_;
    QPoint lastButtonDownScreenPos_;

    QPointF lastScenePos_;
    QPoint lastScreenPos_;

    void ensureBufferUpdated();

    void postEvent(QEvent::Type eventType, bool spontaneous = false);
    void postEvent(QEvent *event, bool spontaneous = false);
};

#endif // QTQUICK2ADAPTER_H