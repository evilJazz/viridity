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

#ifndef LONGPOLLINGHANDLER_H
#define LONGPOLLINGHANDLER_H

#include <QObject>
#include <QPointer>
#include <QMutex>

#include "viridityrequesthandler.h"

class ViriditySession;

class LongPollingHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit LongPollingHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~LongPollingHandler();

    static bool staticDoesHandleRequest(ViridityWebServer *server, QSharedPointer<ViridityHttpServerRequest> request);

    bool doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request);
    void doHandleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response);

private slots:
    void handleSessionInteractionDormant();
    void handleSessionReleaseRequired();
    void handleMessagesAvailable();
    void handleSessionDestroyed();

private:
    QMutex sessionMutex_;
    QPointer<ViriditySession> session_;

    QSharedPointer<ViridityHttpServerResponse> response_;

    void pushMessageAndEnd(QSharedPointer<ViridityHttpServerResponse> response, const QByteArray &msg);
    void releaseSession();
};

#endif // LONGPOLLINGHANDLER_H
