#include "viridityqmlrequesthandler.h"

#ifdef USE_QTQUICK1
    #include <QDeclarativeEngine>
    typedef QDeclarativeEngine DeclarativeEngine;
#else
    #include <QQmlEngine>
    typedef QQmlEngine DeclarativeEngine;
#endif

#include <QSharedPointer>
#include <QPointer>

#include "viriditywebserver.h"
#include "viridityconnection.h"

#include "KCL/objectutils.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

/* PrivateViridityRequestWrapper */

class PrivateViridityRequestWrapper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString method READ method CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)
public:
    PrivateViridityRequestWrapper(QSharedPointer<ViridityHttpServerRequest> request) :
        QObject(),
        request_(request)
    {
        DGUARDMETHODTIMED;
        connect(request_->socket().data(), SIGNAL(disconnected()), this, SLOT(deleteLater()));
    }

    virtual ~PrivateViridityRequestWrapper()
    {
        DGUARDMETHODTIMED;
    }

    QString method() const { return request_ ? QString::fromLatin1(request_->method()) : QString::null; }
    QString url() const { return request_ ? QString::fromUtf8(request_->url()) : QString::null; }

private:
    QSharedPointer<ViridityHttpServerRequest> request_;
};

/* PrivateViridityResponseWrapper */

class PrivateViridityResponseWrapper : public QObject
{
    Q_OBJECT
public:
    PrivateViridityResponseWrapper(QSharedPointer<ViridityHttpServerResponse> response) :
        QObject(),
        response_(response)
    {
        DGUARDMETHODTIMED;
        connect(response_.data(), SIGNAL(finished()), this, SLOT(deleteLater()));
        connect(response_->socket().data(), SIGNAL(disconnected()), this, SLOT(deleteLater()));
    }

    virtual ~PrivateViridityResponseWrapper()
    {
        DGUARDMETHODTIMED;
    }

public slots:

    void end(const QByteArray &chunk = QByteArray())
    {
        DGUARDMETHODTIMED;
        if (response_)
            QMetaObject::invokeMethod(response_.data(), "end", Qt::QueuedConnection, Q_ARG(QByteArray, chunk));
    }

    void write(const QByteArray &chunk)
    {
        DGUARDMETHODTIMED;
        if (response_)
            QMetaObject::invokeMethod(response_.data(), "write", Qt::QueuedConnection, Q_ARG(QByteArray, chunk));
    }

    void writeHead(int statusCode)
    {
        DGUARDMETHODTIMED;
        if (response_)
            QMetaObject::invokeMethod(response_.data(), "writeHead", Qt::QueuedConnection, Q_ARG(int, statusCode));
    }

private:
    QSharedPointer<ViridityHttpServerResponse> response_;
};

/* PrivateQmlRequestHandler */

class ViridityQmlRequestHandler;

class PrivateQmlRequestHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit PrivateQmlRequestHandler(ViridityQmlRequestHandler *proxy, ViridityWebServer *server, QObject *parent = NULL) :
        ViridityBaseRequestHandler(server, parent),
        proxy_(proxy)
    {
        DGUARDMETHODTIMED;
    }

    virtual ~PrivateQmlRequestHandler()
    {
        DGUARDMETHODTIMED;
    }

    bool doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request)
    {
        DGUARDMETHODTIMED;

        if (!proxy_) return false;

        QVariant result = false;

        PrivateViridityRequestWrapper *requestWrapper = new PrivateViridityRequestWrapper(request);
        requestWrapper->moveToThread(proxy_->thread());
        QVariant requestVar = ObjectUtils::objectToVariant(requestWrapper);

        if (!QMetaObject::invokeMethod(
                proxy_,
                "doesHandleRequest",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QVariant, result),
                Q_ARG(QVariant, requestVar)
            ))
        {
            qDebug("Please implement the function doesHandleRequest(request) in ViridityQmlRequestHandler instance.");
        }

        return result.toBool();
    }

    void handleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
    {
        DGUARDMETHODTIMED;

        if (!proxy_) return;

        PrivateViridityRequestWrapper *requestWrapper = new PrivateViridityRequestWrapper(request);
        requestWrapper->moveToThread(proxy_->thread());
        QVariant requestVar = ObjectUtils::objectToVariant(requestWrapper);

        PrivateViridityResponseWrapper *responseWrapper = new PrivateViridityResponseWrapper(response);
        responseWrapper->moveToThread(proxy_->thread());
        QVariant responseVar = ObjectUtils::objectToVariant(responseWrapper);

        if (!QMetaObject::invokeMethod(
                proxy_,
                "handleRequest",
                Qt::QueuedConnection,
                Q_ARG(QVariant, requestVar),
                Q_ARG(QVariant, responseVar)
            ))
        {
            qDebug("Please implement the function handleRequest(request, response) in ViridityQmlRequestHandler instance.");
        }
    }

private:
    QPointer<ViridityQmlRequestHandler> proxy_;
};

/* ViridityQmlRequestHandler */

ViridityQmlRequestHandler::ViridityQmlRequestHandler(QObject *parent) :
    ViridityDeclarativeBaseObject(parent),
    requestHandler_(NULL)
{
    DGUARDMETHODTIMED;
}

ViridityQmlRequestHandler::~ViridityQmlRequestHandler()
{
    DGUARDMETHODTIMED;
    if (requestHandler_ && requestHandler_->server())
        requestHandler_->server()->unregisterRequestHandler(requestHandler_);
}

void ViridityQmlRequestHandler::componentComplete()
{
    DGUARDMETHODTIMED;

    ViridityWebServer *server = this->webServer();

    if (server)
    {
        requestHandler_ = QSharedPointer<PrivateQmlRequestHandler>(new PrivateQmlRequestHandler(this, server));
        server->registerRequestHandler(requestHandler_);
    }
    else
        qDebug("Can't determine engine context for object.");
}

#include "viridityqmlrequesthandler.moc"