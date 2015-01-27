#include <QCoreApplication>
#include <QDebug>

#include "graphicsscenedisplay.h"
#include "graphicsscenedisplaysessionmanager.h"
#include "commandbridge.h"

CommandBridge::CommandBridge(GraphicsSceneDisplaySession *session, QObject *parent) :
    QObject(parent),
    session_(session),
    responseId_(0),
    response_(QString::null)
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

QVariant CommandBridge::sendCommand(const QString &command, const QString &destinationDisplayId)
{
    int result = getNewResponseId();

    QStringList messages;
    messages << QString("command(%1,%2)").arg(result).arg(command);

    int dispatched = 0;
    QStringList displayIds;

    // Broadcast to all displays of this scene if no specific destination display was set!
    if (destinationDisplayId.isEmpty())
        displayIds << session_->sessionManager->displaysIds(session_->scene);
    else
        displayIds << destinationDisplayId;

    foreach (const QString &displayId, displayIds)
    {
        GraphicsSceneDisplay *display = session_->sessionManager->acquireDisplay(displayId);
        if (display)
        {
            QMetaObject::invokeMethod(display, "dispatchAdditionalMessages", Q_ARG(const QStringList &, messages));
            ++dispatched;

            session_->sessionManager->releaseDisplay(display);
        }
    }

    return dispatched > 0 ? result : false;
}

bool CommandBridge::canHandleCommand(const QString &command, const QStringList &params, const QString &displayId)
{
    return command.startsWith("commandResponse") && params.count() == 2;
}

bool CommandBridge::handleCommand(const QString &command, const QStringList &params, const QString &displayId)
{
    if (canHandleCommand(command, params, displayId))
    {
        emit responseReceived(params[0], params[1], displayId);
        return true;
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
