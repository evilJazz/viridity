#ifndef FILEREQUESTHANDLER_H
#define FILEREQUESTHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

#include "viridityrequesthandler.h"

class FileRequestHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit FileRequestHandler(ViridityConnection *parent);
    virtual ~FileRequestHandler();

    void publishFile(const QByteArray &url, const QByteArray &fileName, const QByteArray &mimeType);
    void unpublishFile(const QByteArray &url);

    static void publishFileGlobally(const QByteArray &url, const QByteArray &fileName, const QByteArray &mimeType);
    static void unpublishFileGlobally(const QByteArray &url);

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private:
    QHash<QByteArray, QByteArray> fileNames_;
    QHash<QByteArray, QByteArray> contentTypes_;

    static QHash<QByteArray, QByteArray> globalFileNames_;
    static QHash<QByteArray, QByteArray> globalContentTypes_;
};

#endif // FILEREQUESTHANDLER_H
