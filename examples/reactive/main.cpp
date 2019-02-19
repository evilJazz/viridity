#ifdef VIRIDITY_USE_QTQUICK1
#include <QApplication>
#else
#include <QGuiApplication>
#endif

#include <Viridity/ViridityQmlExtendedAppCore>
#include <Viridity/FileRequestHandler>
#include <Viridity/RewriteRequestHandler>

#include "KCL/debug.h"

int main(int argc, char *argv[])
{
#ifdef VIRIDITY_USE_QTQUICK1
    QApplication a(argc, argv);
#else
    QGuiApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
#endif

    forceEnableQtDebugOutput();
    setDiagnosticOutputEnabled(true);

    ViridityQmlExtendedAppCore appCore;

    appCore.initialize(
        QUrl("qrc:/ReactiveGlobalLogic.qml"), // Our global logic
        QUrl("qrc:/ReactiveSessionLogic.qml"), // Our session logic
        QString() // No data location since we don't use it
    );

    appCore.rewriteRequestHandler()->addRule("^/$ /index.html [R]");

    // Read port from command line arguments...
    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    if (appCore.startWebServer(QHostAddress::Any, dataPort))
        qDebug("Server is now listening on http://127.0.0.1:%d", dataPort);
    else
        qFatal("Could not setup server on http://127.0.0.1:%d", dataPort);

    return a.exec();
}
