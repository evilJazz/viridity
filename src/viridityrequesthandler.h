/****************************************************************************
**
** Copyright (C) 2012-2016 Andre Beckedorf, Meteora Softworks
** Contact: info@meteorasoftworks.com
**
** This file is part of Viridity
**
** $VIRIDITY_BEGIN_LICENSE:COMMERCIAL_AGPL$
**
** This library is licensed under either a separately available commercial
** license or the GNU Affero General Public License Version 3.0,
** published 19 November 2007.
**
** See https://www.gnu.org/licenses/agpl-3.0.html or LICENSE-agpl-3.0.txt for
** details.
**
** If you wish to use and distribute the Viridity library in your commercial
** product without making your sourcecode available to the public, please
** contact us for a commercial license at info@meteorasoftworks.com
**
** $VIRIDITY_END_LICENSE$
**
****************************************************************************/

#ifndef VIRIDITYREQUESTHANDLER_H
#define VIRIDITYREQUESTHANDLER_H

#include <QObject>
#include <QSharedPointer>
#include <QAbstractSocket>
#include <QIODevice>
#include "Tufao/HttpServerRequest"
#include "Tufao/HttpServerResponse"
#include "Tufao/Headers"

/*!
    \addtogroup virid
    @{
*/

class ViridityWebServer;
class ViridityTcpSocket;

/*!
 * ViridityHttpServerRequest defines an incoming request from a peer/client/browser.
 * The current implementation is based on Tufao::HttpServerRequest and might be replaced in a source-compatible manner in the future.
 */

class ViridityHttpServerRequest : public Tufao::HttpServerRequest
{
    Q_OBJECT
public:
    explicit ViridityHttpServerRequest(QSharedPointer<ViridityTcpSocket> socket, QObject *parent = 0);
    virtual ~ViridityHttpServerRequest();

    /*!
     Extracts the peer's IP address from the request.
     \return The IP address as byte array.
     \sa ViriditySession::initialPeerAddress, AbstractViriditySessionManager::getNewSession
    */
    QByteArray getPeerAddressFromRequest() const;

    QSharedPointer<ViridityTcpSocket> socket() const { return socket_; }

protected:
    friend class ViridityConnection;
    static void sharedPointerDeleteLater(ViridityHttpServerRequest *request);

private:
    QSharedPointer<ViridityTcpSocket> socket_;
};

/*!
 * ViridityHttpServerResponse defines a response to a ViridityHttpServerRequest.
 * The current implementation is based on Tufao::HttpServerResponse and might be replaced in a source-compatible manner in the future.
 */

class ViridityHttpServerResponse : public Tufao::HttpServerResponse
{
    Q_OBJECT
public:
    explicit ViridityHttpServerResponse(QSharedPointer<ViridityTcpSocket> socket, Options options, QObject *parent = 0);
    virtual ~ViridityHttpServerResponse();

    /*!
     Adds fields to the response's headers that inform the client/browser to not cache any data.
     Commonly used in a custom class derived from ViridityRequestHandler to control caching behavior of content sent out.
     \param response The response instance provided by the ViridityRequestHandler::handleRequest method.
     \sa ViridityRequestHandler ViridityRequestHandler::handleRequest
    */
    void addNoCachingResponseHeaders();

    QSharedPointer<ViridityTcpSocket> socket() const { return socket_; }

protected:
    friend class ViridityConnection;
    static void sharedPointerDeleteLater(ViridityHttpServerResponse *response);

private:
    QSharedPointer<ViridityTcpSocket> socket_;
};

/*!
    Abstract class that provides the interface for handling requests in a ViridityWebServer instance.
    Custom request handler classes have to implement this interface. Instances of these custom classes should be registered with
    a running ViridityWebServer instance via ViridityWebServer::registerRequestHandler.
    For a simpler starting point consider using ViridityBaseRequestHandler as base class.
    \sa ViridityWebServer, ViridityBaseRequestHandler
*/

class ViridityRequestHandler
{
public:
    virtual ~ViridityRequestHandler() {}

    /*!
     * Unconditionally called to filter the request and possibly set up headers in the response.
     * \param request The request instance received from the client/browser via the ViridityWebServer.
     * \param response The pending response instance to the client/browser.
     */
    virtual void filterRequestResponse(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response) = 0;

    /*!
     * Determines whether this class can handle (or wishes to handle) the request.
     * \param request The request instance received from the client/browser via the ViridityWebServer.
     * \return Return true if this class can handle the request, false otherwise.
     */
    virtual bool doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request) = 0;

    /*! Called to handle an incoming request and send out the response.
     * \param request The request instance received from the client/browser via the ViridityWebServer.
     * \param response The pending response instance to the client/browser.
     */
    virtual void handleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response) = 0;
};

/*!
 * The ViridityBaseRequestHandler class should be used for custom classes that implement the ViridityRequestHandler interface.
 * \sa ViridityWebServer, ViridityBaseRequestHandler
 */

class ViridityBaseRequestHandler : public QObject, public ViridityRequestHandler
{
    Q_OBJECT
public:
    explicit ViridityBaseRequestHandler(ViridityWebServer *server, QObject *parent = NULL);
    virtual ~ViridityBaseRequestHandler();

    // ViridityRequestHandler
    virtual void filterRequestResponse(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response);
    virtual bool doesHandleRequest(QSharedPointer<ViridityHttpServerRequest> request);
    virtual void handleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response);

    ViridityWebServer *server() const { return server_; }
    bool handlingRequest() const { return handlingRequest_; }

protected:
    virtual void beginHandleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response);
    virtual void doHandleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response);
    virtual void endHandleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response);

private:
    ViridityWebServer *server_;
    bool handlingRequest_;
};

/*! @}*/

#endif // VIRIDITYREQUESTHANDLER_H
