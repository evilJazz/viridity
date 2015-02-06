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
    explicit FileUploadHandler(ViridityConnection *parent);
    virtual ~FileUploadHandler();

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);
};

#endif // FILEUPLOADHANDLER_H
