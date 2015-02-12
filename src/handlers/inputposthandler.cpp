#include "inputposthandler.h"

#include "viriditywebserver.h"
#include "viriditysessionmanager.h"

InputPostHandler::InputPostHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, ViriditySession *session, QObject *parent) :
    QObject(parent),
    request_(request),
    response_(response),
    session_(session)
{
    connect(request_, SIGNAL(data(QByteArray)), this, SLOT(onData(QByteArray)));
    connect(request_, SIGNAL(end()), this, SLOT(onEnd()));
}

void InputPostHandler::onData(const QByteArray &chunk)
{
    data_ += chunk;
}

void InputPostHandler::onEnd()
{
    QList<QByteArray> messages = data_.split('\n');

    foreach (const QByteArray &message, messages)
        session_->dispatchMessageToHandlers(message);

    // handle request
    response_->headers().insert("Content-Type", "text/plain");
    response_->writeHead(Tufao::HttpServerResponse::OK);
    response_->end();
}
