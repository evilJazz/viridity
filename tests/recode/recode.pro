QT       += core gui declarative

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = recode
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

include(../../3rdparty/kcl/kcl.pri)
include(../../3rdparty/tufao/tufao-min.pri)

CONFIG += viridity_module_display_tools
include(../../viridity.pri)

SOURCES += main.cpp
