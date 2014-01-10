QT += core gui network

VIRIDITY_SRC_PATH = $$PWD/src
VIRIDITY_RES_PATH = $$PWD/resources
VIRIDITY_INCLUDE_PATH = $$PWD/include

DEFINES += USE_MULTITHREADED_WEBSERVER

INCLUDEPATH += \
    $$VIRIDITY_INCLUDE_PATH \
    $$VIRIDITY_SRC_PATH

HEADERS += \
    $$VIRIDITY_SRC_PATH/moveanalyzer.h \
    $$VIRIDITY_SRC_PATH/imagecomparer.h \
    $$VIRIDITY_SRC_PATH/graphicsscenewebcontrolcommandinterpreter.h \
    $$VIRIDITY_SRC_PATH/graphicsscenewebcontrol.h \
    $$VIRIDITY_SRC_PATH/graphicssceneobserver.h \
    $$VIRIDITY_SRC_PATH/graphicsscenebufferrenderer.h \
    $$VIRIDITY_SRC_PATH/viridity_global.h \
    $$VIRIDITY_SRC_PATH/private/qtestspontaneevent.h \
    $$VIRIDITY_SRC_PATH/private/synchronizedscenechangedhandler.h \
    $$VIRIDITY_SRC_PATH/private/synchronizedscenerenderer.h \
    $$VIRIDITY_SRC_PATH/private/moveanalyzerdebugview.h \
    $$VIRIDITY_SRC_PATH/private/commandbridge.h

SOURCES += \
    $$VIRIDITY_SRC_PATH/moveanalyzer.cpp \
    $$VIRIDITY_SRC_PATH/imagecomparer.cpp \
    $$VIRIDITY_SRC_PATH/graphicsscenewebcontrolcommandinterpreter.cpp \
    $$VIRIDITY_SRC_PATH/graphicsscenewebcontrol.cpp \
    $$VIRIDITY_SRC_PATH/graphicssceneobserver.cpp \
    $$VIRIDITY_SRC_PATH/graphicsscenebufferrenderer.cpp \
    $$VIRIDITY_SRC_PATH/private/synchronizedscenechangedhandler.cpp \
    $$VIRIDITY_SRC_PATH/private/synchronizedscenerenderer.cpp \
    $$VIRIDITY_SRC_PATH/private/moveanalyzerdebugview.cpp \
    $$VIRIDITY_SRC_PATH/private/commandbridge.cpp

OTHER_FILES += \
    $$VIRIDITY_SRC_PATH/displayRenderer.js \
    $$VIRIDITY_SRC_PATH/index.html \
    $$VIRIDITY_SRC_PATH/jquery.mousewheel.js

RESOURCES += \
    $$VIRIDITY_RES_PATH/webresources.qrc


!has_katastrophos_debug {
    SOURCES += \
        $$VIRIDITY_SRC_PATH/private/debug.cpp \

    HEADERS += \
        $$VIRIDITY_SRC_PATH/private/debug.h \
}
