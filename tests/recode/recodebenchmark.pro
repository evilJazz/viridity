QT       += core gui declarative testlib

TARGET = recodebenchmark

CONFIG += TUFAO

TEMPLATE = app

#QMAKE_CC=ccache clang-3.5
#QMAKE_CXX=ccache clang-3.5

QMAKE_CC=ccache gcc
QMAKE_CXX=ccache gcc

#QMAKE_CFLAGS -= -O2
#QMAKE_CXXFLAGS -= -O2
#QMAKE_CFLAGS += -O3
#QMAKE_CXXFLAGS += -O3

QMAKE_CFLAGS += -g
QMAKE_CXXFLAGS += -g

SOURCES += mainbenchmark.cpp

include(../../3rdparty/kcl/kcl.pri)
include(../../3rdparty/tufao/tufao-min.pri)
include(../../viridity.pri)

#DEFINES += USE_MOVE_ANALYZER USE_MOVE_ANALYZER_FINEGRAINED
