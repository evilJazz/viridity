#ifndef INPUTPOSTHANDLER_H
#define INPUTPOSTHANDLER_H

#include <QObject>

#include "viridityrequesthandler.h"

class ViriditySession;

class InputPostHandler : public QObject
{
    Q_OBJECT
public:
    explicit InputPostHandler(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response, ViriditySession *session, QObject *parent = 0);

private slots:
    void onData(const QByteArray &chunk);
    void onEnd();

private:
    ViridityHttpServerRequest *request_;
    ViridityHttpServerResponse *response_;
    ViriditySession *session_;
    QByteArray data_;
};

#endif // INPUTPOSTHANDLER_H
