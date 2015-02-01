#ifndef COMMANDBRIDGE_H
#define COMMANDBRIDGE_H

#include <QObject>
#include <QVariant>

#include "viriditysessionmanager.h"

class CommandBridge : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
    Q_PROPERTY(QString response READ response WRITE setResponse)

public:
    explicit CommandBridge(ViriditySession *session, QObject *parent = 0);
    virtual ~CommandBridge();

    QString response() const { return response_; }
    void setResponse(const QString &value);

    Q_INVOKABLE QVariant sendCommand(const QString &command, const QString &destinationSessionId);

protected:
    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

public slots:
    QString handleCommandReady(const QString &id, const QString &command);

private:
    Q_INVOKABLE bool localHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

signals:
    void commandReceived(const QString &id, const QString &command);

    void responseReceived(const QVariant &responseId, const QString &response, const QString &displayId);

private:
    ViriditySession *session_;

    QString response_;
    int responseId_;

    int getNewResponseId();
};

#endif // COMMANDBRIDGE_H
