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

#include "debugrequesthandler.h"

#include "viriditywebserver.h"
#include "viriditysessionmanager.h"

#include <QCoreApplication>
#include <QStringList>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QJsonDocument>
#endif

DebugRequestHandler::DebugRequestHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent)
{
}

DebugRequestHandler::~DebugRequestHandler()
{
}

bool DebugRequestHandler::doesHandleRequest(ViridityHttpServerRequest *request)
{
    return request->url().endsWith("/debug") || request->url().endsWith("/quit");
}

void DebugRequestHandler::handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
    response->writeHead(ViridityHttpServerResponse::OK);
    response->headers().insert("Content-Type", "text/plain");
    response->addNoCachingResponseHeaders();

    if (request->url().endsWith("/debug"))
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QVariant stats = server()->stats();
        response->write(QJsonDocument::fromVariant(stats).toJson(QJsonDocument::Indented) + "\n");
#else
        response->write("JSON output not supported on Qt 4.x\n");
#endif
    }
    else if (request->url().endsWith("/quit"))
    {
        response->write("Sent message to quit server.\n");
        QMetaObject::invokeMethod(qApp, "quit");
    }

    response->end();
}
