QT       += core sql

TARGET = qmlwebviewer

TEMPLATE = app

SOURCES += main.cpp

CONFIG(debug, debug|release) {
    message("Configuring debug mode...")
    DEFINES += DEBUG
    QMAKE_CFLAGS += -O0
    QMAKE_CXXFLAGS += -O0
}

CONFIG(release, debug|release) {
    message("Configuring release mode...")
    DEFINES -= DEBUG
    QMAKE_CFLAGS -= -g
    QMAKE_CXXFLAGS -= -g
}

contains(QT_VERSION, ^4\\..*): CONFIG += viridity_declarative viridity_qtquick1
contains(QT_VERSION, ^5\\..*): CONFIG += viridity_declarative viridity_qtquick2
CONFIG += viridity_module_display
CONFIG  += kcl_enable_ccache
include(../../viridity-static.pri)

linux-* {
    target.path = /opt/qmlwebviewer/bin
    QMAKE_LFLAGS += -Wl,--rpath=../lib/
    INSTALLS += target
}

RESOURCES += \
    resources.qrc

#DEFINES += QT_SHAREDPOINTER_TRACK_POINTERS
#QMAKE_CFLAGS += -lasan -g -fsanitize=address -fno-omit-frame-pointer
#QMAKE_CXXFLAGS += -lasan -g -fsanitize=address -fno-omit-frame-pointer
#QMAKE_LFLAGS += -lasan -g -fsanitize=address -fno-omit-frame-pointer

DEFINES += VIRIDITY_DEBUG_REQUESTHANDLER
#DEFINES += VIRIDITY_DEBUG

