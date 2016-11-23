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

#ifndef FILEUPLOADHANDLER_H
#define FILEUPLOADHANDLER_H

#include <QObject>
#include <QVariantList>
#include <QRegExp>

#include "viridityrequesthandler.h"

class ViriditySession;

class FileUploadHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit FileUploadHandler(const QString &urlEndPoint, ViridityWebServer *server, QObject *parent = NULL);
    virtual ~FileUploadHandler();

    bool doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request);
    void handleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response);

signals:
    friend class FileUploadDataHandler;
    void newFilesUploaded(const QVariantList &files);

private:
    QRegExp urlRe_;
};

#endif // FILEUPLOADHANDLER_H
