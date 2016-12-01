#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QObject>

#include "KCL/webcall.h"

class ClientServerTest : public QObject
{
    Q_OBJECT

public:
    ClientServerTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void stressWebCallOnWrongUrlWithKeepAlive();
    void stressWebCallOnWrongUrlWithoutKeepAlive();

    void stressWebCallOnDebugWithKeepAlive();
    void stressWebCallOnDebugWithoutKeepAlive();
};

struct StressTestWebCall
{
    struct Result
    {
        Result() : requestsExecuted(0), successfulRequests(0), non2xxErrorsReceived(0), errorsReceived(0) {}
        int requestsExecuted;
        int successfulRequests;
        int non2xxErrorsReceived;
        int errorsReceived;

        QString toString() const { return QString().sprintf("requestsExecuted: %d, successfulRequests: %d, non2xxErrorsReceived: %d, errorsReceived: %d", requestsExecuted, successfulRequests, non2xxErrorsReceived, errorsReceived); }
    };

    static Result run(const QUrl &url, int concurrencyLevel = 8, int requests = 20000, bool useKeepAlive = true, bool stopOnError = true)
    {
        int pendingConnectionCount = 0;
        bool error = false;
        Result result;

        QEventLoop e;

        QNetworkAccessManager *nam = NULL;

        if (useKeepAlive)
            nam = new QNetworkAccessManager();

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

                    error = true;
                }
            );

            wc->setAutoDelete(true);

            if (nam)
                wc->setNetworkAccessManager(nam);

            wc->get(url);
            ++result.requestsExecuted;
            ++pendingConnectionCount;
        }

        while (pendingConnectionCount > 0)
            e.processEvents();

        return result;
    }
};

ClientServerTest::ClientServerTest()
{
}

void ClientServerTest::initTestCase()
{
}

void ClientServerTest::cleanupTestCase()
{

}

void ClientServerTest::stressWebCallOnWrongUrlWithKeepAlive()
{
    StressTestWebCall::Result result = StressTestWebCall::run(QUrl("http://127.0.0.1:8080/testcall_"), 8, 20000, true, false);
    QVERIFY2(result.non2xxErrorsReceived == result.requestsExecuted, QString("Failure: %1").arg(result.toString()).toUtf8().constData());
}

void ClientServerTest::stressWebCallOnWrongUrlWithoutKeepAlive()
{
    StressTestWebCall::Result result = StressTestWebCall::run(QUrl("http://127.0.0.1:8080/testcall_"), 8, 20000, false, false);
    QVERIFY2(result.non2xxErrorsReceived == result.requestsExecuted, QString("Failure: %1").arg(result.toString()).toUtf8().constData());
}

void ClientServerTest::stressWebCallOnDebugWithKeepAlive()
{
    StressTestWebCall::Result result = StressTestWebCall::run(QUrl("http://127.0.0.1:8080/debug"), 8, 20000, true, true);
    QVERIFY2(result.successfulRequests == result.requestsExecuted, QString("Failure: %1").arg(result.toString()).toUtf8().constData());
}

void ClientServerTest::stressWebCallOnDebugWithoutKeepAlive()
{
    StressTestWebCall::Result result = StressTestWebCall::run(QUrl("http://127.0.0.1:8080/debug"), 8, 20000, false, true);
    QVERIFY2(result.successfulRequests == result.requestsExecuted, QString("Failure: %1").arg(result.toString()).toUtf8().constData());
}

QTEST_MAIN(ClientServerTest)

#include "clientservertest.moc"
