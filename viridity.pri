# Enable if you want to debug Viridity...
#DEFINES += VIRIDITY_DEBUG

contains(QT_VERSION, ^4\\.[0-6]\\..*) {
    message("Viridity: Cannot build using Qt version $${QT_VERSION}.")
    error("Use at least Qt 4.7.")
}

QT += core network

contains(QT_VERSION, ^4\\..*) {
    CONFIG += viridity_qt4

    contains(QT, declarative): CONFIG += viridity_qtquick1 viridity_declarative
    contains(QT, gui): CONFIG += viridity_widgets

    message("Viridity: Configuring for Qt 4, actual version: $${QT_VERSION}")
}

contains(QT_VERSION, ^5\\..*) {
    CONFIG += viridity_qt5
    QT += concurrent

    contains(QT, quick): CONFIG += viridity_qtquick2 viridity_declarative

    contains(QT, declarative): CONFIG += viridity_qtquick1 viridity_declarative
    contains(QT, widgets): CONFIG += viridity_widgets

    message("Viridity: Configuring for Qt 5, actual version: $${QT_VERSION}")
}

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
    $$VIRIDITY_SRC_PATH/viridityconnection.h \
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
    $$VIRIDITY_SRC_PATH/viridityconnection.cpp \
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

viridity_declarative {
    QT += qml

    viridity_qtquick1 {
        message("Viridity: Configuring QtQuick 1.x declarative support classes")
        QT += declarative
        DEFINES += USE_QTQUICK1
        DEFINES -= USE_QTQUICK2
    }

    viridity_qtquick2 {
        message("Viridity: Configuring QtQuick 2.x declarative support classes")
        QT += qml quick
        DEFINES += USE_QTQUICK2
        DEFINES -= USE_QTQUICK1
    }

    HEADERS += \
        $$VIRIDITY_SRC_PATH/viriditydeclarative.h \
        $$VIRIDITY_SRC_PATH/viridityqmlsessionmanager.h \
        $$VIRIDITY_SRC_PATH/viridityqmlrequesthandler.h

    SOURCES += \
        $$VIRIDITY_SRC_PATH/viriditydeclarative.cpp \
        $$VIRIDITY_SRC_PATH/viridityqmlsessionmanager.cpp \
        $$VIRIDITY_SRC_PATH/viridityqmlrequesthandler.cpp
}

# Display related
!win32: CONFIG += viridity_use_improved_jpeg viridity_use_improved_png

viridity_module_display {
    DEFINES += VIRIDITY_MODULE_DISPLAY

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

    viridity_declarative {

        include($${PWD}/3rdparty/kcl/qmlpp/src/qmlpp.pri)

        viridity_qtquick1 {
            message("Viridity: Configuring display module with QtQuick 1.x support")

            CONFIG += viridity_module_display_qgraphicscene viridity_module_display_declarative

            HEADERS += \
                $$VIRIDITY_SRC_PATH/display/adapters/qtquick1adapter.h

            SOURCES += \
                $$VIRIDITY_SRC_PATH/display/adapters/qtquick1adapter.cpp
        }

        viridity_qtquick2 {
            message("Viridity: Configuring display module with QtQuick 2.x support")

            CONFIG += viridity_module_display_declarative

            HEADERS += \
                $$VIRIDITY_SRC_PATH/display/adapters/qtquick2adapter.h

            SOURCES += \
                $$VIRIDITY_SRC_PATH/display/adapters/qtquick2adapter.cpp
        }

        contains(DEFINES, USE_QTQUICK1): qmlPreprocessFolder($$VIRIDITY_SRC_PATH/qml, @QtQuick1, 1.0)
        contains(DEFINES, USE_QTQUICK2): qmlPreprocessFolder($$VIRIDITY_SRC_PATH/qml, @QtQuick2, 2.0)

        HEADERS += \
            $$VIRIDITY_SRC_PATH/display/declarativescenesizehandler.h \
            $$VIRIDITY_SRC_PATH/display/viridityqtquickdisplay.h

        SOURCES += \
            $$VIRIDITY_SRC_PATH/display/declarativescenesizehandler.cpp \
            $$VIRIDITY_SRC_PATH/display/viridityqtquickdisplay.cpp
    }

    viridity_module_display_qgraphicscene {
        message("Viridity: Configuring display module with QGraphicsScene support")

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
