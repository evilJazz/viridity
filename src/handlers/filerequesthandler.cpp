#include "filerequesthandler.h"

#include "viriditywebserver.h"
#include <QFile>

QHash<QByteArray, QByteArray> FileRequestHandler::globalFileNames_;
QHash<QByteArray, QByteArray> FileRequestHandler::globalContentTypes_;

FileRequestHandler::FileRequestHandler(ViridityConnection *parent) :
    QObject(parent),
    connection_(parent)
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
    QUrl url(request->url());
    QString filename = url.path();
    return fileNames_.contains(filename.toUtf8()) || globalFileNames_.contains(filename.toUtf8());
}

void FileRequestHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    QUrl url(request->url());
    QString filename = url.path();
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
        file.open(QIODevice::ReadOnly);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", contentType);
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(file.readAll());
    }
    else
    {
        response->writeHead(404);
        response->end("Not found");
    }
}
