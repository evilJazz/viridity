#include "graphicsscenewebcontrolcommandinterpreter.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>

#include <QDeclarativeItem>

#include "KCL/debug.h"
#include "private/qtestspontaneevent.h"


/* GraphicsSceneWebControlCommandInterpreter */

GraphicsSceneWebControlCommandInterpreter::GraphicsSceneWebControlCommandInterpreter(QObject *parent) :
    QObject(parent),
    scene_(NULL),
    buttonDown_(false),
    keyDownKeyCodeHandled_(false)
{
    registerHandler(this);
}

GraphicsSceneWebControlCommandInterpreter::~GraphicsSceneWebControlCommandInterpreter()
{
}

void GraphicsSceneWebControlCommandInterpreter::setTargetGraphicsScene(QGraphicsScene *scene)
{
    scene_ = scene;
}

void GraphicsSceneWebControlCommandInterpreter::postEvent(QEvent *event, bool spontaneous)
{
    if (spontaneous)
        QSpontaneKeyEvent::setSpontaneous(event);

    QApplication::postEvent(scene_, event);

    /*
    if (!QApplication::sendEvent(scene_, &event))
    {
        DPRINTF("Event not accepted...");
        return false;
    }

    return true;
    */
}

void GraphicsSceneWebControlCommandInterpreter::postEvent(QEvent::Type eventType, bool spontaneous)
{
    QEvent *event = new QEvent(eventType);
    postEvent(event, spontaneous);
}

Qt::KeyboardModifier GraphicsSceneWebControlCommandInterpreter::parseParamKeyboardModifiers(const QStringList &params, int index)
{
    if (index < params.count())
    {
        bool converted = false;
        int value = params[index].toInt(&converted);

        if (converted)
            return static_cast<Qt::KeyboardModifier>(value);
    }

    return Qt::NoModifier;
}

Qt::MouseButton GraphicsSceneWebControlCommandInterpreter::parseParamMouseButton(const QStringList &params, int index)
{
    if (index < params.count())
    {
        bool converted = false;
        int paramButton = params[index].toInt(&converted);

        if (converted)
        {
            switch (paramButton)
            {
            case 1:
                return Qt::LeftButton;
                break;
            case 2:
                return Qt::MiddleButton;
                break;
            case 3:
                return Qt::RightButton;
                break;
            default:
                return Qt::NoButton;
            }
        }
    }

    return Qt::NoButton;
}

QPoint GraphicsSceneWebControlCommandInterpreter::parseParamPoint(const QStringList &params, int startIndex)
{
    if (startIndex < params.count() - 1)
    {
        bool posXConverted = false;
        int posX = params[startIndex].toInt(&posXConverted);

        bool posYConverted = false;
        int posY = params[startIndex + 1].toInt(&posYConverted);

        if (posXConverted && posYConverted)
            return QPoint(posX, posY);
    }

    return QPoint();
}

bool GraphicsSceneWebControlCommandInterpreter::handleMouseEnter(const QString &command, const QStringList &params)
{
    if (!scene_->isActive())
        postEvent(QEvent::WindowActivate);

    postEvent(QEvent::Enter, true);

    if (!scene_->hasFocus())
        scene_->setFocus();

    QInputMethodEvent *enterFocus = new QInputMethodEvent();
    postEvent(enterFocus);

    return true;
}

bool GraphicsSceneWebControlCommandInterpreter::handleMouseExit(const QString &command, const QStringList &params)
{
    //postEvent(QEvent::Leave, true);
    //scene_->clearFocus();
    //postEvent(QEvent::WindowDeactivate, false);
    return true;
}

