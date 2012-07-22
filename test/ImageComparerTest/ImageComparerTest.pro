#-------------------------------------------------
#
# Project created by QtCreator 2012-07-21T00:03:15
#
#-------------------------------------------------

QT       += core

QT       += gui

TARGET = ImageComparerTest
CONFIG   += console
CONFIG   -= app_bundle

DEFINES += QT4

TEMPLATE = app

INCLUDEPATH += \
    ../.. \
    ../../../../core \
    ../../../../compat

SOURCES += main.cpp \
    ../../moveanalyzer.cpp \
    ../../imagecomparer.cpp \
    ../../../../core/debug.cpp

HEADERS += \
    ../../moveanalyzer.h \
    ../../imagecomparer.h \
    ../../../../core/debug.h
