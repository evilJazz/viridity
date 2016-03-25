#ifndef VIRIDITYWEBSERVER_H
#define VIRIDITYWEBSERVER_H

#include "viridity_global.h"

#include <QTimer>

#include <QtNetwork/QTcpSocket>
#include <Tufao/HttpServer>
#include <Tufao/WebSocket>
#include <QBuffer>
#include <QMutex>
#include <QWaitCondition>

#include <QUrl>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    #include <QUrlQuery>
    #define UrlQuery(...) QUrlQuery(QUrl(__VA_ARGS__))
#else
    #define UrlQuery(...) QUrl(__VA_ARGS__)
#endif

#include "viriditysessionmanager.h"

/*!
    \class ViridityWebServer
    \brief The ViridityWebServer class provides the basic multi-threaded & session-aware Viridity web server.

    \sa ViridityRequestHandler, ViriditySessionManager
*/

class ViridityWebServer : public QTcpServer, private ViridityRequestHandler
{
    Q_OBJECT
public:
    explicit ViridityWebServer(QObject *parent, ViriditySessionManager *sessionManager);
    virtual ~ViridityWebServer();

    bool listen(const QHostAddress &address, quint16 port, int threadsNumber);

    ViriditySessionManager *sessionManager();

    void registerRequestHandler(ViridityRequestHandler *handler);
    void unregisterRequestHandler(ViridityRequestHandler *handler);

    static void addNoCachingResponseHeaders(Tufao::HttpServerResponse *response);
    static QByteArray getPeerAddressFromRequest(Tufao::HttpServerRequest *request);

private slots:
    void newSessionCreated(ViriditySession *session);

private:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    virtual void incomingConnection(qintptr handle);
#else
    virtual void incomingConnection(int handle);
#endif

    friend class ViridityConnection;
    virtual bool doesHandleRequest(Tufao::HttpServerRequest *request);
    virtual void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private:
    ViriditySessionManager *sessionManager_;

    QList<QThread *> connectionThreads_;
    int incomingConnectionCount_;

    QList<QThread *> sessionThreads_;

    QList<ViridityRequestHandler *> requestHandlers_;
    ViridityRequestHandler *fileRequestHandler_;
    ViridityRequestHandler *sessionRoutingRequestHandler_;
};

#endif // VIRIDITYWEBSERVER_H
