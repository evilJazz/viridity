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

#ifndef VIRIDITYDATABRIDGE_H
#define VIRIDITYDATABRIDGE_H

#include <QObject>
#include <QVariant>
#include <QPointer>
#include <QSet>

#include "viriditysessionmanager.h"

class ViridityNativeDataBridge : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
    Q_PROPERTY(ViriditySession *session READ session WRITE setSession NOTIFY sessionChanged)
    Q_PROPERTY(AbstractViriditySessionManager *sessionManager READ sessionManager WRITE setSessionManager NOTIFY sessionManagerChanged)
    Q_PROPERTY(QString targetId READ targetId WRITE setTargetId NOTIFY targetIdChanged)
    Q_PROPERTY(QString response READ response WRITE setResponse NOTIFY responseChanged)

public:
    explicit ViridityNativeDataBridge(QObject *parent = 0);
    virtual ~ViridityNativeDataBridge();

    void setSessionManager(AbstractViriditySessionManager *sessionManager);
    AbstractViriditySessionManager *sessionManager() const;

    void setSession(ViriditySession *session);
    ViriditySession *session() const { return session_; }

    void setTargetId(const QString &targetId);
    QString targetId() const { return targetId_; }

    QString response() const { return response_; }
    void setResponse(const QString &value);

    Q_INVOKABLE QVariant sendData(const QString &data, const QString &destinationSessionId = QString::null);

protected:
    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

private:
    Q_INVOKABLE bool localHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

signals:
    void dataReceived(const QString &responseId, const QString &input);
    void responseReceived(const QString &responseId, const QString &response, const QString &sessionId);
    void sessionSubscribed(const QString &responseId, const QString &subscribingSessionId);
    void sessionUnsubscribed(const QString &responseId, const QString &subscribingSessionId);

    void sessionManagerChanged();
    void sessionChanged();
    void targetIdChanged();
    void responseChanged();

private slots:
    void handleSessionDestroyed();

private:
    QPointer<ViriditySession> session_;
    QPointer<AbstractViriditySessionManager> sessionManager_;
    QSet<QString> subscriberSessionIds_;
    QString targetId_;

    QString response_;
    int responseId_;

    int getNewResponseId();

    void registerMessageHandler();
    void unregisterMessageHandler();

    void subscribeSession(const QString &sessionId);
    void unsubscribeSession(const QString &unsubscribingSessionId);
};

#endif // VIRIDITYDATABRIDGE_H
