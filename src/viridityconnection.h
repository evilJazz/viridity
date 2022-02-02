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

#ifndef VIRIDITYCONNECTION_H
#define VIRIDITYCONNECTION_H

#include <QObject>
#include <QTime>
#include <QTcpSocket>
#include <QReadWriteLock>
#include <QSharedPointer>

class WebSocketHandler;
class SSEHandler;
class LongPollingHandler;

class ViridityHttpServerRequest;
class ViridityHttpServerResponse;
class ViridityWebServer;
class ViridityConnection;

/*!
    \addtogroup virid
    @{
*/

/* ViridityTcpSocket */

class ViridityTcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit ViridityTcpSocket(QSharedPointer<ViridityConnection> connection);
    virtual ~ViridityTcpSocket();

protected:
    friend class ViridityConnection;
    static void sharedPointerDeleteLater(ViridityTcpSocket *socket);

private:
    QSharedPointer<ViridityConnection> connection_;
};


/* ViridityConnection */

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    typedef qintptr ViriditySocketDescriptor;
#else
    typedef int ViriditySocketDescriptor;
#endif

class ViridityConnection : public QObject
{
    Q_OBJECT
public:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    explicit ViridityConnection(ViridityWebServer *server, qintptr socketDescriptor);
#else
    explicit ViridityConnection(ViridityWebServer *server, int socketDescriptor);
#endif

    virtual ~ViridityConnection();

    ViriditySocketDescriptor socketDescriptor() const { return socketDescriptor_; }

    ViridityWebServer *server() { return server_; }

    /*!
     * Returns debug statistics and details of the current connection.
     * \sa ViridityWebServer::stats
     * \sa ViriditySession::stats
     */
    QVariant stats();

public slots:
    void setupConnection(QSharedPointer<ViridityConnection> reference);
    void close(QSharedPointer<ViridityConnection> reference = QSharedPointer<ViridityConnection>());

protected:
    friend class ViridityWebServer;
    static void sharedPointerDeleteLater(ViridityConnection *connection);

private slots:
    void handleRequestReady();
    void handleRequestUpgrade(const QByteArray &);
    void handleResponseFinished();
    void handleSocketDisconnected();

private:
    mutable QReadWriteLock sharedMemberMREW_;

    WebSocketHandler *webSocketHandler_;
    SSEHandler *sseHandler_;
    LongPollingHandler *longPollingHandler_;

    ViridityWebServer *server_;

    QSharedPointer<ViridityTcpSocket> socket_;
    QSharedPointer<ViridityHttpServerRequest> request_;
    QSharedPointer<ViridityHttpServerResponse> response_;

    ViriditySocketDescriptor socketDescriptor_;

    // Copied here because Tufao classes are not thread-safe internally
    QByteArray requestUrl_;
    QByteArray requestMethod_;

    QTime created_;
    QTime lastUsed_;
    int reUseCount_;
};

Q_DECLARE_METATYPE( QSharedPointer<ViridityConnection> )

/*! @} */

#endif // VIRIDITYCONNECTION_H
