#ifndef VIRIDITYREQUESTHANDLER_H
#define VIRIDITYREQUESTHANDLER_H

#include <QObject>
#include <Tufao/HttpServer>

class ViridityWebServer;

class ViridityRequestHandler
{
public:
    virtual ~ViridityRequestHandler() {}
    virtual bool doesHandleRequest(Tufao::HttpServerRequest *request) = 0;
    virtual void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response) = 0;
};

class ViridityBaseRequestHandler : public QObject, public ViridityRequestHandler
{
    Q_OBJECT
public:
    explicit ViridityBaseRequestHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~ViridityBaseRequestHandler();

    // ViridityRequestHandler
    virtual bool doesHandleRequest(Tufao::HttpServerRequest *request);
    virtual void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

    ViridityWebServer *server() const { return server_; }

protected:
    ViridityWebServer *server_;
};

#endif // VIRIDITYREQUESTHANDLER_H
