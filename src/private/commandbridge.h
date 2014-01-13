#ifndef COMMANDBRIDGE_H
#define COMMANDBRIDGE_H

#include <QObject>

class CommandBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString response READ response WRITE setResponse)

public:
    static CommandBridge& singleton();
#   define globalCommandBridge CommandBridge::singleton()

    QString response() const { return response_; }
    void setResponse(const QString &value) { response_ = value; }

protected:
    explicit CommandBridge(QObject *parent = 0);

public slots:
    QString handleCommandReady(const QString &id, const QString &command);

signals:
    void commandReceived(const QString &id, const QString &command);

private:
    QString response_;
};

#endif // COMMANDBRIDGE_H
