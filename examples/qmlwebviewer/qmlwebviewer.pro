QT       += core gui declarative

TARGET = qmlwebviewer

TEMPLATE = app

SOURCES += main.cpp

#CONFIG += tufao
include(../../3rdparty/tufao/tufao.pri)

DEFINES += USE_MULTITHREADED_WEBSERVER
include(../../viridity.pri)

OTHER_FILES += \
    test.qml

RESOURCES += \
    qmlresource.qrc

linux-* {
    target.path = /opt/bin
    INSTALLS += target
}
