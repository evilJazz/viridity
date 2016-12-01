#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QObject>

#include "KCL/webcall.h"

#include "viriditywebserver.h"
#include "handlers/debugrequesthandler.h"

class ClientServerTest : public QObject
{
    Q_OBJECT

public:
    ClientServerTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void stressWebCall404WithKeepAlive();
    void stressWebCall404WithoutKeepAlive();

    void stressWebCallOnDebugWithKeepAlive();
    void stressWebCallOnDebugWithoutKeepAlive();

    void benchmarkWebCall404WithKeepAlive();
    void benchmarkWebCall404WithoutKeepAlive();

    void benchmarkWebCallOnDebugWithKeepAlive();
    void benchmarkWebCallOnDebugWithoutKeepAlive();

private:
    ViridityWebServer server_;
    QSharedPointer<DebugRequestHandler> debugRequestHandler_;
    QString baseUrl_;
};

struct StressTestWebCall
{
    struct Result
    {
        Result() : requestsExecuted(0), successfulRequests(0), non2xxErrorsReceived(0), notFound404ErrorsReceived(0), errorsReceived(0) {}
        int requestsExecuted;
        int successfulRequests;
        int non2xxErrorsReceived;
        int notFound404ErrorsReceived;
        int errorsReceived;

        QString toString() const { return QString().sprintf("requestsExecuted: %d, successfulRequests: %d, non2xxErrorsReceived: %d, errorsReceived: %d", requestsExecuted, successfulRequests, non2xxErrorsReceived, errorsReceived); }
    };

    static Result run(const QUrl &url, int concurrencyLevel = 8, int requests = 20000, bool useKeepAlive = true, bool stopOnError = true)
    {
        int pendingConnectionCount = 0;
        bool error = false;
        Result result;

        QEventLoop e;

        QScopedPointer<QNetworkAccessManager> nam(new QNetworkAccessManager());

        for (int i = 0; i < requests; ++i)
        {
            while (pendingConnectionCount > concurrencyLevel)
                e.processEvents();

            if (stopOnError && error) break;

            WebCall *wc = new WebCall();

            QObject::connect(wc, &WebCall::success,
                [&pendingConnectionCount, &result] (QByteArray data)
                {
                    --pendingConnectionCount;
                    ++result.successfulRequests;
                }
            );

            QObject::connect(wc, &WebCall::error,
                [wc, &pendingConnectionCount, &error, &result] (int errorCode, const QString &errorText)
                {
                    --pendingConnectionCount;
                    ++result.errorsReceived;

                    if (wc->statusCode() < 200 || wc->statusCode() >= 300)
                        ++result.non2xxErrorsReceived;

                    if (wc->statusCode() == 404)
                        ++result.notFound404ErrorsReceived;

                    error = true;
                }
            );

            wc->setAutoDelete(true);
            wc->setNetworkAccessManager(nam.data());
            wc->setRequestHeader("Connection", useKeepAlive ? "Keep-Alive" : "Close");
            wc->get(url);
            ++result.requestsExecuted;
            ++pendingConnectionCount;
        }

        while (pendingConnectionCount > 0)
            e.processEvents();

        return result;
    }
};

ClientServerTest::ClientServerTest() :
    QObject(),
    server_(this, NULL),
    debugRequestHandler_(new DebugRequestHandler(&server_))
{
    server_.registerRequestHandler(debugRequestHandler_);

    const int port = 10101;
    baseUrl_ = QString("http://127.0.0.1:") + QString::number(port);

    if (!server_.listen(QHostAddress::LocalHost, port))
        qFatal("Could not setup in-process test web server on port %d.", port);
}

void ClientServerTest::initTestCase()
{

}

void ClientServerTest::cleanupTestCase()
{
    server_.close();
}

void ClientServerTest::stressWebCall404WithKeepAlive()
{
    StressTestWebCall::Result result = StressTestWebCall::run(QUrl(baseUrl_ + "/testcall_"), 8, 20000, true, false);
    QVERIFY2(result.notFound404ErrorsReceived == result.requestsExecuted, QString("Failure: %1").arg(result.toString()).toUtf8().constData());
}

void ClientServerTest::stressWebCall404WithoutKeepAlive()
{
    StressTestWebCall::Result result = StressTestWebCall::run(QUrl(baseUrl_ + "/testcall_"), 8, 20000, false, false);
    QVERIFY2(result.notFound404ErrorsReceived == result.requestsExecuted, QString("Failure: %1").arg(result.toString()).toUtf8().constData());
}

void ClientServerTest::stressWebCallOnDebugWithKeepAlive()
{
    StressTestWebCall::Result result = StressTestWebCall::run(QUrl(baseUrl_ + "/debug"), 8, 20000, true, true);
    QVERIFY2(result.successfulRequests == result.requestsExecuted, QString("Failure: %1").arg(result.toString()).toUtf8().constData());
}

void ClientServerTest::stressWebCallOnDebugWithoutKeepAlive()
{
    StressTestWebCall::Result result = StressTestWebCall::run(QUrl(baseUrl_ + "/debug"), 8, 20000, false, true);
    QVERIFY2(result.successfulRequests == result.requestsExecuted, QString("Failure: %1").arg(result.toString()).toUtf8().constData());
}

void ClientServerTest::benchmarkWebCall404WithKeepAlive()
{
    QBENCHMARK
    {
        StressTestWebCall::Result result = StressTestWebCall::run(QUrl(baseUrl_ + "/testcall_"), 8, 200000, true, false);
    }
}

void ClientServerTest::benchmarkWebCall404WithoutKeepAlive()
{
    QBENCHMARK
    {
        StressTestWebCall::Result result = StressTestWebCall::run(QUrl(baseUrl_ + "/testcall_"), 8, 200000, false, false);
    }
}

void ClientServerTest::benchmarkWebCallOnDebugWithKeepAlive()
{
    QBENCHMARK
    {
        StressTestWebCall::Result result = StressTestWebCall::run(QUrl(baseUrl_ + "/debug"), 8, 200000, true, false);
    }
}

void ClientServerTest::benchmarkWebCallOnDebugWithoutKeepAlive()
{
    QBENCHMARK
    {
        StressTestWebCall::Result result = StressTestWebCall::run(QUrl(baseUrl_ + "/debug"), 8, 200000, false, false);
    }
}

QTEST_MAIN(ClientServerTest)

#include "clientservertest.moc"
