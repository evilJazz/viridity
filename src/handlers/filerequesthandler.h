#ifndef FILEREQUESTHANDLER_H
#define FILEREQUESTHANDLER_H

#include <QObject>
#include <QHash>
#include <QByteArray>
#include <QString>

#include "viridityrequesthandler.h"

class FileRequestHandler : public ViridityBaseRequestHandler
{
    Q_OBJECT
public:
    explicit FileRequestHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~FileRequestHandler();

    void publishFile(const QByteArray &url, const QString &fileName, QByteArray mimeType = QByteArray());
    void unpublishFile(const QByteArray &url);

    static void publishFileGlobally(const QByteArray &url, const QString &fileName, QByteArray mimeType = QByteArray());
    static void unpublishFileGlobally(const QByteArray &url);

    static void publishDirectoryGlobally(const QByteArray &baseUrl, const QString &directoryName);
    static void unpublishDirectoryGlobally(const QByteArray &baseUrl);

    static void publishViridityFiles();

    static QByteArray determineMimeType(const QString &fileName);

    bool doesHandleRequest(ViridityHttpServerRequest *request);
    void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);

private:
    QHash<QByteArray, QString> fileNames_;
    QHash<QByteArray, QByteArray> contentTypes_;

    static QHash<QByteArray, QString> globalFileNames_;
    static QHash<QByteArray, QByteArray> globalContentTypes_;
};

#endif // FILEREQUESTHANDLER_H
