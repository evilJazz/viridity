QT       += core gui declarative

TARGET = qmlwebviewer

TEMPLATE = app

SOURCES += main.cpp

#CONFIG += tufao

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

include(../../3rdparty/kcl/kcl.pri)
include(../../3rdparty/tufao/tufao.pri)
include(../../viridity.pri)

OTHER_FILES += \
    test.qml

RESOURCES += \
    qmlresource.qrc

linux-* {
    target.path = /opt/bin
    INSTALLS += target
}
