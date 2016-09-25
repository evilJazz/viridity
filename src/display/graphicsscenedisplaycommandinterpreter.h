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