bool GraphicsSceneWebControlCommandInterpreter::handleMouseEvent(const QString &command, const QStringList &params)
{
    QPoint screenPos(parseParamPoint(params, 0));
    QPointF scenePos(screenPos);

    Qt::MouseButton button(parseParamMouseButton(params, 2));
    Qt::MouseButtons buttons(button);
    Qt::KeyboardModifier modifiers(parseParamKeyboardModifiers(params, 3));

    if (command.endsWith("Wheel") && params.count() >= 6)
    {
        QPoint delta(parseParamPoint(params, 4));

        QGraphicsSceneWheelEvent *we = new QGraphicsSceneWheelEvent(QEvent::GraphicsSceneWheel);
        we->setWidget(NULL);
        we->setScenePos(scenePos);
        we->setScreenPos(screenPos);
        we->setButtons(buttons);
        we->setModifiers(modifiers);

        if (delta.y() != 0)
        {
            we->setDelta(delta.y() * 10);
            we->setOrientation(Qt::Vertical);
        }
        else
        {
            we->setDelta(delta.x() * 10);
            we->setOrientation(Qt::Horizontal);
        }

        we->setAccepted(false);
        postEvent(we, true);
        return true;
    }
    else
    {
        QEvent::Type type(QEvent::GraphicsSceneMouseMove);

        if (command.endsWith("Move"))
        {
            type = QEvent::GraphicsSceneMouseMove;

            if (buttonDown_)
                buttons = button = lastButton_;
            else
                buttons = button = Qt::NoButton;
        }
        else if (command.endsWith("Down"))
        {
            type = QEvent::GraphicsSceneMousePress;
            lastButtonDownScenePos_ = scenePos;
            lastButtonDownScreenPos_ = screenPos;
            lastButton_ = button;
            buttonDown_ = true;
        }
        else if (command.endsWith("Up"))
        {
            type = QEvent::GraphicsSceneMouseRelease;
            buttonDown_ = false;
        }
        else if (command.endsWith("DblClick"))
        {
            type = QEvent::GraphicsSceneMouseDoubleClick;
            buttonDown_ = false;
        }
        else
        {
            buttonDown_ = false;
            return false;
        }

        QGraphicsSceneMouseEvent *me = new QGraphicsSceneMouseEvent(type);
        me->setButtonDownScenePos(button, lastButtonDownScenePos_);
        me->setButtonDownScreenPos(button, lastButtonDownScreenPos_);

        me->setScenePos(scenePos);
        me->setScreenPos(screenPos);

        me->setLastScenePos(lastScenePos_);
        me->setLastScreenPos(lastScreenPos_);

        lastScenePos_ = scenePos;
        lastScreenPos_ = screenPos;

        me->setButtons(buttons);
        me->setButton(button);
        me->setModifiers(modifiers);

        me->setAccepted(false);

        postEvent(me, true);
        return true;
    }
}

QString GraphicsSceneWebControlCommandInterpreter::textForKey(int key, Qt::KeyboardModifier modifiers)
{
    QString text = QChar(key);
    if (modifiers & Qt::ShiftModifier)
        text = text.toUpper();
    else
        text = text.toLower();

    return text;
}

