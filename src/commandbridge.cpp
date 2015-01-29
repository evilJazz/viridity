#include <QCoreApplication>
#include <QDebug>

#include "graphicsscenedisplay.h"
#include "graphicsscenedisplaysessionmanager.h"
#include "commandbridge.h"

CommandBridge::CommandBridge(GraphicsSceneDisplaySession *session, QObject *parent) :
    QObject(parent),
    session_(session),
    response_(QString::null),
    responseId_(0)
{
}

CommandBridge::~CommandBridge()
{
}

int CommandBridge::getNewResponseId()
{
    ++responseId_;
    return responseId_;
}

bool CommandBridge::dispatchMessage(const QString &message, const QString &displayId)
{
    QStringList messages;
    messages << message;

    GraphicsSceneDisplay *display = session_->sessionManager->acquireDisplay(displayId);
    if (display)
    {
        QMetaObject::invokeMethod(display, "dispatchAdditionalMessages", Q_ARG(const QStringList &, messages));
        session_->sessionManager->releaseDisplay(display);
        return true;
    }

    return false;
}

QVariant CommandBridge::sendCommand(const QString &command, const QString &destinationDisplayId)
{
    int result = getNewResponseId();

    QString message = QString("command(%1,%2)").arg(result).arg(command);

    int dispatched = 0;
    QStringList displayIds;

    // Broadcast to all displays of this scene if no specific destination display was set!
    if (destinationDisplayId.isEmpty())
        displayIds << session_->sessionManager->displaysIds(session_->scene);
    else
        displayIds << destinationDisplayId;

    foreach (const QString &displayId, displayIds)
    {
        if (dispatchMessage(message, displayId))
            ++dispatched;
    }

    return dispatched > 0 ? result : false;
}

bool CommandBridge::canHandleMessage(const QByteArray &message, const QString &displayId)
{
    return message.startsWith("commandResponse") || message.startsWith("command");
}

bool CommandBridge::handleMessage(const QByteArray &message, const QString &displayId)
{
    if (canHandleMessage(message, displayId))
    {
        int paramStartIndex = message.indexOf("(");
        int paramStopIndex = message.indexOf(")");
        QByteArray rawParams = message.mid(paramStartIndex + 1, paramStopIndex - paramStartIndex - 1);

        paramStartIndex = rawParams.indexOf(",");
        QString responseId = rawParams.mid(0, paramStartIndex);

        QString input = QString::fromUtf8(rawParams.mid(paramStartIndex + 1, rawParams.length() - paramStartIndex - 1));

        if (message.startsWith("commandResponse"))
        {
            emit responseReceived(responseId, input, displayId);
            return true;
        }
        else
        {
            response_ = QString::null;
            emit commandReceived(responseId, input);

            QString message = QString("commandResponse(%1,%2)").arg(responseId).arg(response());
            dispatchMessage(message, displayId);
            return true;
        }
    }

    return false;
}

QString CommandBridge::handleCommandReady(const QString &id, const QString &command)
{
qDebug("CommandBridge got command %s for display ID %s", command.toUtf8().constData(), id.toUtf8().constData());
    emit commandReceived(id, command);

qDebug("CommandBridge answers with %s for display ID %s", response_.toUtf8().constData(), id.toUtf8().constData());
    QString result = response_;
    response_ = QString::null;
    return result;
}
