#ifndef GRAPHICSSCENEWEBCONTROLCOMMANDINTERPRETER_H
#define GRAPHICSSCENEWEBCONTROLCOMMANDINTERPRETER_H

#include "viridity_global.h"

#include <QObject>
#include <QEvent>
#include <QGraphicsScene>

class GraphicsSceneWebControlCommandInterpreter : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneWebControlCommandInterpreter(QObject *parent = 0);
    virtual ~GraphicsSceneWebControlCommandInterpreter();

    void setTargetGraphicsScene(QGraphicsScene *scene);
    QGraphicsScene *targetGraphicsScene() const { return scene_; }

    bool sendCommand(const QString &command, const QStringList &params);

signals:
    void fullUpdateRequested();

private:
    QGraphicsScene *scene_;

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
};

#endif // GRAPHICSSCENEWEBCONTROLCOMMANDINTERPRETER_H
