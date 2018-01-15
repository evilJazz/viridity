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
 * Helper class that contains methods used in the declarative/QML context.
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
#ifdef VIRIDITY_USE_QTQUICK1
    QDeclarativeEngine *engine();
#else
    QQmlEngine *engine();
#endif

    ViridityWebServer *webServer();
    AbstractViriditySessionManager *sessionManager();
    ViriditySession *currentSession();
};


/* ViridityQmlSessionWrapper */

class ViridityQmlSessionWrapper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QByteArray initialPeerAddress READ initialPeerAddress CONSTANT)
    Q_PROPERTY(QVariant userData READ userData WRITE setUserData NOTIFY userDataChanged)

    Q_PROPERTY(ViriditySession *nativeSession READ nativeSession CONSTANT)
public:
    explicit ViridityQmlSessionWrapper(ViriditySession *session, QObject *parent = 0);
    virtual ~ViridityQmlSessionWrapper();

    ViriditySession *nativeSession() const;

signals:
    void initialized();
    void attached();
    void interactionDormant();
    void releaseRequired();
    void detached();
    void deinitializing();
    void userDataChanged();

private:
    QPointer<ViriditySession> session_;

    const QString id() const;
    QByteArray initialPeerAddress() const;
    QVariant userData() const;
    void setUserData(const QVariant &userData);
};

#endif // VIRIDITYDECLARATIVE_H
