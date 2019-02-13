#include "viridityqmlrequesthandler.h"

#ifdef VIRIDITY_USE_QTQUICK1
    #include <QDeclarativeEngine>
    typedef QDeclarativeEngine DeclarativeEngine;
#else
    #include <QQmlEngine>
    typedef QQmlEngine DeclarativeEngine;
#endif

#include <QSharedPointer>
#include <QPointer>
#include <QRegExp>

#include <QReadLocker>
#include <QWriteLocker>

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
        response_(response),
        captureOutput_(false),
        newContent_(),
        statusCode_(0)
    {
        DGUARDMETHODTIMED;
        connect(response_.data(), SIGNAL(finished()), this, SLOT(deleteLater()));
        connect(response_->socket().data(), SIGNAL(disconnected()), this, SLOT(deleteLater()));
    }

    virtual ~PrivateViridityResponseWrapper()
    {
        DGUARDMETHODTIMED;
    }

    QSharedPointer<ViridityHttpServerResponse> response() { return response_; }

    void setCaptureOutputEnabled(bool enabled) { captureOutput_ = enabled; }
    QByteArray content() const { return newContent_; }
    ViridityHttpHeaders headers() const { return headers_; }
    int statusCode() const { return statusCode_; }

public slots:

    void end(const QByteArray &chunk = QByteArray())
    {
        DGUARDMETHODTIMED;
        if (response_)
            QMetaObject::invokeMethod(response_.data(), "end", Qt::QueuedConnection, Q_ARG(QByteArray, chunk));

        if (captureOutput_)
            newContent_ += chunk;

        emit done(this);
    }

    void write(const QByteArray &chunk)
    {
        DGUARDMETHODTIMED;
        if (response_)
            QMetaObject::invokeMethod(response_.data(), "write", Qt::QueuedConnection, Q_ARG(QByteArray, chunk));

        if (captureOutput_)
            newContent_ += chunk;
    }

    void writeHead(int statusCode)
    {
        DGUARDMETHODTIMED;
        if (response_)
            QMetaObject::invokeMethod(response_.data(), "writeHead", Qt::QueuedConnection, Q_ARG(int, statusCode));

        if (captureOutput_)
            statusCode_ = statusCode;
    }

    void addHeader(const QByteArray &key, const QByteArray &value)
    {
        DGUARDMETHODTIMED;

        if (response_)
            response_->headers().insert(key, value);

        if (captureOutput_)
            headers_.insert(key, value);
    }

    void removeHeader(const QByteArray &key, const QByteArray &value)
    {
        DGUARDMETHODTIMED;

        if (response_)
            response_->headers().remove(key, value);

        if (captureOutput_)
            headers_.remove(key);
    }

    void removeHeader(const QByteArray &key)
    {
        DGUARDMETHODTIMED;

        if (response_)
            response_->headers().remove(key);

        if (captureOutput_)
            headers_.remove(key);
    }

signals:
    void done(PrivateViridityResponseWrapper *);

private:
    QSharedPointer<ViridityHttpServerResponse> response_;

    bool captureOutput_;
    QByteArray newContent_;
    ViridityHttpHeaders headers_;
    int statusCode_;
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

    void filterRequestResponse(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
    {
        DGUARDMETHODTIMED;

        if (!proxy_ || proxy_->contentCachingEnabled()) return;

        if (proxy_->metaObject()->indexOfMethod("filterRequestResponse(QVariant,QVariant)") == -1)
            return;

        PrivateViridityRequestWrapper *requestWrapper = new PrivateViridityRequestWrapper(request);
        requestWrapper->moveToThread(proxy_->thread());
        QVariant requestVar = ObjectUtils::objectToVariant(requestWrapper);

        PrivateViridityResponseWrapper *responseWrapper = new PrivateViridityResponseWrapper(response);
        responseWrapper->moveToThread(proxy_->thread());
        QVariant responseVar = ObjectUtils::objectToVariant(responseWrapper);

        QMetaObject::invokeMethod(
            proxy_,
            "filterRequestResponse",
            Qt::QueuedConnection,
            Q_ARG(QVariant, requestVar),
            Q_ARG(QVariant, responseVar)
        );
    }

    bool doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request)
    {
        DGUARDMETHODTIMED;

        if (!proxy_) return false;

        if (!proxy_->handlesUrl().isEmpty())
        {
            QRegExp re(proxy_->handlesUrl());
            re.setMinimal(true);
            return re.indexIn(request->url()) > -1;
        }

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

        if (proxy_->contentCachingEnabled() && proxy_->cachedContentValid())
        {
            response->writeHead(proxy_->cachedStatusCode(), proxy_->cachedHeaders());
            response->end(proxy_->cachedContent());
        }
        else
        {
            PrivateViridityRequestWrapper *requestWrapper = new PrivateViridityRequestWrapper(request);
            requestWrapper->moveToThread(proxy_->thread());
            QVariant requestVar = ObjectUtils::objectToVariant(requestWrapper);

            PrivateViridityResponseWrapper *responseWrapper = new PrivateViridityResponseWrapper(response);

            if (proxy_->contentCachingEnabled())
            {
                responseWrapper->setCaptureOutputEnabled(true);
                connect(responseWrapper, SIGNAL(done(PrivateViridityResponseWrapper *)), this, SLOT(responseWrapperDone(PrivateViridityResponseWrapper *)));
            }

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
    }

private slots:
    void responseWrapperDone(PrivateViridityResponseWrapper *responseWrapper)
    {
        if (proxy_ && proxy_->contentCachingEnabled())
        {
            proxy_->setCachedContent(responseWrapper->content());
            proxy_->setCachedStatusCode(responseWrapper->statusCode() != 0 ? responseWrapper->statusCode() : 200);
            proxy_->setCachedHeaders(responseWrapper->headers());
        }
    }

private:
    QPointer<ViridityQmlRequestHandler> proxy_;
};

