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

#include "declarativescenesizehandler.h"

#include <QStringList>
#include <QThread>

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

DeclarativeSceneSizeHandler::DeclarativeSceneSizeHandler(ViriditySession *session, const QString &id, AbstractGraphicsSceneAdapter *adapter, bool scaleItem, QObject *parent) :
    QObject(parent),
    session_(session),
    id_(id),
    adapter_(adapter),
    scaleItem_(scaleItem)
{
    DGUARDMETHODTIMED;
    if (session_)
    {
        session_->registerMessageHandler(this);
        connect(session_, SIGNAL(destroyed()), this, SLOT(handleSessionDestroyed()), Qt::DirectConnection);
    }
}

DeclarativeSceneSizeHandler::~DeclarativeSceneSizeHandler()
{
    DGUARDMETHODTIMED;
    if (session_)
        session_->unregisterMessageHandler(this);
}

void DeclarativeSceneSizeHandler::handleSessionDestroyed()
{
    session_ = NULL;
}

bool DeclarativeSceneSizeHandler::canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    return targetId == id_ && message.startsWith("resize");
}

bool DeclarativeSceneSizeHandler::handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    bool result;

    QMetaObject::invokeMethod(
        this, "localHandleMessage",
        this->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(bool, result),
        Q_ARG(const QByteArray &, message),
        Q_ARG(const QString &, sessionId),
        Q_ARG(const QString &, targetId)
    );

    return result;
}

bool DeclarativeSceneSizeHandler::localHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    bool result = false;

    QString command;
    QStringList params;

    ViridityMessageHandler::splitMessage(message, command, params);

    if (message.startsWith("resize") && params.count() == 3)
    {
        int width = params[0].toInt();
        int height = params[1].toInt();
        qreal ratio = params[2].toDouble();

        DPRINTF("Received new size: %d x %d, pixel ratio: %f", width, height, ratio);

        if (adapter_)
            adapter_->setSize(width, height, scaleItem_ ? ratio : 0.f);

        emit resized(width, height, ratio);

        result = true;
    }

    return result;
}
