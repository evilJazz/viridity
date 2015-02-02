#ifndef VIRIDITYDATABRIDGE_H
#define VIRIDITYDATABRIDGE_H

#include <QObject>
#include <QVariant>

#include "viriditysessionmanager.h"

class ViridityDataBridge : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
    Q_PROPERTY(ViriditySession *session READ session WRITE setSession NOTIFY sessionChanged)
    Q_PROPERTY(QString targetId READ targetId WRITE setTargetId NOTIFY targetIdChanged)
    Q_PROPERTY(QString response READ response WRITE setResponse NOTIFY responseChanged)

public:
    explicit ViridityDataBridge(QObject *parent = 0);
    virtual ~ViridityDataBridge();

    void setSession(ViriditySession *session);
    ViriditySession *session() const { return session_; }

    void setTargetId(const QString &targetId);
    QString targetId() const { return targetId_; }

    QString response() const { return response_; }
    void setResponse(const QString &value);

    Q_INVOKABLE QVariant sendData(const QString &command, const QString &destinationSessionId);

protected:
    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

private:
    Q_INVOKABLE bool localHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

signals:
    void dataReceived(const QString &responseId, const QString &input);
    void responseReceived(const QVariant &responseId, const QString &response, const QString &sessionId);

    void sessionChanged();
    void targetIdChanged();
    void responseChanged();

private:
    ViriditySession *session_;
    QString targetId_;

    QString response_;
    int responseId_;

    int getNewResponseId();

    void registerHandler();
    void unregisterHandler();
};

#endif // VIRIDITYDATABRIDGE_H
