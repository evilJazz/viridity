#include <QCoreApplication>

#include <Viridity/ViridityQmlExtendedAppCore>
#include <Viridity/FileRequestHandler>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ViridityQmlExtendedAppCore appCore;

    FileRequestHandler::publishFileGlobally("/", ":/index.html");
    FileRequestHandler::publishFileGlobally("/index.html", ":/index.html");

    appCore.initialize(
        QUrl(), // No global logic
        QUrl("qrc:/simple.qml"), // Our session logic
        QString() // No data location since we don't use it
    );

    // Read port from command line arguments...
    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    if (appCore.startWebServer(QHostAddress::LocalHost, dataPort))
        qDebug("Server is now listening on http://127.0.0.1:%d", dataPort);
    else
        qFatal("Could not setup server on http://127.0.0.1:%d", dataPort);

    return a.exec();
}
