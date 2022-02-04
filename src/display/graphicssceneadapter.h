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

    void showInputMethod();
    void hideInputMethod();
};

#endif // GRAPHICSSCENEADAPTER_H
