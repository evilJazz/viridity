QT       += testlib

QT       -= gui

TARGET = clientservertest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

CONFIG  += kcl_enable_ccache
include(../../viridity-static.pri)

SOURCES += clientservertest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

#DEFINES += QT_SHAREDPOINTER_TRACK_POINTERS
QMAKE_CFLAGS += -lasan -g -fsanitize=address -fno-omit-frame-pointer
QMAKE_CXXFLAGS += -lasan -g -fsanitize=address -fno-omit-frame-pointer
QMAKE_LFLAGS += -lasan -g -fsanitize=address -fno-omit-frame-pointer
