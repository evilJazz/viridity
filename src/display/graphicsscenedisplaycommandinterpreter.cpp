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

#include "graphicsscenedisplaycommandinterpreter.h"

#include <QCoreApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QStringList>

#include "graphicssceneadapter.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

/* GraphicsSceneDisplayCommandInterpreter */

GraphicsSceneDisplayCommandInterpreter::GraphicsSceneDisplayCommandInterpreter(QObject *parent) :
    QObject(parent),
    adapter_(NULL),
    keyDownKeyCodeHandled_(false)
{
}

GraphicsSceneDisplayCommandInterpreter::~GraphicsSceneDisplayCommandInterpreter()
{
}

void GraphicsSceneDisplayCommandInterpreter::setTargetGraphicsSceneAdapter(AbstractGraphicsSceneAdapter *adapter)
{
    adapter_ = adapter;
}

Qt::KeyboardModifier GraphicsSceneDisplayCommandInterpreter::parseParamKeyboardModifiers(const QStringList &params, int index)
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

Qt::MouseButton GraphicsSceneDisplayCommandInterpreter::parseParamMouseButton(const QStringList &params, int index)
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

QPoint GraphicsSceneDisplayCommandInterpreter::parseParamPoint(const QStringList &params, int startIndex)
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

bool GraphicsSceneDisplayCommandInterpreter::handleMouseEvent(const QString &command, const QStringList &params)
{
    QPoint screenPos(parseParamPoint(params, 0));
    QPointF scenePos(screenPos);

    Qt::MouseButton button(parseParamMouseButton(params, 2));
    Qt::MouseButtons buttons(button);
    Qt::KeyboardModifier modifiers(parseParamKeyboardModifiers(params, 3));

    if (command.endsWith("Wheel") && params.count() >= 6)
    {
        QPoint delta(parseParamPoint(params, 4));
        adapter_->handleMouseWheelEvent(delta, scenePos, button, buttons, modifiers);
        return true;
    }
    else if (command == "contextMenu")
    {
        adapter_->handleContextMenuEvent(scenePos, modifiers, QContextMenuEvent::Mouse);
        return true;
    }
    else if (command.endsWith("Enter"))
    {
        adapter_->handleMouseEnterEvent(scenePos, button, buttons, modifiers);
        return true;
    }
    else if (command.endsWith("Exit"))
    {
        adapter_->handleMouseExitEvent(scenePos, button, buttons, modifiers);
        return true;
    }
    else
    {
        QEvent::Type type(QEvent::GraphicsSceneMouseMove);

        if (command.endsWith("Move"))
            type = QEvent::MouseMove;
        else if (command.endsWith("Down"))
            type = QEvent::MouseButtonPress;
        else if (command.endsWith("Up"))
            type = QEvent::MouseButtonRelease;
        else if (command.endsWith("DblClick"))
            type = QEvent::MouseButtonDblClick;
        else
            return false;

        adapter_->handleMouseEvent(type, scenePos, button, buttons, modifiers);
        return true;
    }
}

QString GraphicsSceneDisplayCommandInterpreter::textForKey(int key, Qt::KeyboardModifier modifiers)
{
    QString text = QChar(key);
    if (modifiers & Qt::ShiftModifier)
        text = text.toUpper();
    else
        text = text.toLower();

    return text;
}

bool GraphicsSceneDisplayCommandInterpreter::handleKeyEvent(const QString &command, const QStringList &params)
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

    adapter_->handleKeyEvent(type, key, modifiers, text);
    return true;
}

bool GraphicsSceneDisplayCommandInterpreter::handleTextEvent(const QString &command, const QStringList &params)
{
    QString textBase64 = params[0];

    QString text = QString::fromUtf8(QByteArray::fromBase64(textBase64.toUtf8()));

    Qt::KeyboardModifier modifiers = Qt::NoModifier;
    int key = 0;

    if (text == "\n")
        key = Qt::Key_Return;
    else if (text.length() == 1)
    {
        QChar c = text[0];

        if (c.isUpper())
            modifiers = Qt::ShiftModifier;
        else
            c = c.toUpper();

        key = c.unicode();
    }

    qDebug("Received text from client: >%s<  key: 0x%x, modifiers: 0x%x", text.toUtf8().constData(), key, modifiers);

    adapter_->handleKeyEvent(QEvent::KeyPress, key, modifiers, text);
    adapter_->handleKeyEvent(QEvent::KeyRelease, key, modifiers, text);
    return true;
}

bool GraphicsSceneDisplayCommandInterpreter::canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    return message.startsWith("mouse") ||
           message.startsWith("key") ||
           message.startsWith("text") ||
           message.startsWith("contextMenu");
}

bool GraphicsSceneDisplayCommandInterpreter::handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    if (!adapter_)
        return false;

    QString command;
    QStringList params;

    ViridityMessageHandler::splitMessage(message, command, params);

    if ((message.startsWith("mouse") || message.startsWith("contextMenu")) && params.count() >= 2)
        return handleMouseEvent(command, params);
    else if (message.startsWith("key") && params.count() >= 1)
        return handleKeyEvent(command, params);
    else if (message.startsWith("text") && params.count() >= 1)
        return handleTextEvent(command, params);

    return false;
}
