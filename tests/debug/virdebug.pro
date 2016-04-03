QT       += core gui declarative

TARGET = virdebug

TEMPLATE = app

SOURCES += main.cpp

CONFIG(debug, debug|release) {
    message("Configuring debug mode...")
    DEFINES += DEBUG
    QMAKE_CFLAGS += -O0
    QMAKE_CXXFLAGS += -O0

    QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
    QMAKE_CFLAGS += -fsanitize=address -fno-omit-frame-pointer
    QMAKE_LFLAGS += -fsanitize=address
}

CONFIG(release, debug|release) {
    message("Configuring release mode...")
    DEFINES -= DEBUG
    QMAKE_CFLAGS -= -g
    QMAKE_CXXFLAGS -= -g
}

CONFIG += viridity_module_display_tools viridity_module_display_qtquick1
DEFINES += VIRIDITY_STATIC
include(../../viridity.pri)

DEFINES += TUFAO_STATIC
include(../../3rdparty/tufao/tufao-min.pri)

DEFINES += KCL_STATIC
include(../../3rdparty/kcl/kcl.pri)

RESOURCES += \
    ../testdata/resources.qrc

linux-* {
    target.path = /opt/bin
    INSTALLS += target
}
