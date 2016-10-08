# Enable if you want to debug Viridity...
#DEFINES += VIRIDITY_DEBUG

QT += core network concurrent

DEFINES += USE_MULTITHREADING

VIRIDITY_SRC_PATH = $$PWD/src
VIRIDITY_RES_PATH = $$PWD/resources
VIRIDITY_INCLUDE_PATH = $$PWD/include

INCLUDEPATH += \
    $$VIRIDITY_INCLUDE_PATH \
    $$VIRIDITY_SRC_PATH

# Generic stuff
HEADERS += \
    $$VIRIDITY_SRC_PATH/viridity_global.h \
    $$VIRIDITY_SRC_PATH/viriditywebserver.h \
    $$VIRIDITY_SRC_PATH/viriditysessionmanager.h \
    $$VIRIDITY_SRC_PATH/viriditydatabridge.h \
    $$VIRIDITY_SRC_PATH/viridityrequesthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/websockethandler.h \
    $$VIRIDITY_SRC_PATH/handlers/longpollinghandler.h \
    $$VIRIDITY_SRC_PATH/handlers/filerequesthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/inputposthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/ssehandler.h \
    $$VIRIDITY_SRC_PATH/handlers/fileuploadhandler.h \
    $$VIRIDITY_SRC_PATH/handlers/sessionroutingrequesthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/debugrequesthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/rewriterequesthandler.h

SOURCES += \
    $$VIRIDITY_SRC_PATH/viriditywebserver.cpp \
    $$VIRIDITY_SRC_PATH/viriditysessionmanager.cpp \
    $$VIRIDITY_SRC_PATH/viriditydatabridge.cpp \
    $$VIRIDITY_SRC_PATH/viridityrequesthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/websockethandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/longpollinghandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/filerequesthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/inputposthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/ssehandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/fileuploadhandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/sessionroutingrequesthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/debugrequesthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/rewriterequesthandler.cpp

CONFIG += viridity_declarative

viridity_declarative {
    DEFINES += USE_QTQUICK2

    QT += qml

    HEADERS += $$VIRIDITY_SRC_PATH/viriditydeclarative.h
    SOURCES += $$VIRIDITY_SRC_PATH/viriditydeclarative.cpp
}

# Display related
CONFIG += viridity_module_display
!win32: CONFIG += viridity_use_improved_jpeg viridity_use_improved_png

