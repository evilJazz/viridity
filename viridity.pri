QT += core gui network

VIRIDITY_SRC_PATH = $$PWD/src
VIRIDITY_RES_PATH = $$PWD/resources
VIRIDITY_INCLUDE_PATH = $$PWD/include

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
    $$VIRIDITY_SRC_PATH/private/commandbridge.h \
    $$VIRIDITY_SRC_PATH/handlers/graphicssceneinputposthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/commandposthandler.h \
    $$VIRIDITY_SRC_PATH/graphicsscenedisplay.h \
    $$VIRIDITY_SRC_PATH/handlers/websockethandler.h \
    $$VIRIDITY_SRC_PATH/handlers/longpollinghandler.h \
    $$VIRIDITY_SRC_PATH/handlers/patchrequesthandler.h \
    $$VIRIDITY_SRC_PATH/handlers/filerequesthandler.h

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
    $$VIRIDITY_SRC_PATH/private/commandbridge.cpp \
    $$VIRIDITY_SRC_PATH/handlers/graphicssceneinputposthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/commandposthandler.cpp \
    $$VIRIDITY_SRC_PATH/graphicsscenedisplay.cpp \
    $$VIRIDITY_SRC_PATH/handlers/websockethandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/longpollinghandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/patchrequesthandler.cpp \
    $$VIRIDITY_SRC_PATH/handlers/filerequesthandler.cpp

OTHER_FILES += \
    $$VIRIDITY_SRC_PATH/displayRenderer.js \
    $$VIRIDITY_SRC_PATH/index.html \
    $$VIRIDITY_SRC_PATH/jquery.mousewheel.js

RESOURCES += \
    $$VIRIDITY_RES_PATH/webresources.qrc
