QT       += core gui declarative

TARGET = qmlwebviewer

CONFIG += TUFAO

TEMPLATE = app

SOURCES += main.cpp

include(../../viridity.pri)

OTHER_FILES += \
    test.qml

RESOURCES += \
    qmlresource.qrc

linux-* {
    target.path = /opt/bin
    INSTALLS += target
}
