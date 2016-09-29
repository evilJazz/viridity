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

#include "qtquick2adapter.h"

#include <QQuickItem>
#include <QQuickWindow>
#include <QPainter>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOffscreenSurface>

#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickRenderControl>

#include <QInputMethodEvent>

#include <QThread>

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

/*! /internal Hacker class to get access to private QEvent::spont member. Not nice, but better than pointer hackery. */
class QCoreApplicationPrivate
{
public:
    static void setEventSpontaneous(QEvent *event)
    {
        event->spont = true;
    }
};

/* QtQuick2Adapter */

QtQuick2Adapter::QtQuick2Adapter(QQuickItem *rootItem) :
    AbstractGraphicsSceneAdapter(NULL),
    rootItemPreviousParent_(rootItem->parentItem()),
    rootItem_(rootItem),
    window_(NULL),
    fbo_(0),
    updateRequired_(true),
    buttonDown_(false)
{
    DGUARDMETHODTIMED;

    if (!rootItem_)
        qFatal("Root item is not assigned.");

    init();
}

QtQuick2Adapter::QtQuick2Adapter(QQuickWindow *window) :
    AbstractGraphicsSceneAdapter(NULL),
    rootItem_(NULL),
    window_(window),
    fbo_(0),
    updateRequired_(true),
    buttonDown_(false)
{
    DGUARDMETHODTIMED;

    if (!window_)
        qFatal("Window is not assigned.");

    rootItem_ = window_->contentItem();
    rootItemPreviousParent_ = rootItem_->parentItem();
    window_->hide();

    init();
}

void QtQuick2Adapter::init()
{
    QSurfaceFormat format;
    // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);

    context_ = new QOpenGLContext;
    context_->setFormat(format);
    context_->create();

    offscreenSurface_ = new QOffscreenSurface;
    // Pass context_->format(), not format. Format does not specify and color buffer
    // sizes, while the context, that has just been created, reports a format that has
    // these values filled in. Pass this to the offscreen surface to make sure it will be
    // compatible with the context's configuration.
    offscreenSurface_->setFormat(context_->format());
    offscreenSurface_->create();

    renderControl_ = new QQuickRenderControl(this);

    // Create a QQuickWindow that is associated with our render control. Note that this
    // window never gets created or shown, meaning that it will never get an underlying
    // native (platform) window.
    quickWindow_ = new QQuickWindow(renderControl_);
    quickWindow_->setObjectName("RenderControlWindow");

    // Now hook up the signals. For simplicy we don't differentiate between
    // renderRequested (only render is needed, no sync) and sceneChanged (polish and sync is needed too).
    connect(quickWindow_, &QQuickWindow::sceneGraphInitialized, this, &QtQuick2Adapter::createFbo);
    connect(quickWindow_, &QQuickWindow::sceneGraphInvalidated, this, &QtQuick2Adapter::destroyFbo);
    connect(renderControl_, &QQuickRenderControl::renderRequested, this, &QtQuick2Adapter::handleSceneChanged);
    connect(renderControl_, &QQuickRenderControl::sceneChanged, this, &QtQuick2Adapter::handleSceneChanged);

    // Parent the root item to the window's content item.
    rootItem_->setParentItem(quickWindow_->contentItem());
    connect(rootItem_, &QQuickItem::destroyed, this, &QtQuick2Adapter::detachFromRootItem);

    // Update item and rendering related geometries.
    setSize(rootItem_->width(), rootItem_->height(), 1.f);

    // Initialize the render control and our OpenGL resources.
    context_->makeCurrent(offscreenSurface_);
    renderControl_->initialize(context_);
}

QtQuick2Adapter::~QtQuick2Adapter()
{
    DGUARDMETHODTIMED;

    detachFromRootItem();

    destroyFbo();

    context_->doneCurrent();
    delete offscreenSurface_;

    delete context_;
}

void QtQuick2Adapter::detachFromRootItem()
{
    DGUARDMETHODTIMED;

    if (rootItem_)
    {
        disconnect(rootItem_, &QQuickItem::destroyed, this, &QtQuick2Adapter::detachFromRootItem);

        rootItem_->setParentItem(rootItemPreviousParent_);

        rootItem_ = NULL;
        window_ = NULL;
    }

    // Make sure the context is current while doing cleanup. Note that we use the
    // offscreen surface here because passing 'this' at this point is not safe: the
    // underlying platform window may already be destroyed. To avoid all the trouble, use
    // another surface that is valid for sure.
    context_->makeCurrent(offscreenSurface_);

    // Delete the render control first since it will free the scenegraph resources.
    // Destroy the QQuickWindow only afterwards.
    if (renderControl_)
    {
        delete renderControl_;
        renderControl_ = NULL;
    }

    if (quickWindow_)
    {
        delete quickWindow_;
        quickWindow_ = NULL;
    }
}

int QtQuick2Adapter::width() const
{
    return quickWindow_ ? quickWindow_->width() : 0;
}

int QtQuick2Adapter::height() const
{
    return quickWindow_ ? quickWindow_->height() : 0;
}

void QtQuick2Adapter::setSize(int width, int height, qreal ratio)
{
    DGUARDMETHODTIMED;

    if (!rootItem_ || !quickWindow_)
        return;

    dpr_ = ratio;

    if (!qFuzzyCompare(ratio, 1.))
    {
        rootItem_->setWidth(width / ratio);
        rootItem_->setHeight(height / ratio);
    }
    else
    {
        rootItem_->setWidth(width);
        rootItem_->setHeight(height);
    }

    rootItem_->setTransformOrigin(QQuickItem::TopLeft);
    rootItem_->setScale(ratio);

    quickWindow_->setGeometry(0, 0, width, height);

    if (context_->makeCurrent(offscreenSurface_))
    {
        destroyFbo();
        createFbo();
        context_->doneCurrent();
    }

    updateRequired_ = true;
    handleSceneChanged();
}

