#include "filerequesthandler.h"

#include "viriditywebserver.h"
#include <QFile>

FileRequestHandler::FileRequestHandler(ViridityConnection *parent) :
    QObject(parent),
    connection_(parent)
{
}

void FileRequestHandler::insertFileInformation(const QByteArray &url, const QByteArray &fileName, const QByteArray &mimeType)
{
    fileNames_.insert(url, fileName);
    contentTypes_.insert(url, mimeType);
}

bool FileRequestHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    return fileNames_.contains(request->url());
}

void FileRequestHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    QFile file(fileNames_.value(request->url()));
    file.open(QIODevice::ReadOnly);

    response->writeHead(Tufao::HttpServerResponse::OK);
    response->headers().insert("Content-Type", contentTypes_.value(request->url()));
    response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
    response->headers().insert("Pragma", "no-cache");
    response->end(file.readAll());
}
