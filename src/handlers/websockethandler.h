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

    bool doesHandleRequest(ViridityHttpServerRequest *request);
    void handleUpgrade(ViridityHttpServerRequest *request, const QByteArray &head);

private slots:
    void handleMessagesAvailable();
    void handleSessionInteractionDormant();

    void clientMessageReceived(QByteArray data);
    void clientDisconnected();

private:
    ViriditySession *session_;

    QAbstractSocket *socket_;
    Tufao::WebSocket *websocket_;
};

#endif // WEBSOCKETHANDLER_H
