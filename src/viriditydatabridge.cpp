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

#include <QCoreApplication>
#include <QDebug>

#include <QThread>

#include "viriditysessionmanager.h"
#include "viriditydatabridge.h"

ViridityNativeDataBridge::ViridityNativeDataBridge(QObject *parent) :
    QObject(parent),
    session_(NULL),
    sessionManager_(NULL),
    response_(QString::null),
    responseId_(0)
{
}

ViridityNativeDataBridge::~ViridityNativeDataBridge()
{
    unregisterMessageHandler();
}

void ViridityNativeDataBridge::handleSessionDestroyed()
{
    session_ = NULL;
}

void ViridityNativeDataBridge::setSessionManager(AbstractViriditySessionManager *sessionManager)
{
    unregisterMessageHandler();
    sessionManager_ = sessionManager;
    registerMessageHandler();
    emit sessionManagerChanged();
}

void ViridityNativeDataBridge::setSession(ViriditySession *session)
{
    unregisterMessageHandler();
    session_ = session;
    registerMessageHandler();
    emit sessionChanged();
}

void ViridityNativeDataBridge::setTargetId(const QString &targetId)
{
    unregisterMessageHandler();
    targetId_ = targetId;
    registerMessageHandler();
    emit targetIdChanged();
}

void ViridityNativeDataBridge::setResponse(const QString &value)
{
    response_ = value;
    emit responseChanged();
}

int ViridityNativeDataBridge::getNewResponseId()
{
    ++responseId_;
    return responseId_;
}

void ViridityNativeDataBridge::registerMessageHandler()
{
    if (session_)
    {
        session_->registerMessageHandler(this);
        connect(session_, SIGNAL(destroyed()), this, SLOT(handleSessionDestroyed()), Qt::DirectConnection);
    }

    if (sessionManager_)
        sessionManager_->registerMessageHandler(this);
}

void ViridityNativeDataBridge::unregisterMessageHandler()
{
    if (session_)
    {
        disconnect(session_, SIGNAL(destroyed()), this, SLOT(handleSessionDestroyed()));
        session_->unregisterMessageHandler(this);
    }

    if (sessionManager_)
        sessionManager_->unregisterMessageHandler(this);
}

AbstractViriditySessionManager *ViridityNativeDataBridge::sessionManager() const
{
    return session_ ? session_->sessionManager() : sessionManager_.data();
}

QVariant ViridityNativeDataBridge::sendData(const QString &data, const QString &destinationSessionId)
{
    int result = getNewResponseId();

    if (!session_ && !sessionManager_)
    {
        qWarning("No session or session manager assigned. Will not send data.");
        return false;
    }

    QString message = QString("data(%1):%2").arg(result).arg(data);

    bool dispatched = false;

    if (session_)
    {
        // Broadcast to all sessions that implement the same logic if no specific destination session was set!
        if (destinationSessionId.isEmpty())
            dispatched = sessionManager()->dispatchMessageToClientMatchingLogic(message.toUtf8(), session_->logic(), targetId_);
        else
            dispatched = sessionManager()->dispatchMessageToClient(message.toUtf8(), destinationSessionId, targetId_);
    }
    else
    {
        // Send data only to subscribers, i.e. when this data bridge is not in context of a specific owner session...
        foreach (const QString &sessionId, subscriberSessionIds_)
            dispatched = sessionManager()->dispatchMessageToClient(message.toUtf8(), sessionId, targetId_);
    }

    return dispatched ? result : false;
}

bool ViridityNativeDataBridge::canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    return targetId == targetId_ && (message.startsWith("dataResponse") || message.startsWith("data") || message.startsWith("dataSubscribe"));
}

bool ViridityNativeDataBridge::handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    bool result;

    QMetaObject::invokeMethod(
        this, "localHandleMessage",
        this->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(bool, result),
        Q_ARG(const QByteArray &, message),
        Q_ARG(const QString &, sessionId),
        Q_ARG(const QString &, targetId)
    );

    return result;
}

bool ViridityNativeDataBridge::localHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    if (canHandleMessage(message, sessionId, targetId))
    {
        QString command;
        QStringList params;

        int datagramStartIndex = ViridityMessageHandler::splitMessage(message, command, params);

        if (params.length() == 1)
        {
            if (datagramStartIndex > -1)
            {
                QString responseId = params.at(0);
                QString input = QString::fromUtf8(message.mid(datagramStartIndex));

                if (command == "dataResponse")
                {
                    emit responseReceived(responseId, input, sessionId);
                    return true;
                }
                else
                {
                    setResponse(QString::null);
                    emit dataReceived(responseId, input);

                    QString message = QString("dataResponse(%1):%2").arg(responseId).arg(response());
                    sessionManager()->dispatchMessageToClient(message.toUtf8(), sessionId, targetId);
                    return true;
                }
            }
        }
        else if (params.length() == 2 && command == "dataSubscribe")
        {
            QString responseId = params.at(0);
            QString subscribingSessionId = params.at(1);

            setResponse(QString::null);
            emit sessionSubscribed(responseId, subscribingSessionId);

            if (response() != "false" && !subscriberSessionIds_.contains(subscribingSessionId))
                subscriberSessionIds_.insert(subscribingSessionId);

            QString message = QString("dataResponse(%1):%2").arg(responseId).arg(response());
            sessionManager()->dispatchMessageToClient(message.toUtf8(), subscribingSessionId, targetId);
            return true;
        }
    }

    return false;
}