bool GraphicsSceneWebControlCommandInterpreter::handleKeyEvent(const QString &command, const QStringList &params)
{
    int key = params[0].toInt();

    Qt::KeyboardModifier modifiers(parseParamKeyboardModifiers(params, 1));
    QEvent::Type type(QEvent::KeyPress);
    QString text;

    if (command.endsWith("Press"))
    {
        if (keyDownKeyCodeHandled_)
            return false;

        type = QEvent::KeyPress;
        text = textForKey(key, modifiers);
        key = keyDownKeyCode_; // make key the keyCode from keyDown event
    }
    else if (command.endsWith("Up"))
    {
        type = QEvent::KeyRelease;
        text = "";
    }
    else if (command.endsWith("Down"))
    {
        keyDownKeyCode_ = key;
        keyDownKeyCodeHandled_ = true;
        text = "";

        switch (key)
        {
        case 8  : key = Qt::Key_Backspace; break;
        case 9  : key = Qt::Key_Tab; break;
        case 13 : key = Qt::Key_Return; break;
        case 19 : key = Qt::Key_Pause; break;
        case 27 : key = Qt::Key_Escape; break;
        //case 32 : key = Qt::Key_Space; text = textForKey(key, modifiers); break;
        case 33 : key = Qt::Key_PageUp; break;
        case 34 : key = Qt::Key_PageDown; break;
        case 35 : key = Qt::Key_End; break;
        case 36 : key = Qt::Key_Home; break;
        case 37 : key = Qt::Key_Left; break;
        case 38 : key = Qt::Key_Up; break;
        case 39 : key = Qt::Key_Right; break;
        case 40 : key = Qt::Key_Down; break;
        case 44 : key = Qt::Key_Print; break;
        case 45 : key = Qt::Key_Insert; break;
        case 46 : key = Qt::Key_Delete; break;
        //case 48 ... 57 : /*key = Qt::Key_0 + (key - 48);*/ text = textForKey(key, modifiers); break; // 0 to 9
        //case 65 ... 90 : /*key = Qt::Key_A + (key - 64);*/ text = textForKey(key, modifiers); break; // A to Z
        case 96 : key = Qt::Key_0; break;
        case 97 : key = Qt::Key_1; break;
        case 98 : key = Qt::Key_2; break;
        case 99 : key = Qt::Key_3; break;
        case 100: key = Qt::Key_4; break;
        case 101: key = Qt::Key_5; break;
        case 102: key = Qt::Key_6; break;
        case 103: key = Qt::Key_7; break;
        case 104: key = Qt::Key_8; break;
        case 105: key = Qt::Key_9; break;
        case 112: key = Qt::Key_F1; break;
        case 113: key = Qt::Key_F2; break;
        case 114: key = Qt::Key_F3; break;
        case 115: key = Qt::Key_F4; break;
        case 116: key = Qt::Key_F5; break;
        case 117: key = Qt::Key_F6; break;
        case 118: key = Qt::Key_F7; break;
        case 119: key = Qt::Key_F8; break;
        case 120: key = Qt::Key_F9; break;
        case 121: key = Qt::Key_F10; break;
        case 122: key = Qt::Key_F11; break;
        case 123: key = Qt::Key_F12; break;
        case 144: key = Qt::Key_NumLock; break;
        case 145: key = Qt::Key_ScrollLock; break;
        default: keyDownKeyCodeHandled_ = false;
        }

        if (!keyDownKeyCodeHandled_)
            return false;

        type = QEvent::KeyPress;
    }
    else
    {
        qWarning("Unknown key command: %s", command.toUtf8().constData());
        return false;
    }

    QKeyEvent *ke = new QKeyEvent(type, key, modifiers, text);
    postEvent(ke, true);
    return true;
}

bool GraphicsSceneWebControlCommandInterpreter::canHandleMessage(const QByteArray &message, const QString &sessionId)
{
    return message.startsWith("mouseEnter") ||
           message.startsWith("mouseExit") ||
           message.startsWith("mouse") ||
           message.startsWith("key") ||
           message.startsWith("resize");
}

bool GraphicsSceneWebControlCommandInterpreter::handleMessage(const QByteArray &message, const QString &sessionId)
{
    QString command;
    QStringList params;

    MessageHandler::splitMessage(message, command, params);

    if (message.startsWith("mouseEnter"))
        return handleMouseEnter(command, params);
    else if (message.startsWith("mouseExit"))
        return handleMouseExit(command, params);
    else if (message.startsWith("mouse") && params.count() >= 2)
        return handleMouseEvent(command, params);
    else if (message.startsWith("key") && params.count() >= 1)
        return handleKeyEvent(command, params);
    else if (message.startsWith("resize") && params.count() == 2)
    {
        resizeScene(params);
        return true;
    }

    return false;
}

void GraphicsSceneWebControlCommandInterpreter::resizeScene(const QStringList &params)
{
    int width = params[0].toInt();
    int height = params[1].toInt();

    DPRINTF("Received new size: %d x %d", width, height);

    if (scene_->items().count() > 0)
    {
        QDeclarativeItem *mainItem = dynamic_cast<QDeclarativeItem *>(scene_->items().last());
        if (mainItem)
        {
            mainItem->setWidth(width);
            mainItem->setHeight(height);
        }
    }
}
