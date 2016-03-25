#ifndef GRAPHICSSCENEDISPLAYCOMMANDINTERPRETER_H
#define GRAPHICSSCENEDISPLAYCOMMANDINTERPRETER_H

#include "viridity_global.h"

#include <QObject>
#include <QEvent>
#include <QPoint>

#include "viriditysessionmanager.h"

class AbstractGraphicsSceneAdapter;

class GraphicsSceneDisplayCommandInterpreter : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
public:
    explicit GraphicsSceneDisplayCommandInterpreter(QObject *parent = 0);
    virtual ~GraphicsSceneDisplayCommandInterpreter();

    void setTargetGraphicsSceneAdapter(AbstractGraphicsSceneAdapter *adapter);
    AbstractGraphicsSceneAdapter *targetGraphicsSceneAdapter() const { return adapter_; }

protected:
    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    Q_INVOKABLE virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

private:
    AbstractGraphicsSceneAdapter *adapter_;

    bool keyDownKeyCodeHandled_;
    int keyDownKeyCode_;

    Qt::KeyboardModifier parseParamKeyboardModifiers(const QStringList &params, int index);
    Qt::MouseButton parseParamMouseButton(const QStringList &params, int index);
    QPoint parseParamPoint(const QStringList &params, int startIndex);

    bool handleMouseEvent(const QString &command, const QStringList &params);
    bool handleKeyEvent(const QString &command, const QStringList &params);
    QString textForKey(int key, Qt::KeyboardModifier modifiers);
};

#endif // GRAPHICSSCENEDISPLAYCOMMANDINTERPRETER_H
