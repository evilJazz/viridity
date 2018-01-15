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

#include "viriditydeclarative.h"

#include "viriditysessionmanager.h"
#include "viriditywebserver.h"
#include "viridityqmlrequesthandler.h"
#include "viriditydatabridge.h"

#ifdef VIRIDITY_USE_QTQUICK1
    #include <QtDeclarative>
    #include <QDeclarativeEngine>
    #include <QDeclarativeContext>
    #include <QDeclarativeComponent>
    typedef QDeclarativeEngine DeclarativeEngine;
    typedef QDeclarativeContext DeclarativeContext;
    typedef QDeclarativeComponent DeclarativeComponent;
#else
    #include <QtQml>
    #include <QQmlEngine>
    #include <QQmlContext>
    #include <QQmlComponent>
    typedef QQmlEngine DeclarativeEngine;
    typedef QQmlContext DeclarativeContext;
    typedef QQmlComponent DeclarativeComponent;
#endif

#ifdef VIRIDITY_MODULE_DISPLAY
    #include "display/viridityqtquickdisplay.h"
#endif

#include "KCL/objectutils.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

/* ViridityDeclarative */

void ViridityDeclarative::registerTypes()
{
    qmlRegisterType<ViridityQmlRequestHandler>("Viridity", 1, 0, "ViridityRequestHandler");
    qmlRegisterType<ViridityNativeDataBridge>("Viridity", 1, 0, "ViridityNativeDataBridge");
    qmlRegisterUncreatableType<ViriditySession>("Viridity", 1, 0, "ViriditySession", "Can't create a session out of thin air.");

#ifdef VIRIDITY_MODULE_DISPLAY
    qmlRegisterType<ViridityQtQuickDisplay>("Viridity", 1, 0, "ViridityDisplay");
#endif
}

/* ViridityDeclarativeBaseObject */

ViridityDeclarativeBaseObject::ViridityDeclarativeBaseObject(QObject *parent) :
    QObject(parent)
{
    DGUARDMETHODTIMED;
}

ViridityDeclarativeBaseObject::~ViridityDeclarativeBaseObject()
{
    DGUARDMETHODTIMED;
}

void ViridityDeclarativeBaseObject::classBegin()
{
    DGUARDMETHODTIMED;
}

void ViridityDeclarativeBaseObject::componentComplete()
{
    DGUARDMETHODTIMED;
}

DeclarativeEngine *ViridityDeclarativeBaseObject::engine()
{
    DeclarativeContext *context = DeclarativeEngine::contextForObject(this);

    if (context && context->engine())
    {
        if (DeclarativeEngine::objectOwnership(context->engine()) != DeclarativeEngine::CppOwnership)
            DeclarativeEngine::setObjectOwnership(context->engine(), DeclarativeEngine::CppOwnership);

        return context->engine();
    }

    return NULL;
}

ViridityWebServer *ViridityDeclarativeBaseObject::webServer()
{
    AbstractViriditySessionManager *sessionManager = this->sessionManager();

    if (sessionManager)
    {
        ViridityWebServer *server = sessionManager->server();

        if (server)
        {
            if (DeclarativeEngine::objectOwnership(server) != DeclarativeEngine::CppOwnership)
                DeclarativeEngine::setObjectOwnership(server, DeclarativeEngine::CppOwnership);

            return server;
        }
    }

    return NULL;
}

AbstractViriditySessionManager *ViridityDeclarativeBaseObject::sessionManager()
{
    DeclarativeContext *context = DeclarativeEngine::contextForObject(this);

    if (context)
    {
        // Works for sessionLogic and globalLogic!
        QObject *obj = ObjectUtils::objectify(context->contextProperty("sessionManager"));
        AbstractViriditySessionManager *sessionManager = qobject_cast<AbstractViriditySessionManager *>(obj);

        if (sessionManager)
        {
            if (DeclarativeEngine::objectOwnership(sessionManager) != DeclarativeEngine::CppOwnership)
                DeclarativeEngine::setObjectOwnership(sessionManager, DeclarativeEngine::CppOwnership);

            return sessionManager;
        }
    }

    return NULL;
}

ViriditySession *ViridityDeclarativeBaseObject::currentSession()
{
    DeclarativeContext *context = DeclarativeEngine::contextForObject(this);

    if (context)
    {
        // Works only in sessionLogic!
        QObject *obj = ObjectUtils::objectify(context->contextProperty("currentSession"));
        ViridityQmlSessionWrapper *sessionWrapper = qobject_cast<ViridityQmlSessionWrapper *>(obj);

        if (sessionWrapper)
        {
            ViriditySession *session = sessionWrapper->nativeSession();

            if (DeclarativeEngine::objectOwnership(session) != DeclarativeEngine::CppOwnership)
                DeclarativeEngine::setObjectOwnership(session, DeclarativeEngine::CppOwnership);

            return session;
        }
    }

    return NULL;
}


/* ViridityQmlSessionWrapper */

ViridityQmlSessionWrapper::ViridityQmlSessionWrapper(ViriditySession *session, QObject *parent) :
    QObject(parent),
    session_(session)
{
    DGUARDMETHODTIMED;
    connect(session, SIGNAL(initialized()), this, SIGNAL(initialized()));
    connect(session, SIGNAL(attached()), this, SIGNAL(attached()));
    connect(session, SIGNAL(interactionDormant()), this, SIGNAL(interactionDormant()));
    connect(session, SIGNAL(releaseRequired()), this, SIGNAL(releaseRequired()));
    connect(session, SIGNAL(detached()), this, SIGNAL(detached()));
    connect(session, SIGNAL(deinitializing()), this, SIGNAL(deinitializing()));
    connect(session, SIGNAL(userDataChanged()), this, SIGNAL(userDataChanged()));
}

ViridityQmlSessionWrapper::~ViridityQmlSessionWrapper()
{
    DGUARDMETHODTIMED;
}

ViriditySession *ViridityQmlSessionWrapper::nativeSession() const
{
    return session_;
}

const QString ViridityQmlSessionWrapper::id() const
{
    return session_ ? session_->id() : QString();
}

QByteArray ViridityQmlSessionWrapper::initialPeerAddress() const
{
    return session_ ? session_->initialPeerAddress() : QByteArray();
}

QVariant ViridityQmlSessionWrapper::userData() const
{
    return session_ ? session_->userData() : QVariant();
}

void ViridityQmlSessionWrapper::setUserData(const QVariant &userData)
{
    if (session_) session_->setUserData(userData);
}
