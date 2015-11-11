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

class WebSocketHandler;
class SSEHandler;
class LongPollingHandler;
class FileRequestHandler;
class SessionRoutingRequestHandler;

class ViridityWebServer;

class ViridityConnection : public QObject
{
    Q_OBJECT
public:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    explicit ViridityConnection(ViridityWebServer *parent, qintptr socketDescriptor);
#else
    explicit ViridityConnection(ViridityWebServer *parent, int socketDescriptor);
#endif

    virtual ~ViridityConnection();

    ViridityWebServer *server() { return server_; }

    static void addNoCachingResponseHeaders(Tufao::HttpServerResponse *response);
    static QByteArray getPeerAddressFromRequest(Tufao::HttpServerRequest *request);

public slots:
    void setupConnection();

private slots:
    void onRequestReady();
    void onUpgrade(const QByteArray &);

private:
    WebSocketHandler *webSocketHandler_;
    SSEHandler *sseHandler_;
    LongPollingHandler *longPollingHandler_;

    ViridityWebServer *server_;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    qintptr socketDescriptor_;
#else
    int socketDescriptor_;
#endif
};

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

protected:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    virtual void incomingConnection(qintptr handle);
#else
    virtual void incomingConnection(int handle);
#endif

    friend class ViridityConnection;
    virtual bool doesHandleRequest(Tufao::HttpServerRequest *request);
    virtual void handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response);

private slots:
    void newSessionCreated(ViriditySession *session);

private:
    ViriditySessionManager *sessionManager_;

    QList<QThread *> connectionThreads_;
    int incomingConnectionCount_;

    QList<QThread *> sessionThreads_;

    QList<ViridityRequestHandler *> requestHandlers_;
    FileRequestHandler *fileRequestHandler_;
    SessionRoutingRequestHandler *sessionRoutingRequestHandler_;
};

#endif // VIRIDITYWEBSERVER_H
