#ifndef VIRIDITYQMLREQUESTHANDLER_H
#define VIRIDITYQMLREQUESTHANDLER_H

#include <QObject>
#include <QReadWriteLock>

#include "viriditydeclarative.h"
#include "viridityrequesthandler.h"

class PrivateQmlRequestHandler;

/*!
 * ViridityQmlRequestHandler allows writing request handlers in QML.
 * Evaluation happens in the main thread contrary to using a native C++ based ViridityBaseRequestHandler implementation.
 * It provides optional content caching, which is multi-threaded and boosts performance compared to uncached contents.
 * \ingroup viridqml
 */

class ViridityQmlRequestHandler : public ViridityDeclarativeBaseObject
{
    Q_OBJECT
    /*! Determines whether content caching is enabled or not. */
    Q_PROPERTY(bool contentCachingEnabled READ contentCachingEnabled WRITE setContentCachingEnabled NOTIFY contentCachingEnabledChanged)
    /*! Determines whether the content in the cache is still valid. */
    Q_PROPERTY(bool cachedContentValid READ cachedContentValid WRITE setCachedContentValid NOTIFY cachedContentValidChanged)
    /*! Contains the cached content. */
    Q_PROPERTY(QByteArray cachedContent READ cachedContent WRITE setCachedContent NOTIFY cachedContentChanged)
    Q_PROPERTY(int cachedStatusCode READ cachedStatusCode WRITE setCachedStatusCode NOTIFY cachedStatusCodeChanged)
    Q_PROPERTY(QString handlesUrl READ handlesUrl WRITE setHandlesUrl NOTIFY handlesUrlChanged)
public:
    ViridityQmlRequestHandler(QObject *parent = NULL);
    virtual ~ViridityQmlRequestHandler();

    virtual void componentComplete();

    bool contentCachingEnabled() const;
    void setContentCachingEnabled(bool enabled);

    bool cachedContentValid() const;
    void setCachedContentValid(bool valid);

    QByteArray cachedContent() const;
    void setCachedContent(const QByteArray &content);

    QString handlesUrl() const;
    void setHandlesUrl(const QString &regex);

    ViridityHttpHeaders cachedHeaders();
    void setCachedHeaders(ViridityHttpHeaders headers);

    int cachedStatusCode() const;
    void setCachedStatusCode(int statusCode);

signals:
    void contentCachingEnabledChanged();
    void cachedContentValidChanged();
    void cachedContentChanged();
    void cachedHeadersChanged();
    void cachedStatusCodeChanged();
    void handlesUrlChanged();

private:
    QSharedPointer<PrivateQmlRequestHandler> requestHandler_;

    mutable QReadWriteLock cachedContentMREW_;
    bool contentCachingEnabled_;
    bool cachedContentValid_;
    QByteArray cachedContent_;
    int cachedStatusCode_;
    ViridityHttpHeaders cachedHeaders_;

    QString handlesUrl_;
};

#endif // VIRIDITYQMLREQUESTHANDLER_H
