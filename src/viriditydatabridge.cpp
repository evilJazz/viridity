#include <QCoreApplication>
#include <QDebug>

#include <QThread>

#include "viriditysessionmanager.h"
#include "viriditydatabridge.h"

ViridityDataBridge::ViridityDataBridge(QObject *parent) :
    QObject(parent),
    response_(QString::null),
    responseId_(0)
{
}

ViridityDataBridge::~ViridityDataBridge()
{
}

void ViridityDataBridge::setSession(ViriditySession *session)
{
    unregisterHandler();
    session_ = session;
    registerHandler();
    emit sessionChanged();
}

void ViridityDataBridge::setTargetId(const QString &targetId)
{
    unregisterHandler();
    targetId_ = targetId;
    registerHandler();
    emit targetIdChanged();
}

void ViridityDataBridge::setResponse(const QString &value)
{
    response_ = value;
    emit responseChanged();
}

int ViridityDataBridge::getNewResponseId()
{
    ++responseId_;
    return responseId_;
}

void ViridityDataBridge::registerHandler()
{
    if (session_)
        session_->registerHandler(this);
}

void ViridityDataBridge::unregisterHandler()
{
    if (session_)
        session_->unregisterHandler(this);
}

QVariant ViridityDataBridge::sendData(const QString &command, const QString &destinationSessionId)
{
    int result = getNewResponseId();

    QString message = QString("data(%1,%2)").arg(result).arg(command);

    bool dispatched = false;

    // Broadcast to all sessions that implement the same logic if no specific destination session was set!
    if (destinationSessionId.isEmpty())
        dispatched = session_->sessionManager()->dispatchMessageToClientMatchingLogic(message.toUtf8(), session_->logic, targetId_);
    else
        dispatched = session_->sessionManager()->dispatchMessageToClient(message.toUtf8(), destinationSessionId, targetId_);

    return dispatched ? result : false;
}

bool ViridityDataBridge::canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    return targetId == targetId_ && (message.startsWith("dataResponse") || message.startsWith("data"));
}

bool ViridityDataBridge::handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    bool result;

    QMetaObject::invokeMethod(
        this, "localHandleMessage",
        thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(bool, result),
        Q_ARG(const QByteArray &, message),
        Q_ARG(const QString &, sessionId),
        Q_ARG(const QString &, targetId)
    );

    return result;
}

bool ViridityDataBridge::localHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    if (canHandleMessage(message, sessionId, targetId))
    {
        int paramStartIndex = message.indexOf("(");
        int paramStopIndex = message.indexOf(")");
        QByteArray rawParams = message.mid(paramStartIndex + 1, paramStopIndex - paramStartIndex - 1);

        paramStartIndex = rawParams.indexOf(",");
        QString responseId = rawParams.mid(0, paramStartIndex);

        QString input = QString::fromUtf8(rawParams.mid(paramStartIndex + 1, rawParams.length() - paramStartIndex - 1));

        if (message.startsWith("dataResponse"))
        {
            emit responseReceived(responseId, input, sessionId);
            return true;
        }
        else
        {
            setResponse(QString::null);
            emit dataReceived(responseId, input);

            QString message = QString("dataResponse(%1,%2)").arg(responseId).arg(response());
            session_->dispatchMessageToClient(message.toUtf8(), targetId);
            return true;
        }
    }

    return false;
}