void QtQuick2Adapter::postEvent(QEvent *event, bool spontaneous)
{
    if (!quickWindow_)
        return;

    if (spontaneous)
        QCoreApplicationPrivate::setEventSpontaneous(event);

    QCoreApplication::postEvent(quickWindow_, event);
}

void QtQuick2Adapter::postEvent(QEvent::Type eventType, bool spontaneous)
{
    QEvent *event = new QEvent(eventType);
    postEvent(event, spontaneous);
}

void QtQuick2Adapter::handleKeyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers, const QString &text)
{
    QKeyEvent *ke = new QKeyEvent(type, key, modifiers, text);
    postEvent(ke, true);
}

void QtQuick2Adapter::handleMouseEnterEvent(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    /*
    postEvent(QEvent::WindowActivate);
    postEvent(QEvent::Enter, true);

    QInputMethodEvent *enterFocus = new QInputMethodEvent();
    postEvent(enterFocus);
    */
}

void QtQuick2Adapter::handleMouseExitEvent(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{

}

void QtQuick2Adapter::handleMouseEvent(QEvent::Type type, const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    if (type == QEvent::MouseMove)
    {
        if (buttonDown_)
            buttons = button = lastButton_;
        else
            buttons = button = Qt::NoButton;
    }
    else if (type == QEvent::MouseButtonPress)
    {
        buttonDown_ = true;
    }
    else if (type == QEvent::MouseButtonRelease)
    {
        buttonDown_ = false;

        button = lastButton_;
        // Set buttons to NoButton to make synthesized Click event work.
        buttons = Qt::NoButton;
    }
    else if (type == QEvent::MouseButtonDblClick)
    {
        // Send DoubleClick event directly.
        postEvent(new QMouseEvent(QEvent::MouseButtonDblClick, scenePos, button, buttons, modifiers), true);

        // Finally change to MouseButtonRelease. This is required to avoid sticky condition.
        type = QEvent::MouseButtonRelease;
        button = Qt::NoButton;
        buttons = Qt::NoButton;

        buttonDown_ = false;
    }
    else
    {
        buttonDown_ = false;
        return;
    }

    lastButton_ = button;

    QMouseEvent *me = new QMouseEvent(type, scenePos, button, buttons, modifiers);
    postEvent(me, true);
}

void QtQuick2Adapter::handleMouseWheelEvent(const QPoint delta, const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    QWheelEvent *we;

    if (delta.y() != 0)
        we = new QWheelEvent(scenePos, delta.y() * 10, buttons, modifiers, Qt::Vertical);
    else
        we = new QWheelEvent(scenePos, delta.x() * 10, buttons, modifiers, Qt::Horizontal);

    postEvent(we, true);
}

void QtQuick2Adapter::handleContextMenuEvent(const QPointF &scenePos, Qt::KeyboardModifiers modifiers, QContextMenuEvent::Reason reason)
{
    QContextMenuEvent *cme = new QContextMenuEvent(reason, scenePos.toPoint(), scenePos.toPoint(), modifiers);
    postEvent(cme, true);
}

void QtQuick2Adapter::render(QPainter *painter, const QRect &rect)
{
    ensureBufferUpdated();
    painter->drawImage(rect.topLeft(), buffer_, rect);
}

void QtQuick2Adapter::render(QPainter *painter, const QVector<QRect> &rects)
{
    ensureBufferUpdated();

    foreach (const QRect &rect, rects)
        painter->drawImage(rect.topLeft(), buffer_, rect);
}

void QtQuick2Adapter::createFbo()
{
    DGUARDMETHODTIMED;
    if (!quickWindow_)
        return;

    destroyFbo();

    fbo_ = new QOpenGLFramebufferObject(quickWindow_->width(), quickWindow_->height(), QOpenGLFramebufferObject::CombinedDepthStencil);
    quickWindow_->setRenderTarget(fbo_);
}

void QtQuick2Adapter::destroyFbo()
{
    DGUARDMETHODTIMED;
    if (fbo_)
    {
        delete fbo_;
        fbo_ = 0;
    }
}

void QtQuick2Adapter::handleSceneChanged()
{
    if (quickWindow_ && rootItem_)
    {
        updateRequired_ = true;

        QList<QRectF> rects;
        rects << QRectF(QPoint(0, 0), quickWindow_->size());
        emit sceneChanged(rects);
    }
}

void QtQuick2Adapter::updateBuffer()
{
    DGUARDMETHODTIMED;

    if (!context_->makeCurrent(offscreenSurface_) || !renderControl_ || !quickWindow_)
        return;

    // Polish, synchronize and render the next frame (into our fbo).  In this example
    // everything happens on the same thread and therefore all three steps are performed
    // in succession from here. In a threaded setup the render() call would happen on a
    // separate thread.
    renderControl_->polishItems();
    renderControl_->sync();
    renderControl_->render();

    buffer_ = fbo_->toImage();

    quickWindow_->resetOpenGLState();
    QOpenGLFramebufferObject::bindDefault();

    context_->functions()->glFlush();

    updateRequired_ = false;
}

void QtQuick2Adapter::ensureBufferUpdated()
{
    DGUARDMETHODTIMED;

    if (updateRequired_)
    {
        QMetaObject::invokeMethod(
            this, "updateBuffer",
            thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection
        );
    }
}
