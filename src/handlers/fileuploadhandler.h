#ifndef FILEUPLOADHANDLER_H
#define FILEUPLOADHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

#include "viridityrequesthandler.h"

class FileUploadHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit FileUploadHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~FileUploadHandler();

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

signals:
    void newFilesUploaded(const QVariantList &files);
};

#endif // FILEUPLOADHANDLER_H
