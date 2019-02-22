#!/bin/bash
SCRIPT_FILENAME=$(readlink -f "`cd \`dirname \"$0\"\`; pwd`/`basename \"$0\"`")
SCRIPT_ROOT=$(dirname "$SCRIPT_FILENAME")

VIRIDITY_ROOT=$SCRIPT_ROOT

usage()
{
    cat << EOF
usage: $0 [options] <new project name> <new project directory>

OPTIONS:
   -h                Show this message.
EOF
}

writeQMLFiles()
{
    cat > "$QMLDIR/GlobalLogic.qml" << QML
import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

QtObjectWithChildren {
    id: global

    ViridityHTMLDocument {
        id: indexDoc
        targetId: "indexDoc"
        templateSource: "qrc:/web/index.template.html"

        title: "$PROJECTNAME"
        publishAtUrl: "/index.html"

        ViridityHTMLColumn {
            name: "bodyContent"

            ViridityHTMLSegment {
                contentMarkerElement: "h1"
                templateText: indexDoc.title
            }

            ViridityHTMLSegment {
                id: helloWorldSegment
                templateText: "Hello World! You clicked the button \${buttonClickCount} times."
                property int buttonClickCount: 0
            }

            ViridityHTMLSegment {
                name: "sessionSegment" // Specified in SessionLogic
            }
        }
    }
}
QML

    cat > "$QMLDIR/SessionLogic.qml" << QML
import QtQuick 2.0
import Viridity 1.0
import KCL 1.0

QtObjectWithChildren {
    id: session

    ViridityHTMLSessionSegment {
        targetHTMLDocument: indexDoc
        name: "sessionSegment"

        ViridityHTMLSegment {
            id: clickSegment
            templateText: "You clicked \${buttonClickCount} times in this session."
            property int buttonClickCount: 0
        }

        ViridityHTMLButton {
            title: "CLICK ME!"
            onClicked: 
            {
                ++helloWorldSegment.buttonClickCount;
                ++clickSegment.buttonClickCount;
            }
        }
    }
}
QML

    cat > "$QMLDIR/qmldir" << QMLDIR
GlobalLogic 1.0 GlobalLogic.qml
SessionLogic 1.0 SessionLogic.qml
QMLDIR
}

writeWebFiles()
{
    cat > "$WEBDIR/index.template.html" << HTML
<!DOCTYPE html>
<html>
  <head>
    <title>\${title}</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, height=device-height, initial-scale=1.0, user-scalable=no">
    \${insertDependencies all}
  </head>
  <body data-vdr-targetId="\${targetId}">
    \${bodyContent}
  </body>
</html>
HTML
}

writeMainCPPFile()
{
    cat > "$SRCDIR/main.cpp" << MAINCPP
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
        QUrl("qrc:/qml/GlobalLogic.qml"), // Our global logic
        QUrl("qrc:/qml/SessionLogic.qml"), // Our session logic
        QString() // No data location since we don't use it
    );

    appCore.rewriteRequestHandler()->addRule("^/$ /index.html [R]");

    // Read port from command line arguments...
    const int dataPort = a.arguments().count() > 1 ? a.arguments().at(1).toInt() : 8080;

    if (appCore.startWebServer(QHostAddress::LocalHost, dataPort))
        qDebug("Server is now listening on http://127.0.0.1:%d", dataPort);
    else
        qFatal("Could not setup server on http://127.0.0.1:%d", dataPort);

    return a.exec();
}
MAINCPP
}

writeResourcesFile()
{
    cat > "$QMLDIR/resourcesqml.qrc" << RESOURCESQRC
<RCC>
    <qresource prefix="/qml">
        <file>qmldir</file>
        <file>GlobalLogic.qml</file>
        <file>SessionLogic.qml</file>
    </qresource>
</RCC>
RESOURCESQRC

    cat > "$WEBDIR/resourcesweb.qrc" << RESOURCESQRC
<RCC>
    <qresource prefix="/web">
        <file>index.template.html</file>
    </qresource>
</RCC>
RESOURCESQRC
}

writeProjectFile()
{
    REL_VIRIDITY=$(realpath --relative-to="$PROJECTDIR" "$VIRIDITY_ROOT/viridity-static.pri")
    REL_RESOURCESWEB=$(realpath --relative-to="$PROJECTDIR" "$WEBDIR/resourcesweb.qrc")
    REL_RESOURCESQML=$(realpath --relative-to="$PROJECTDIR" "$QMLDIR/resourcesqml.qrc")
    
    cat > "$PROJECTDIR/$PROJECTNAME.pro" << PRO_FILE
QT       += core

TARGET = "$PROJECTNAME"

TEMPLATE = app

SOURCES += "src/main.cpp"

contains(QT_VERSION, ^4\\..*): CONFIG += viridity_declarative viridity_qtquick1
contains(QT_VERSION, ^5\\..*): CONFIG += viridity_declarative viridity_qtquick2
CONFIG += viridity_module_display
CONFIG  += kcl_enable_ccache

include($REL_VIRIDITY)

# Rewrite QML files to proper version...
viridity_qtquick1: qmlPreprocessFolder(\$\$PWD, @QtQuick1, 1.0)
viridity_qtquick2: qmlPreprocessFolder(\$\$PWD, @QtQuick2, 2.0)

RESOURCES += \\
    "$REL_RESOURCESWEB" \\
    "$REL_RESOURCESQML"

QTQUICK_COMPILER_SKIPPED_RESOURCES += \\
    "$REL_RESOURCESWEB"
PRO_FILE
}

scaffold()
{
    PROJECTNAME="$1"
    PROJECTDIR="$2"

    SRCDIR="$PROJECTDIR/src"
    QMLDIR="$SRCDIR/qml"
    WEBDIR="$SRCDIR/web"

    mkdir -p "$SRCDIR"
    mkdir -p "$QMLDIR"
    mkdir -p "$WEBDIR"

    writeProjectFile
    writeMainCPPFile
    writeQMLFiles
    writeWebFiles
    writeResourcesFile
}

while getopts ":h" OPTION; do
    case $OPTION in
        h)
            usage
            exit 1
            ;;
        ?)
            echo "Invalid option: -$OPTARG" >&2
            usage
            exit 1
            ;;
    esac
done

shift $((OPTIND-1))

if [ $# -eq 2 ]; then
    scaffold "$1" "$2"
else
	echo "Please specify a project name and destination directory."
	usage
	exit 1
fi

exit 0
