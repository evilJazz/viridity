QT       += core qml

TARGET = simple

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

DEFINES += VIRIDITY_STATIC
include(../../viridity.pri)

DEFINES += TUFAO_STATIC
include(../../3rdparty/tufao/tufao-min.pri)

DEFINES += KCL_STATIC
include(../../3rdparty/kcl/kcl.pri)

linux-* {
    target.path = /opt/viriditysimple
    INSTALLS += target
}

RESOURCES += \
    resources.qrc
