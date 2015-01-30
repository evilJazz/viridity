#ifndef COMMANDPOSTHANDLER_H
#define COMMANDPOSTHANDLER_H

#include <QObject>

#include "Tufao/HttpServerRequest"


class CommandPostHandler : public QObject
{
    Q_OBJECT

public:
    explicit CommandPostHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, QObject *parent = 0);

private slots:
    void onData(const QByteArray &chunk);
    void onEnd();

private:
    Tufao::HttpServerRequest *request_;
    Tufao::HttpServerResponse *response_;
    QByteArray data_;
};

#endif // COMMANDPOSTHANDLER_H
