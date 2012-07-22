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

#QMAKE_CFLAGS -= -O2
#QMAKE_CXXFLAGS -= -O2
#QMAKE_CFLAGS += -O3
#QMAKE_CXXFLAGS += -O3

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
