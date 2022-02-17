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

#include "filerequesthandler.h"

#include <QFile>
#include <QFileInfo>
#include <QDirIterator>

#include "KCL/filesystemutils.h"

#include "viriditywebserver.h"

QHash<QByteArray, QString> FileRequestHandler::globalFileNames_;
QHash<QByteArray, QByteArray> FileRequestHandler::globalContentTypes_;

FileRequestHandler::FileRequestHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent)
{
}

FileRequestHandler::~FileRequestHandler()
{
}

void FileRequestHandler::publishFile(const QByteArray &url, const QString &fileName, QByteArray mimeType)
{
    if (!fileNames_.contains(url))
    {
        if (mimeType.isEmpty())
            mimeType = determineMimeType(fileName);

        fileNames_.insert(url, fileName);
        contentTypes_.insert(url, mimeType);
    }
}

void FileRequestHandler::unpublishFile(const QByteArray &url)
{
    fileNames_.remove(url);
    contentTypes_.remove(url);
}

void FileRequestHandler::publishFileGlobally(const QByteArray &url, const QString &fileName, QByteArray mimeType)
{
    if (!globalFileNames_.contains(url))
    {
        if (mimeType.isEmpty())
            mimeType = determineMimeType(fileName);

        globalFileNames_.insert(url, fileName);
        globalContentTypes_.insert(url, mimeType);
    }
}

void FileRequestHandler::unpublishFileGlobally(const QByteArray &url)
{
    globalFileNames_.remove(url);
    globalContentTypes_.remove(url);
}

void FileRequestHandler::publishDirectoryGlobally(const QByteArray &baseUrl, const QString &directoryName, bool followSymLinks)
{
    QDir docDir(directoryName);

    if (docDir.exists())
    {
        QDirIterator it(docDir.absolutePath(), QDir::Files, followSymLinks ? QDirIterator::FollowSymlinks | QDirIterator::Subdirectories : QDirIterator::Subdirectories);

        while (it.hasNext())
        {
            QString fileName = it.next();;
            QString relFilePath = docDir.relativeFilePath(fileName);
            QString url = FileSystemUtils::pathAppend(baseUrl, relFilePath);
            publishFileGlobally(url.toUtf8(), fileName.toUtf8());
        }
    }
}

void FileRequestHandler::unpublishDirectoryGlobally(const QByteArray &baseUrl)
{

}

void FileRequestHandler::publishViridityFiles()
{
    publishFileGlobally("/Viridity.js", ":/Client/Viridity.js", "application/javascript; charset=utf-8");
    publishFileGlobally("/DataBridge.js", ":/Client/DataBridge.js", "application/javascript; charset=utf-8");
    publishFileGlobally("/DisplayRenderer.js", ":/Client/DisplayRenderer.js", "application/javascript; charset=utf-8");
    publishFileGlobally("/DocumentRenderer.js", ":/Client/DocumentRenderer.js", "application/javascript; charset=utf-8");
    publishFileGlobally("/ViridityAuto.js", ":/Client/ViridityAuto.js", "application/javascript; charset=utf-8");
    publishFileGlobally("/jquery.mousewheel.js", ":/Client/jquery.mousewheel.js", "application/javascript; charset=utf-8");
}

QByteArray FileRequestHandler::determineMimeType(const QString &fileName)
{
    QByteArray ext = QFileInfo(fileName).suffix().toLower().toLatin1();
    if (ext == "js")
        return "application/javascript";
    else if (ext == "css")
        return "text/css";
    else if (ext == "jpeg" || ext == "jpg")
        return "image/jpeg";
    else if (ext == "png")
        return "image/png";
    else if (ext == "svg")
        return "image/svg+xml";
    else if (ext == "html" || ext == "htm")
        return "text/html";
    else if (ext == "css")
        return "text/css";
    else
        return "application/octet-stream";
}

bool FileRequestHandler::doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request)
{
    QList<QByteArray> parts = request->url().split('?');
    return parts.count() > 0 && (fileNames_.contains(parts.at(0)) || globalFileNames_.contains(parts.at(0)));
}

void FileRequestHandler::handleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
{
    QList<QByteArray> parts = request->url().split('?');

    QString filename = QString::fromUtf8(parts.at(0));
    QString localFileName = fileNames_.value(filename.toUtf8());
    QByteArray contentType = contentTypes_.value(request->url());

    if (localFileName.isEmpty())
    {
        localFileName = globalFileNames_.value(filename.toUtf8());
        contentType = globalContentTypes_.value(filename.toUtf8());
    }

    if (!localFileName.isEmpty())
    {
        QFile file(localFileName);
        if (file.open(QIODevice::ReadOnly))
        {
            response->writeHead(ViridityHttpServerResponse::OK);
            response->headers().insert("Content-Type", contentType);
            response->addNoCachingResponseHeaders();
            response->end(file.readAll());
            return;
        }
    }

    response->writeHead(404);
    response->end("Not found");
}
