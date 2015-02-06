#ifndef VIRIDITYREQUESTHANDLER_H
#define VIRIDITYREQUESTHANDLER_H

#include <QObject>
#include <Tufao/HttpServer>

class ViridityConnection;

class ViridityRequestHandler
{
public:
    virtual ~ViridityRequestHandler() {}
    virtual bool doesHandleRequest(Tufao::HttpServerRequest *request) = 0;
    virtual void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response) = 0;
};

class ViridityBaseRequestHandler : public QObject
{
    Q_OBJECT
public:
    explicit ViridityBaseRequestHandler(ViridityConnection *parent);
    virtual ~ViridityBaseRequestHandler();

    // ViridityRequestHandler
    virtual bool doesHandleRequest(Tufao::HttpServerRequest *request);
    virtual void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

protected:
    ViridityConnection *connection_;
};

#endif // VIRIDITYREQUESTHANDLER_H
