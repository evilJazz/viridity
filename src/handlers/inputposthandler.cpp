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

#include "inputposthandler.h"

#include "viriditywebserver.h"
#include "viriditysessionmanager.h"

InputPostHandler::InputPostHandler(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response, ViriditySession *session, QObject *parent) :
    QObject(parent),
    request_(request),
    response_(response),
    session_(session)
{
    connect(request_, SIGNAL(data(QByteArray)), this, SLOT(onData(QByteArray)));
    connect(request_, SIGNAL(end()), this, SLOT(onEnd()));
}

void InputPostHandler::onData(const QByteArray &chunk)
{
    data_ += chunk;
}

void InputPostHandler::onEnd()
{
    QList<QByteArray> messages = data_.split('\n');

    foreach (const QByteArray &message, messages)
        session_->dispatchMessageToHandlers(message);

    // handle request
    response_->headers().insert("Content-Type", "text/plain");
    response_->writeHead(ViridityHttpServerResponse::OK);
    response_->end();
}
