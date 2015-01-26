#ifndef COMMANDBRIDGE_H
#define COMMANDBRIDGE_H

#include <QObject>
#include <QVariant>

class CommandBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString response READ response WRITE setResponse)

public:
    static CommandBridge& singleton();
#   define globalCommandBridge CommandBridge::singleton()

    QString response() const { return response_; }
    void setResponse(const QString &value) { response_ = value; }

    Q_INVOKABLE QVariant sendCommand(const QString &command);

protected:
    explicit CommandBridge(QObject *parent = 0);

public slots:
    QString handleCommandReady(const QString &id, const QString &command);

signals:
    void commandReceived(const QString &id, const QString &command);

    void responseReceived(const QVariant responseId, const QString response);

private:
    QString response_;
    int responseId_;

    int getNewResponseId();
};

#endif // COMMANDBRIDGE_H
