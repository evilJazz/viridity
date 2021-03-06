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

#ifndef WEBSOCKETHANDLER_H
#define WEBSOCKETHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QPointer>
#include <QMutex>

#include "viridityrequesthandler.h"

class ViriditySession;

namespace Tufao {
    class WebSocket;
}

class WebSocketHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit WebSocketHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~WebSocketHandler();

    bool doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request);
    void handleUpgrade(QSharedPointer<ViridityHttpServerRequest> request, const QByteArray &head);

private slots:
    void handleMessagesAvailable();
    void handleSessionInteractionDormant();
    void handleSessionDestroyed();

    void clientMessageReceived(QByteArray data);
    void clientDisconnected();
    void close();

private:
    QMutex sessionMutex_;
    QPointer<ViriditySession> session_;

    QSharedPointer<ViridityTcpSocket> socket_;
    Tufao::WebSocket *websocket_;
};

#endif // WEBSOCKETHANDLER_H
