# Enable if you want to debug Viridity...
#DEFINES += VIRIDITY_DEBUG

QT += core gui network concurrent

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
    $$VIRIDITY_SRC_PATH/viriditydeclarative.h \
    $$VIRIDITY_SRC_PATH/handlers/websockethandler.h \
    $$VIRIDITY_SRC_PATH/handlers/longpollinghandler.h \
    $$VIRIDITY_SRC_PATH/handlers/patchrequesthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/filerequesthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/inputposthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/ssehandler.h \
    $$VIRIDITY_SRC_PATH/handlers/fileuploadhandler.h \
    $$VIRIDITY_SRC_PATH/handlers/sessionroutingrequesthandler.h

SOURCES += \
    $$VIRIDITY_SRC_PATH/viriditywebserver.cpp \
    $$VIRIDITY_SRC_PATH/viriditysessionmanager.cpp \
    $$VIRIDITY_SRC_PATH/viriditydatabridge.cpp \
    $$VIRIDITY_SRC_PATH/viridityrequesthandler.cpp \
    $$VIRIDITY_SRC_PATH/viriditydeclarative.cpp \
    $$VIRIDITY_SRC_PATH/handlers/websockethandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/longpollinghandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/filerequesthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/inputposthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/ssehandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/fileuploadhandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/sessionroutingrequesthandler.cpp

# Display related
CONFIG += viridity_module_display viridity_use_improved_jpeg viridity_use_improved_png

viridity_module_display {
    INCLUDEPATH += \
        $$VIRIDITY_SRC_PATH/display

    HEADERS += \
        $$VIRIDITY_SRC_PATH/display/comparer/imageaux.h \
        $$VIRIDITY_SRC_PATH/display/comparer/imagecomparer.h \
        $$VIRIDITY_SRC_PATH/display/comparer/imagecompareroptools.h \
        $$VIRIDITY_SRC_PATH/display/comparer/moveanalyzer.h \
        $$VIRIDITY_SRC_PATH/display/comparer/tiles.h \
        $$VIRIDITY_SRC_PATH/display/declarativescenesizehandler.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenewebcontrolcommandinterpreter.h \
        $$VIRIDITY_SRC_PATH/display/graphicssceneobserver.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenebufferrenderer.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplaysessionmanager.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplay.h \
        $$VIRIDITY_SRC_PATH/display/private/synchronizedscenechangedhandler.h \
        $$VIRIDITY_SRC_PATH/display/private/synchronizedscenerenderer.h \
        $$VIRIDITY_SRC_PATH/handlers/patchrequesthandler.h

    SOURCES += \
        $$VIRIDITY_SRC_PATH/display/comparer/imageaux.cpp \
        $$VIRIDITY_SRC_PATH/display/comparer/imagecomparer.cpp \
        $$VIRIDITY_SRC_PATH/display/comparer/imagecompareroptools.cpp \
        $$VIRIDITY_SRC_PATH/display/comparer/moveanalyzer.cpp \
        $$VIRIDITY_SRC_PATH/display/comparer/tiles.cpp \
        $$VIRIDITY_SRC_PATH/display/declarativescenesizehandler.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenewebcontrolcommandinterpreter.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicssceneobserver.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenebufferrenderer.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplaysessionmanager.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplay.cpp  \
        $$VIRIDITY_SRC_PATH/display/private/synchronizedscenechangedhandler.cpp \
        $$VIRIDITY_SRC_PATH/display/private/synchronizedscenerenderer.cpp \
        $$VIRIDITY_SRC_PATH/handlers/patchrequesthandler.cpp

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
}

QML_IMPORT_PATH += $$VIRIDITY_SRC_PATH/qml/

RESOURCES += \
    $$VIRIDITY_RES_PATH/viridityresources.qrc
