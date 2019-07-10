/****************************************************************************
**
** Copyright (C) 2012-2019 Andre Beckedorf, Meteora Softworks
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

#include "posthandler.h"

#include "viriditywebserver.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

PostHandler::PostHandler(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response, QObject *parent) :
    QObject(parent),
    request_(request),
    response_(response),
    data_(),
    finished_(false)
{
    DGUARDMETHODTIMED;

    connect(request_.data(), SIGNAL(data(QByteArray)), this, SLOT(handleRequestData(QByteArray)));
    connect(request_.data(), SIGNAL(end()), this, SLOT(handleRequestEnd()));
    connect(request_.data(), SIGNAL(close()), this, SLOT(handleRequestClose()));
}

PostHandler::~PostHandler()
{
    DGUARDMETHODTIMED;
}

void PostHandler::handleRequestData(const QByteArray &chunk)
{
    data_ += chunk;

    emit dataReceived(chunk);
}

void PostHandler::handleRequestEnd()
{
    finished_ = true;
    emit dataFinished();

    handleRequestClose();
}

void PostHandler::handleRequestClose()
{
    response_.clear();
    request_.clear();

    deleteLater();
}
