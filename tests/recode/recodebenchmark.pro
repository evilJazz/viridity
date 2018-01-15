QT       += core gui widgets testlib
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = recodebenchmark
TEMPLATE = app

CONFIG += \
    viridity_module_display \
    viridity_module_display_tools

CONFIG  += kcl_enable_ccache
include(../../viridity-static.pri)

SOURCES += mainbenchmark.cpp
