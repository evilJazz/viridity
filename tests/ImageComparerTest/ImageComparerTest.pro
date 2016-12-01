QT       += core gui testlib

TARGET = ImageComparerTest

TEMPLATE = app

#QMAKE_CC=clang-3.5
#QMAKE_CXX=clang-3.5

#QMAKE_CFLAGS -= -O2
#QMAKE_CXXFLAGS -= -O2
#QMAKE_CFLAGS += -O3
#QMAKE_CXXFLAGS += -O3

SOURCES += main.cpp

CONFIG += viridity_module_display_areafingerprints

include(../../viridity-static.pri)

DEFINES += USE_MOVE_ANALYZER USE_MOVE_ANALYZER_FINEGRAINED

RESOURCES += \
    ../testdata/resources.qrc
