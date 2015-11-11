#include "filerequesthandler.h"

#include <QFile>

#include "viriditywebserver.h"

QHash<QByteArray, QByteArray> FileRequestHandler::globalFileNames_;
QHash<QByteArray, QByteArray> FileRequestHandler::globalContentTypes_;

FileRequestHandler::FileRequestHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent)
{
}

FileRequestHandler::~FileRequestHandler()
{
}

void FileRequestHandler::publishFile(const QByteArray &url, const QByteArray &fileName, const QByteArray &mimeType)
{
    if (!fileNames_.contains(url))
    {
        fileNames_.insert(url, fileName);
        contentTypes_.insert(url, mimeType);
    }
}

void FileRequestHandler::unpublishFile(const QByteArray &url)
{
    fileNames_.remove(url);
    contentTypes_.remove(url);
}

void FileRequestHandler::publishFileGlobally(const QByteArray &url, const QByteArray &fileName, const QByteArray &mimeType)
{
    if (!globalFileNames_.contains(url))
    {
        globalFileNames_.insert(url, fileName);
        globalContentTypes_.insert(url, mimeType);
    }
}

void FileRequestHandler::unpublishFileGlobally(const QByteArray &url)
{
    globalFileNames_.remove(url);
    globalContentTypes_.remove(url);
}

bool FileRequestHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    QList<QByteArray> parts = request->url().split('?');
    return parts.count() > 0 && (fileNames_.contains(parts.at(0)) || globalFileNames_.contains(parts.at(0)));
}

void FileRequestHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
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
            response->writeHead(Tufao::HttpServerResponse::OK);
            response->headers().insert("Content-Type", contentType);
            ViridityConnection::addNoCachingResponseHeaders(response);
            response->end(file.readAll());
            return;
        }
    }

    response->writeHead(404);
    response->end("Not found");
}
