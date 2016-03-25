#ifndef VIRIDITYREQUESTHANDLER_H
#define VIRIDITYREQUESTHANDLER_H

#include <QObject>
#include "Tufao/HttpServerRequest"
#include "Tufao/HttpServerResponse"
#include "Tufao/Headers"

class ViridityWebServer;

/*! ViridityHttpServerRequest defines an incoming request from a peer/client/browser. The current implementation is based on Tufao::HttpServerRequest and might be replaced in a compatible manner in the future. */

class ViridityHttpServerRequest : public Tufao::HttpServerRequest
{
    Q_OBJECT
public:
    explicit ViridityHttpServerRequest(QAbstractSocket *socket, QObject *parent = 0);
    virtual ~ViridityHttpServerRequest();

    /*!
     Extracts the peer's IP address from the request.
     \return The IP address as byte array.
    */
    QByteArray getPeerAddressFromRequest() const;
};

/*! ViridityHttpServerResponse defines a response to a ViridityHttpServerRequest. The current implementation is based on Tufao::HttpServerResponse and might be replaced in a compatible manner in the future. */

class ViridityHttpServerResponse : public Tufao::HttpServerResponse
{
    Q_OBJECT
public:
    explicit ViridityHttpServerResponse(QIODevice *device, Options options, QObject *parent = 0);
    virtual ~ViridityHttpServerResponse();

    /*!
     Adds fields to the response's headers that inform the client/browser to not cache any data.
     Commonly used in a custom class derived from ViridityRequestHandler to control caching behavior of content sent out.
     \param response The response instance provided by the ViridityRequestHandler::handleRequest method.
     \sa ViridityRequestHandler ViridityRequestHandler::handleRequest
    */
    void addNoCachingResponseHeaders();
};

/*!
    Abstract class that provides the interface for handling requests in a ViridityWebServer instance.
    Custom request handler classes have to implement this interface. Instances of these custom classes should be registered with a running ViridityWebServer instance via ViridityWebServer::registerRequestHandler().
    For a simpler starting point consider using ViridityBaseRequestHandler as base class.
    \sa ViridityWebServer, ViridityBaseRequestHandler
*/

class ViridityRequestHandler
{
public:
    virtual ~ViridityRequestHandler() {}

    /*!
     * Determines whether this class can handle (or wants to) the request.
     * \param request The request instance received from the client/browser via the ViridityWebServer.
     * \return Return true if this class can handle the request, false otherwise.
     */
    virtual bool doesHandleRequest(ViridityHttpServerRequest *request) = 0;

    /*! Called to handle an incoming request and send out the response. */
    virtual void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response) = 0;
};

class ViridityBaseRequestHandler : public QObject, public ViridityRequestHandler
{
    Q_OBJECT
public:
    explicit ViridityBaseRequestHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~ViridityBaseRequestHandler();

    // ViridityRequestHandler
    virtual bool doesHandleRequest(ViridityHttpServerRequest *request);
    virtual void handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response);

    ViridityWebServer *server() const { return server_; }

protected:
    ViridityWebServer *server_;
};

#endif // VIRIDITYREQUESTHANDLER_H