viridity_module_display {
    QT += gui

    INCLUDEPATH += \
        $$VIRIDITY_SRC_PATH/display

    HEADERS += \
        $$VIRIDITY_SRC_PATH/display/comparer/imageaux.h \
        $$VIRIDITY_SRC_PATH/display/comparer/imagecomparer.h \
        $$VIRIDITY_SRC_PATH/display/comparer/imagecompareroptools.h \
        $$VIRIDITY_SRC_PATH/display/comparer/moveanalyzer.h \
        $$VIRIDITY_SRC_PATH/display/comparer/tiles.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplaycommandinterpreter.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenebufferrenderer.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplaymanager.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplay.h \
        $$VIRIDITY_SRC_PATH/display/handlers/patchrequesthandler.h  \
        $$VIRIDITY_SRC_PATH/display/graphicssceneadapter.h

    SOURCES += \
        $$VIRIDITY_SRC_PATH/display/comparer/imageaux.cpp \
        $$VIRIDITY_SRC_PATH/display/comparer/imagecomparer.cpp \
        $$VIRIDITY_SRC_PATH/display/comparer/imagecompareroptools.cpp \
        $$VIRIDITY_SRC_PATH/display/comparer/moveanalyzer.cpp \
        $$VIRIDITY_SRC_PATH/display/comparer/tiles.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplaycommandinterpreter.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenebufferrenderer.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplaymanager.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplay.cpp  \
        $$VIRIDITY_SRC_PATH/display/handlers/patchrequesthandler.cpp

    viridity_use_improved_jpeg {
        DEFINES += USE_IMPROVED_JPEG

        HEADERS += \
            $$VIRIDITY_SRC_PATH/display/private/jpegwriter.h

        SOURCES += \
            $$VIRIDITY_SRC_PATH/display/private/jpegwriter.cpp

        LIBS += -ljpeg
    }

    viridity_use_improved_png {
        DEFINES += USE_IMPROVED_PNG

        HEADERS += \
            $$VIRIDITY_SRC_PATH/display/private/pngwriter.h

        SOURCES += \
            $$VIRIDITY_SRC_PATH/display/private/pngwriter.cpp

        LIBS += -lpng
    }

    viridity_module_display_areafingerprints {
        DEFINES += USE_AREAFINGERPRINTS

        HEADERS += \
            $$VIRIDITY_SRC_PATH/display/comparer/areafingerprint.h

        SOURCES += \
            $$VIRIDITY_SRC_PATH/display/comparer/areafingerprint.cpp
    }

    viridity_module_display_tools {
        HEADERS += \
            $$VIRIDITY_SRC_PATH/display/private/graphicsscenedisplaytests.h \
            $$VIRIDITY_SRC_PATH/display/tools/graphicsscenedisplayrecorder.h \
            $$VIRIDITY_SRC_PATH/display/tools/graphicsscenedisplayplayer.h

        SOURCES += \
            $$VIRIDITY_SRC_PATH/display/private/graphicsscenedisplaytests.cpp \
            $$VIRIDITY_SRC_PATH/display/tools/graphicsscenedisplayrecorder.cpp \
            $$VIRIDITY_SRC_PATH/display/tools/graphicsscenedisplayplayer.cpp
    }

    include($${PWD}/3rdparty/kcl/qmlpp/src/qmlpp.pri)

    viridity_module_display_qtquick1 {
        message("Viridity: Configuring with QtQuick 1.x support")

        QT += declarative
        DEFINES += USE_QTQUICK1
        DEFINES -= USE_QTQUICK2
        CONFIG += viridity_module_display_qgraphicscene viridity_module_display_declarative

        HEADERS += \
            $$VIRIDITY_SRC_PATH/display/adapters/qtquick1adapter.h

        SOURCES += \
            $$VIRIDITY_SRC_PATH/display/adapters/qtquick1adapter.cpp
    }

    viridity_module_display_qtquick2 {
        message("Viridity: Configuring with QtQuick 2.x support")

        QT += qml quick
        DEFINES += USE_QTQUICK2
        DEFINES -= USE_QTQUICK1
        CONFIG += viridity_module_display_declarative

        HEADERS += \
            $$VIRIDITY_SRC_PATH/display/adapters/qtquick2adapter.h

        SOURCES += \
            $$VIRIDITY_SRC_PATH/display/adapters/qtquick2adapter.cpp
    }

    contains(DEFINES, USE_QTQUICK1): qmlPreprocessFolder($$VIRIDITY_SRC_PATH/qml, @QtQuick1, 1.0)
    contains(DEFINES, USE_QTQUICK2): qmlPreprocessFolder($$VIRIDITY_SRC_PATH/qml, @QtQuick2, 2.0)

    viridity_module_display_declarative {
        HEADERS += \
            $$VIRIDITY_SRC_PATH/display/declarativescenesizehandler.h

        SOURCES += \
            $$VIRIDITY_SRC_PATH/display/declarativescenesizehandler.cpp
    }

    viridity_module_display_qgraphicscene {
        message("Viridity: Configuring with QGraphicsScene support")

        DEFINES += USE_QGRAPHICSSCENE

        HEADERS += \
            $$VIRIDITY_SRC_PATH/display/private/synchronizedscenechangedhandler.h \
            $$VIRIDITY_SRC_PATH/display/private/synchronizedscenerenderer.h \
            $$VIRIDITY_SRC_PATH/display/adapters/qgraphicssceneadapter.h

        SOURCES += \
            $$VIRIDITY_SRC_PATH/display/private/synchronizedscenechangedhandler.cpp \
            $$VIRIDITY_SRC_PATH/display/private/synchronizedscenerenderer.cpp \
            $$VIRIDITY_SRC_PATH/display/adapters/qgraphicssceneadapter.cpp
    }
}

QML_IMPORT_PATH += $$VIRIDITY_SRC_PATH/qml/

RESOURCES += \
    $$VIRIDITY_RES_PATH/viridityresources.qrc
