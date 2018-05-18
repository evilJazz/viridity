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

#ifndef QTQUICK2ADAPTER_H
#define QTQUICK2ADAPTER_H

#include "graphicssceneadapter.h"

#include <QImage>

#include <QEvent>

class QQuickItem;
class QQuickWindow;
class QOpenGLContext;
class QOffscreenSurface;
class QOpenGLFramebufferObject;
class QQuickRenderControl;
class QQuickWindow;

class QtQuick2Adapter : public AbstractGraphicsSceneAdapter
{
    Q_OBJECT
public:
    QtQuick2Adapter(QQuickItem *rootItem);
    QtQuick2Adapter(QQuickWindow *window);
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

    void render(QPainter *painter, const QRect &rect);
    void render(QPainter *painter, const QVector<QRect> &rects);

    QQuickItem *rootItem() const { return rootItem_; }

private slots:
    void detachFromRootItem();

    void createFbo();
    void destroyFbo();
    void handleSceneChanged();
    void handleRootItemDestroyed();

    void updateBuffer();

private:
    QQuickItem *rootItemPreviousParent_;
    QQuickItem *rootItem_;
    QQuickWindow *window_;
    QOpenGLContext *context_;
    QOffscreenSurface *offscreenSurface_;
    qreal dpr_;
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

    bool killedAlready_;

    void init();

    void ensureBufferUpdated();

    void postEvent(QEvent::Type eventType, bool spontaneous = false);
    void postEvent(QEvent *event, bool spontaneous = false);
};

#endif // QTQUICK2ADAPTER_H
