QT       += core

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

contains(QT_VERSION, ^4\\..*): CONFIG += viridity_module_display_qtquick1
contains(QT_VERSION, ^5\\..*): CONFIG += viridity_module_display_qtquick2

DEFINES += VIRIDITY_STATIC
include(../../viridity.pri)

DEFINES += TUFAO_STATIC
include(../../3rdparty/tufao/tufao-min.pri)

DEFINES += KCL_STATIC
include(../../3rdparty/kcl/kcl.pri)

linux-* {
    target.path = /opt/qmlwebviewer
    INSTALLS += target
}

RESOURCES += \
    resources.qrc
