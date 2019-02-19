QT       += core

TARGET = reactive

TEMPLATE = app

SOURCES += main.cpp

contains(QT_VERSION, ^4\\..*): CONFIG += viridity_declarative viridity_qtquick1
contains(QT_VERSION, ^5\\..*): CONFIG += viridity_declarative viridity_qtquick2
CONFIG += viridity_module_display
CONFIG  += kcl_enable_ccache

include(../../viridity-static.pri)

# Rewrite QML files to proper version...
viridity_qtquick1: qmlPreprocessFolder($$PWD, @QtQuick1, 1.0)
viridity_qtquick2: qmlPreprocessFolder($$PWD, @QtQuick2, 2.0)

RESOURCES += \
    resources.qrc
