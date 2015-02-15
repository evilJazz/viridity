QT       += core gui declarative

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dumpplayer
TEMPLATE = app

include(../../3rdparty/kcl/kcl.pri)
include(../../3rdparty/tufao/tufao-min.pri)
include(../../viridity.pri)

SOURCES += main.cpp
