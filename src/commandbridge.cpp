#include <QCoreApplication>
#include <QDebug>

#include <QThread>

#include "viriditysessionmanager.h"
#include "commandbridge.h"

CommandBridge::CommandBridge(ViriditySession *session, QObject *parent) :
    QObject(parent),
    session_(session),
    response_(QString::null),
    responseId_(0)
{
}

CommandBridge::~CommandBridge()
{
}

void CommandBridge::setResponse(const QString &value)
{
    response_ = value;
}

int CommandBridge::getNewResponseId()
{
    ++responseId_;
    return responseId_;
}

QVariant CommandBridge::sendCommand(const QString &command, const QString &destinationSessionId)
{
    int result = getNewResponseId();

    QString message = QString("command(%1,%2)").arg(result).arg(command);

    bool dispatched = false;

    // Broadcast to all sessions that implement the same logic if no specific destination session was set!
    if (destinationSessionId.isEmpty())
        dispatched = session_->sessionManager()->dispatchMessageToClientMatchingLogic(message.toUtf8(), session_->logic);
    else
        dispatched = session_->sessionManager()->dispatchMessageToClient(message.toUtf8(), destinationSessionId);

    return dispatched ? result : false;
}

bool CommandBridge::canHandleMessage(const QByteArray &message, const QString &sessionId)
{
    return message.startsWith("commandResponse") || message.startsWith("command");
}

bool CommandBridge::handleMessage(const QByteArray &message, const QString &sessionId)
{
    bool result;

    QMetaObject::invokeMethod(
        this, "localHandleMessage",
        thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(bool, result),
        Q_ARG(const QByteArray &, message),
        Q_ARG(const QString &, sessionId)
    );

    return result;
}

bool CommandBridge::localHandleMessage(const QByteArray &message, const QString &sessionId)
{
    if (canHandleMessage(message, sessionId))
    {
        int paramStartIndex = message.indexOf("(");
        int paramStopIndex = message.indexOf(")");
        QByteArray rawParams = message.mid(paramStartIndex + 1, paramStopIndex - paramStartIndex - 1);

        paramStartIndex = rawParams.indexOf(",");
        QString responseId = rawParams.mid(0, paramStartIndex);

        QString input = QString::fromUtf8(rawParams.mid(paramStartIndex + 1, rawParams.length() - paramStartIndex - 1));

        if (message.startsWith("commandResponse"))
        {
            emit responseReceived(responseId, input, sessionId);
            return true;
        }
        else
        {
            response_ = QString::null;
            emit commandReceived(responseId, input);

            QString message = QString("commandResponse(%1,%2)").arg(responseId).arg(response());
            session_->dispatchMessageToClient(message.toUtf8());
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
