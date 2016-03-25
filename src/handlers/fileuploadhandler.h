#ifndef FILEUPLOADHANDLER_H
#define FILEUPLOADHANDLER_H

#include <QObject>

#include "viridityrequesthandler.h"

class ViriditySession;

class FileUploadHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit FileUploadHandler(ViridityWebServer *server, QObject *parent = NULL);
    explicit FileUploadHandler(ViriditySession *session, QObject *parent = NULL);
    virtual ~FileUploadHandler();

    bool doesHandleRequest(ViridityHttpServerRequest *request);
    void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);

signals:
    friend class FileUploadDataHandler;
    void newFilesUploaded(const QVariantList &files);

private slots:
    void handleSessionDestroyed();

private:
    ViriditySession *session_;
};

#endif // FILEUPLOADHANDLER_H