/* ViridityQmlRequestHandler */

ViridityQmlRequestHandler::ViridityQmlRequestHandler(QObject *parent) :
    ViridityDeclarativeBaseObject(parent),
    requestHandler_(NULL),
    cachedContentMREW_(QReadWriteLock::Recursive),
    cachedContentValid_(false),
    cachedContent_(),
    handlesUrl_()
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

bool ViridityQmlRequestHandler::contentCachingEnabled() const
{
    QReadLocker rl(&cachedContentMREW_);
    return contentCachingEnabled_;
}

void ViridityQmlRequestHandler::setContentCachingEnabled(bool enabled)
{
    QWriteLocker wl(&cachedContentMREW_);
    if (enabled != contentCachingEnabled_)
    {
        contentCachingEnabled_ = enabled;
        emit contentCachingEnabledChanged();
    }
}

bool ViridityQmlRequestHandler::cachedContentValid() const
{
    QReadLocker rl(&cachedContentMREW_);
    return cachedContentValid_;
}

void ViridityQmlRequestHandler::setCachedContentValid(bool valid)
{
    QWriteLocker wl(&cachedContentMREW_);
    if (valid != cachedContentValid_)
    {
        cachedContentValid_ = valid;
        emit cachedContentValidChanged();
    }
}

QByteArray ViridityQmlRequestHandler::cachedContent() const
{
    QReadLocker rl(&cachedContentMREW_);
    return cachedContent_;
}

void ViridityQmlRequestHandler::setCachedContent(const QByteArray &content)
{
    QWriteLocker wl(&cachedContentMREW_);
    if (content != cachedContent_)
    {
        cachedContent_ = content;
        emit cachedContentChanged();
    }
}

QString ViridityQmlRequestHandler::handlesUrl() const
{
    QReadLocker rl(&cachedContentMREW_);
    return handlesUrl_;
}

void ViridityQmlRequestHandler::setHandlesUrl(const QString &regex)
{
    QWriteLocker wl(&cachedContentMREW_);
    if (regex != handlesUrl_)
    {
        handlesUrl_ = regex;
        emit handlesUrlChanged();
    }
}

ViridityHttpHeaders ViridityQmlRequestHandler::cachedHeaders()
{
    QReadLocker rl(&cachedContentMREW_);
    return cachedHeaders_;
}

void ViridityQmlRequestHandler::setCachedHeaders(ViridityHttpHeaders headers)
{
    QWriteLocker wl(&cachedContentMREW_);
    if (headers != cachedHeaders_)
    {
        cachedHeaders_ = headers;
#ifdef VIRIDITY_DEBUG
        cachedHeaders_.insert("X-Viridity-Cached", "true");
#endif
        emit cachedHeadersChanged();
    }
}

int ViridityQmlRequestHandler::cachedStatusCode() const
{
    QReadLocker rl(&cachedContentMREW_);
    return cachedStatusCode_;
}

void ViridityQmlRequestHandler::setCachedStatusCode(int statusCode)
{
    QWriteLocker wl(&cachedContentMREW_);
    if (statusCode != cachedStatusCode_)
    {
        cachedStatusCode_ = statusCode;
        emit cachedStatusCodeChanged();
    }
}

#include "viridityqmlrequesthandler.moc"
