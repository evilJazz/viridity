#ifndef COMMANDBRIDGE_H
#define COMMANDBRIDGE_H

#include <QObject>
#include <QVariant>

#include "graphicsscenewebcontrolcommandinterpreter.h"

class GraphicsSceneDisplaySession;

class CommandBridge : public QObject, public MessageHandler
{
    Q_OBJECT
    Q_PROPERTY(QString response READ response WRITE setResponse)

public:
    explicit CommandBridge(GraphicsSceneDisplaySession *session, QObject *parent = 0);
    virtual ~CommandBridge();

    QString response() const { return response_; }
    void setResponse(const QString &value) { response_ = value; }

    Q_INVOKABLE QVariant sendCommand(const QString &command, const QString &destinationDisplayId);

protected:
    // MessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &displayId);
    virtual bool handleMessage(const QByteArray &message, const QString &displayId);

public slots:
    QString handleCommandReady(const QString &id, const QString &command);

signals:
    void commandReceived(const QString &id, const QString &command);

    void responseReceived(const QVariant &responseId, const QString &response, const QString &displayId);

private:
    GraphicsSceneDisplaySession *session_;

    QString response_;
    int responseId_;

    int getNewResponseId();
};

#endif // COMMANDBRIDGE_H
