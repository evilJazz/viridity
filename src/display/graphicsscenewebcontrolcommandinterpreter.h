#ifndef GRAPHICSSCENEWEBCONTROLCOMMANDINTERPRETER_H
#define GRAPHICSSCENEWEBCONTROLCOMMANDINTERPRETER_H

#include "viridity_global.h"

#include <QObject>
#include <QEvent>

#include "viriditysessionmanager.h"

class GraphicsSceneAdapter;

class GraphicsSceneWebControlCommandInterpreter : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
public:
    explicit GraphicsSceneWebControlCommandInterpreter(QObject *parent = 0);
    virtual ~GraphicsSceneWebControlCommandInterpreter();

    void setTargetGraphicsSceneAdapter(GraphicsSceneAdapter *adapter);
    GraphicsSceneAdapter *targetGraphicsSceneAdapter() const { return adapter_; }

protected:
    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    Q_INVOKABLE virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

private:
    GraphicsSceneAdapter *adapter_;

    bool keyDownKeyCodeHandled_;
    int keyDownKeyCode_;

    Qt::KeyboardModifier parseParamKeyboardModifiers(const QStringList &params, int index);
    Qt::MouseButton parseParamMouseButton(const QStringList &params, int index);
    QPoint parseParamPoint(const QStringList &params, int startIndex);

    bool handleMouseEvent(const QString &command, const QStringList &params);
    bool handleKeyEvent(const QString &command, const QStringList &params);
    QString textForKey(int key, Qt::KeyboardModifier modifiers);
};

#endif // GRAPHICSSCENEWEBCONTROLCOMMANDINTERPRETER_H
