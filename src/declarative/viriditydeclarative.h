/****************************************************************************
**
** Copyright (C) 2012-2016 Andre Beckedorf, Meteora Softworks
** Contact: info@meteorasoftworks.com
**
** This file is part of Viridity
**
** $VIRIDITY_BEGIN_LICENSE:COMMERCIAL_AGPL$
**
** This library is licensed under either a separately available commercial
** license or the GNU Affero General Public License Version 3.0,
** published 19 November 2007.
**
** See https://www.gnu.org/licenses/agpl-3.0.html or LICENSE-agpl-3.0.txt for
** details.
**
** If you wish to use and distribute the Viridity library in your commercial
** product without making your sourcecode available to the public, please
** contact us for a commercial license at info@meteorasoftworks.com
**
** $VIRIDITY_END_LICENSE$
**
****************************************************************************/

#ifndef VIRIDITYDECLARATIVE_H
#define VIRIDITYDECLARATIVE_H

/*!
 * \defgroup viridqml Viridity QML components
 * These classes and components provide access to the C++ classes from within a declarative QML context.
 */


/*!
 * Helper class that contains methods used in the declarative/QML context.
 * \ingroup viridqml
 */

class ViridityDeclarative
{
public:
    /*!
     * Registers all types provided by Viridity with the QML engine.
     * Call this once before setting up the first instance of \a QDeclarativeEngine or \a QQmlEngine.
     */
    static void registerTypes();
};

#ifdef VIRIDITY_USE_QTQUICK1
#include <QDeclarativeEngine>
#include <QDeclarativeParserStatus>
#else
#include <QQmlEngine>
#include <QQmlParserStatus>
#endif

#include <QVariant>
#include <QPointer>

class ViridityWebServer;
class AbstractViriditySessionManager;
class ViriditySession;

class ViridityDeclarativeBaseObject :
    public QObject,
#ifdef VIRIDITY_USE_QTQUICK1
    public QDeclarativeParserStatus
#else
    public QQmlParserStatus
#endif
{
    Q_OBJECT
#ifdef VIRIDITY_USE_QTQUICK1
    Q_INTERFACES(QDeclarativeParserStatus)
#else
    Q_INTERFACES(QQmlParserStatus)
#endif
public:
    ViridityDeclarativeBaseObject(QObject *parent = NULL);
    virtual ~ViridityDeclarativeBaseObject();

    virtual void classBegin();
    virtual void componentComplete();

protected:
    /*! Returns the declarative engined instance this object lives in. */
#ifdef VIRIDITY_USE_QTQUICK1
    QDeclarativeEngine *engine();
#else
    QQmlEngine *engine();
#endif

    /*! Returns the ViridityWebServer instance associated with this object. */
    ViridityWebServer *webServer();

    /*! Returns the AbstractViriditySessionManager instance associated with this object. */
    AbstractViriditySessionManager *sessionManager();

    /*! Returns the ViriditySession instance this object was instantiated by. */
    ViriditySession *currentSession();
};


/*!
 * Wrapper class that encapsulates a native ViriditySession in a declarative QML context.
 * \ingroup viridqml
 */

/* ViridityQmlSessionWrapper */

class ViridityQmlSessionWrapper : public QObject
{
    Q_OBJECT
    /*! Specifies the identifier of the current session instance. */
    Q_PROPERTY(QString id READ id CONSTANT)

    /*! Specifies the initial peer address.
     * \sa AbstractViriditySessionManager::getNewSession()
     */
    Q_PROPERTY(QByteArray initialPeerAddress READ initialPeerAddress CONSTANT)

    /*! Allows access to custom variant data in the session.
     * \sa ViriditySession::userData, ViriditySession::setUserData
     */
    Q_PROPERTY(QVariant userData READ userData WRITE setUserData NOTIFY userDataChanged)

    /*! Returns the native session encapsulated by this QML session wrapper. */
    Q_PROPERTY(ViriditySession *nativeSession READ nativeSession CONSTANT)
public:
    explicit ViridityQmlSessionWrapper(ViriditySession *session, QObject *parent = 0);
    virtual ~ViridityQmlSessionWrapper();

    ViriditySession *nativeSession() const;

signals:
    /*!
     * Signal is emitted when the session was initialized and is ready for action.
     * This signal is emitted after the initial ViriditySession::attached()
     */
    void initialized();

    /*!
     * Signal is emitted when the session is attached to a remote client.
     * \note For long polling connections, i.e. connections handled by LongPollingHandler,
     * this signal can bounce due to the nature of the connection, especially with long-running or blocking operations on the client-side.
     *
     * \sa ViriditySession::isAttached
     */
    void attached();

    /*!
     * Signal is emitted when the session has not received any message from a client for some time.
     * Currently this interval is defined by AbstractViriditySessionManager::sessionTimeout()
     *
     * \sa ViriditySession::interactionCheckInterval, ViriditySession::lastUsed
     */
    void interactionDormant();

    /*!
     * Signal is emitted when the session requests all users to release this session.
     */
    void releaseRequired();

    /*!
     * Signal is emitted when the session is detached from a remote client.
     * \note For long polling connections, i.e. connections handled by LongPollingHandler,
     * this signal can bounce due to the nature of the connection, especially with long-running or blocking operations on the client-side.
     *
     * \sa ViriditySession::isAttached
     */
    void detached();

    /*!
     * Signal is emitted before tearing down the session, when no client is attached,
     * ViriditySession::useCount() is zero and AbstractViriditySessionManager::killExpiredSessions()
     * deems it is time to clean up this instance.
     */
    void deinitializing();

    /*!
     * Signal is emitted when the userData change.
     */
    void userDataChanged();

private:
    QPointer<ViriditySession> session_;

    const QString id() const;
    QByteArray initialPeerAddress() const;
    QVariant userData() const;
    void setUserData(const QVariant &userData);
};

#endif // VIRIDITYDECLARATIVE_H
