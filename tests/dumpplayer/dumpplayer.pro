QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dumpplayer
TEMPLATE = app

CONFIG += \
    viridity_module_display \
    viridity_module_display_tools \
    kcl_enable_ccache

include(../../viridity-static.pri)

SOURCES += main.cpp
