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

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

#include "viridityconnection.h"

/* ViridityHttpServerRequest */

ViridityHttpServerRequest::ViridityHttpServerRequest(QSharedPointer<ViridityTcpSocket> socket, QObject *parent) :
    Tufao::HttpServerRequest(socket.data(), parent),
    socket_(socket)
{
    DGUARDMETHODTIMED;
}

ViridityHttpServerRequest::~ViridityHttpServerRequest()
{
    DGUARDMETHODTIMED;
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

void ViridityHttpServerRequest::sharedPointerDeleteLater(ViridityHttpServerRequest *request)
{
    request->deleteLater();
}

/* ViridityHttpServerResponse */

ViridityHttpServerResponse::ViridityHttpServerResponse(QSharedPointer<ViridityTcpSocket> socket, Tufao::HttpServerResponse::Options options, QObject *parent) :
    Tufao::HttpServerResponse(socket.data(), options, parent),
    socket_(socket)
{
    DGUARDMETHODTIMED;
}

ViridityHttpServerResponse::~ViridityHttpServerResponse()
{
    DGUARDMETHODTIMED;
}

void ViridityHttpServerResponse::addNoCachingResponseHeaders()
{
    headers().insert("Cache-Control", "no-store, no-cache, must-revalidate");
    headers().insert("Pragma", "no-cache");
    headers().insert("Expires", "0");
}

void ViridityHttpServerResponse::sharedPointerDeleteLater(ViridityHttpServerResponse *response)
{
    response->deleteLater();
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

bool ViridityBaseRequestHandler::doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request)
{
    return false;
}

void ViridityBaseRequestHandler::handleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
{
    beginHandleRequest(request, response);
    doHandleRequest(request, response);
    endHandleRequest(request, response);
}

void ViridityBaseRequestHandler::beginHandleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
{
    handlingRequest_ = true;
}

void ViridityBaseRequestHandler::doHandleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
{
}

void ViridityBaseRequestHandler::endHandleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
{
    handlingRequest_ = false;
}
