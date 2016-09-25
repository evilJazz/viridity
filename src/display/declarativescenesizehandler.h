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

#ifndef DECLARATIVESCENESIZEHANDLER_H
#define DECLARATIVESCENESIZEHANDLER_H

#include <QObject>

#include "graphicssceneadapter.h"
#include "viriditysessionmanager.h"

class DeclarativeSceneSizeHandler : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
public:
    explicit DeclarativeSceneSizeHandler(ViriditySession *session, const QString &id, AbstractGraphicsSceneAdapter *adapter, bool scaleItem = false, QObject *parent = 0);
    virtual ~DeclarativeSceneSizeHandler();

    QString id() const { return id_; }

protected:
    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

    Q_INVOKABLE bool localHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

signals:
    void resized(int width, int height, qreal ratio);

private slots:
    void handleSessionDestroyed();

private:
    ViriditySession *session_;
    QString id_;
    AbstractGraphicsSceneAdapter *adapter_;
    bool scaleItem_;
};

#endif // DECLARATIVESCENESIZEHANDLER_H
