#ifndef GRAPHICSSCENEWEBCONTROLCOMMANDINTERPRETER_H
#define GRAPHICSSCENEWEBCONTROLCOMMANDINTERPRETER_H

#include "viridity_global.h"

#include <QObject>
#include <QEvent>
#include <QGraphicsScene>

class MessageHandler
{
public:
    virtual ~MessageHandler() {}
    virtual bool canHandleMessage(const QByteArray &message, const QString &displayId) = 0;
    virtual bool handleMessage(const QByteArray &message, const QString &displayId) = 0;

    static void splitMessage(const QByteArray &message, QString &command, QStringList &params);
};

class GraphicsSceneWebControlCommandInterpreter : public QObject, public MessageHandler
{
    Q_OBJECT
public:
    explicit GraphicsSceneWebControlCommandInterpreter(QObject *parent = 0);
    virtual ~GraphicsSceneWebControlCommandInterpreter();

    void setTargetGraphicsScene(QGraphicsScene *scene);
    QGraphicsScene *targetGraphicsScene() const { return scene_; }

    Q_INVOKABLE bool dispatchMessage(const QByteArray &message, const QString &displayId = QString::null);

    void registerHandler(MessageHandler *handler);
    void registerHandlers(const QList<MessageHandler *> &handlers);
    void unregisterHandler(MessageHandler *handler);

protected:
    // MessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &displayId);
    virtual bool handleMessage(const QByteArray &message, const QString &displayId);

private:
    QGraphicsScene *scene_;

    QList<MessageHandler *> handlers_;

    bool buttonDown_;
    Qt::MouseButton lastButton_;

    QPointF lastButtonDownScenePos_;
    QPoint lastButtonDownScreenPos_;

    QPointF lastScenePos_;
    QPoint lastScreenPos_;

    bool keyDownKeyCodeHandled_;
    int keyDownKeyCode_;

    void postEvent(QEvent *event, bool spontaneous = false);
    void postEvent(QEvent::Type eventType, bool spontaneous = false);

    Qt::KeyboardModifier parseParamKeyboardModifiers(const QStringList &params, int index);
    Qt::MouseButton parseParamMouseButton(const QStringList &params, int index);
    QPoint parseParamPoint(const QStringList &params, int startIndex);

    bool handleMouseEnter(const QString &command, const QStringList &params);
    bool handleMouseExit(const QString &command, const QStringList &params);
    bool handleMouseEvent(const QString &command, const QStringList &params);
    bool handleKeyEvent(const QString &command, const QStringList &params);
    QString textForKey(int key, Qt::KeyboardModifier modifiers);

    void resizeScene(const QStringList &params);
};

#endif // GRAPHICSSCENEWEBCONTROLCOMMANDINTERPRETER_H
