#ifndef INPUTPOSTHANDLER_H
#define INPUTPOSTHANDLER_H

#include "Tufao/HttpServerRequest"

class ViriditySession;

class InputPostHandler : public QObject
{
    Q_OBJECT
public:
    explicit InputPostHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, ViriditySession *session, QObject *parent = 0);

private slots:
    void onData(const QByteArray &chunk);
    void onEnd();

private:
    Tufao::HttpServerRequest *request_;
    Tufao::HttpServerResponse *response_;
    ViriditySession *session_;
    QByteArray data_;
};

#endif // INPUTPOSTHANDLER_H
