#ifndef FILEUPLOADHANDLER_H
#define FILEUPLOADHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

class ViridityConnection;

class FileUploadHandler : public QObject
{
    Q_OBJECT
public:
    explicit FileUploadHandler(ViridityConnection *parent);

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private:
    ViridityConnection *connection_;
};

#endif // FILEUPLOADHANDLER_H
