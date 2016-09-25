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

#include "viridityrequesthandler.h"

#include "viriditywebserver.h"

/* ViridityHttpServerRequest */

ViridityHttpServerRequest::ViridityHttpServerRequest(QAbstractSocket *socket, QObject *parent) :
    Tufao::HttpServerRequest(socket, parent)
{
}

ViridityHttpServerRequest::~ViridityHttpServerRequest()
{
}

QByteArray ViridityHttpServerRequest::getPeerAddressFromRequest() const
{
    if (!socket())
        return QByteArray();

    QByteArray peerAddress = socket()->peerAddress().toString().toLatin1();
    if (headers().contains("X-Forwarded-For"))
        peerAddress = headers().value("X-Forwarded-For");

    return peerAddress;
}

/* ViridityHttpServerResponse */

ViridityHttpServerResponse::ViridityHttpServerResponse(QIODevice *device, Tufao::HttpServerResponse::Options options, QObject *parent) :
    Tufao::HttpServerResponse(device, options, parent)
{
}

ViridityHttpServerResponse::~ViridityHttpServerResponse()
{
}

void ViridityHttpServerResponse::addNoCachingResponseHeaders()
{
    headers().insert("Cache-Control", "no-store, no-cache, must-revalidate");
    headers().insert("Pragma", "no-cache");
    headers().insert("Expires", "0");
}

/* ViridityBaseRequestHandler */

ViridityBaseRequestHandler::ViridityBaseRequestHandler(ViridityWebServer *server, QObject *parent) :
    QObject(parent),
    server_(server),
    handlingRequest_(false)
{
}

ViridityBaseRequestHandler::~ViridityBaseRequestHandler()
{
}

bool ViridityBaseRequestHandler::doesHandleRequest(ViridityHttpServerRequest *request)
{
    return false;
}

void ViridityBaseRequestHandler::handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    beginHandleRequest(request, response);
    doHandleRequest(request, response);
    endHandleRequest(request, response);
}

void ViridityBaseRequestHandler::beginHandleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    handlingRequest_ = true;
}

void ViridityBaseRequestHandler::doHandleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
}

void ViridityBaseRequestHandler::endHandleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    handlingRequest_ = false;
}
