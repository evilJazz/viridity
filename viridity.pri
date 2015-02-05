QT += core gui network concurrent

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
    $$VIRIDITY_SRC_PATH/handlers/websockethandler.h \
    $$VIRIDITY_SRC_PATH/handlers/longpollinghandler.h \
    $$VIRIDITY_SRC_PATH/handlers/patchrequesthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/filerequesthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/inputposthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/ssehandler.h \
    $$PWD/src/display/declarativescenesizehandler.h \
    $$PWD/src/handlers/fileuploadhandler.h

SOURCES += \
    $$VIRIDITY_SRC_PATH/viriditywebserver.cpp \
    $$VIRIDITY_SRC_PATH/viriditysessionmanager.cpp \
    $$VIRIDITY_SRC_PATH/viriditydatabridge.cpp \
    $$VIRIDITY_SRC_PATH/handlers/websockethandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/longpollinghandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/filerequesthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/inputposthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/ssehandler.cpp \
    $$PWD/src/display/declarativescenesizehandler.cpp \
    $$PWD/src/handlers/fileuploadhandler.cpp

# Display related
CONFIG += viridity_module_display

viridity_module_display {
    INCLUDEPATH += \
        $$VIRIDITY_SRC_PATH/display

    HEADERS += \
        $$VIRIDITY_SRC_PATH/display/tiledregion.h \
        $$VIRIDITY_SRC_PATH/display/moveanalyzer.h \
        $$VIRIDITY_SRC_PATH/display/imagecomparer.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenewebcontrolcommandinterpreter.h \
        $$VIRIDITY_SRC_PATH/display/graphicssceneobserver.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenebufferrenderer.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplaysessionmanager.h \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplay.h \
        $$VIRIDITY_SRC_PATH/display/private/qtestspontaneevent.h \
        $$VIRIDITY_SRC_PATH/display/private/synchronizedscenechangedhandler.h \
        $$VIRIDITY_SRC_PATH/display/private/synchronizedscenerenderer.h \
        $$VIRIDITY_SRC_PATH/display/private/moveanalyzerdebugview.h \
        $$VIRIDITY_SRC_PATH/handlers/patchrequesthandler.h

    SOURCES += \
        $$VIRIDITY_SRC_PATH/display/tiledregion.cpp \
        $$VIRIDITY_SRC_PATH/display/moveanalyzer.cpp \
        $$VIRIDITY_SRC_PATH/display/imagecomparer.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenewebcontrolcommandinterpreter.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicssceneobserver.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenebufferrenderer.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplay.cpp \
        $$VIRIDITY_SRC_PATH/display/graphicsscenedisplaysessionmanager.cpp \
        $$VIRIDITY_SRC_PATH/display/private/synchronizedscenechangedhandler.cpp \
        $$VIRIDITY_SRC_PATH/display/private/synchronizedscenerenderer.cpp \
        $$VIRIDITY_SRC_PATH/display/private/moveanalyzerdebugview.cpp \
        $$VIRIDITY_SRC_PATH/handlers/patchrequesthandler.cpp
}

RESOURCES += \
    $$VIRIDITY_RES_PATH/viridityresources.qrc
