#include "handlers/commandposthandler.h"

#include "private/commandbridge.h"

#include "Tufao/Headers"

#include <QUrl>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QUrlQuery>
#endif

CommandPostHandler::CommandPostHandler(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response, QObject *parent) :
    QObject(parent),
    request_(request),
    response_(response)
{
    connect(request_, SIGNAL(data(QByteArray)), this, SLOT(onData(QByteArray)));
    connect(request_, SIGNAL(end()), this, SLOT(onEnd()));
}

void CommandPostHandler::onData(const QByteArray &chunk)
{
    data_ += chunk;
}

void CommandPostHandler::onEnd()
{
    // handle request
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString id = QUrlQuery(QUrl(request_->url())).queryItemValue("id");
#else
    QString id = QUrl(request_->url()).queryItemValue("id");
#endif
    QString command(data_);
qDebug("Command is %s", data_.constData());

    QString result;
    metaObject()->invokeMethod(&globalCommandBridge, "handleCommandReady", Qt::BlockingQueuedConnection,
                               Q_RETURN_ARG(QString, result),
                               Q_ARG(QString, id),
                               Q_ARG(QString, command));

    if (!result.endsWith("\r\n"))
        result.append("\r\n");

    response_->headers().insert("Content-Type", "text/plain");
    response_->writeHead(Tufao::HttpServerResponse::OK);
    response_->end(result.toUtf8());
}

