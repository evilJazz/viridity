#include <QCoreApplication>

#include <Viridity/ViridityQmlExtendedAppCore>
#include <Viridity/FileRequestHandler>
#include <Viridity/RewriteRequestHandler>

#include "KCL/debug.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    forceEnableQtDebugOutput();
    setDiagnosticOutputEnabled(true);

    ViridityQmlExtendedAppCore appCore;

    appCore.initialize(
        QUrl("qrc:/ReactiveGlobalLogic.qml"), // Our global logic
        QUrl("qrc:/ReactiveSessionLogic.qml"), // Our session logic
        QString() // No data location since we don't use it
    );

    appCore.rewriteRequestHandler()->addRule("^/$ /test.html [R]");

    // Read port from command line arguments...
    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    if (appCore.startWebServer(QHostAddress::LocalHost, dataPort))
        qDebug("Server is now listening on http://127.0.0.1:%d", dataPort);
    else
        qFatal("Could not setup server on http://127.0.0.1:%d", dataPort);

    return a.exec();
}
