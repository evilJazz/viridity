#ifndef FILEREQUESTHANDLER_H
#define FILEREQUESTHANDLER_H

#include <QObject>

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

class ViridityConnection;
class GraphicsSceneDisplay;

class FileRequestHandler : public QObject
{
    Q_OBJECT
public:
    explicit FileRequestHandler(ViridityConnection *parent);

    void insertFileInformation(const QByteArray &url, const QByteArray &fileName, const QByteArray &mimeType);

    bool doesHandleRequest(Tufao::HttpServerRequest *request);
    void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private:
    ViridityConnection *connection_;

    QHash<QByteArray, QByteArray> fileNames_;
    QHash<QByteArray, QByteArray> contentTypes_;
};

#endif // FILEREQUESTHANDLER_H
