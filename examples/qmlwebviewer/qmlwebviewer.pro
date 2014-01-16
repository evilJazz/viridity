QT       += core gui declarative

TARGET = qmlwebviewer

TEMPLATE = app

SOURCES += main.cpp

#CONFIG += tufao

DEFINES += DEBUG

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
