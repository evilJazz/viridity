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

#ifndef FILEREQUESTHANDLER_H
#define FILEREQUESTHANDLER_H

#include <QObject>
#include <QHash>
#include <QByteArray>
#include <QString>

#include "viridityrequesthandler.h"

class FileRequestHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit FileRequestHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~FileRequestHandler();

    void publishFile(const QByteArray &url, const QString &fileName, QByteArray mimeType = QByteArray());
    void unpublishFile(const QByteArray &url);

    static void publishFileGlobally(const QByteArray &url, const QString &fileName, QByteArray mimeType = QByteArray());
    static void unpublishFileGlobally(const QByteArray &url);

    static void publishDirectoryGlobally(const QByteArray &baseUrl, const QString &directoryName, bool followSymLinks = false);
    static void unpublishDirectoryGlobally(const QByteArray &baseUrl);

    static void publishViridityFiles();

    static QByteArray determineMimeType(const QString &fileName);

    bool doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request);
    void handleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response);

private:
    QHash<QByteArray, QString> fileNames_;
    QHash<QByteArray, QByteArray> contentTypes_;

    static QHash<QByteArray, QString> globalFileNames_;
    static QHash<QByteArray, QByteArray> globalContentTypes_;
};

#endif // FILEREQUESTHANDLER_H
