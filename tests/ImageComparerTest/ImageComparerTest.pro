QT       += core gui declarative testlib

TARGET = ImageComparerTest

CONFIG += TUFAO

TEMPLATE = app

#QMAKE_CFLAGS -= -O2
#QMAKE_CXXFLAGS -= -O2
#QMAKE_CFLAGS += -O3
#QMAKE_CXXFLAGS += -O3

SOURCES += main.cpp

include(../../3rdparty/kcl/kcl.pri)
include(../../3rdparty/tufao/tufao.pri)
include(../../viridity.pri)

RESOURCES += \
    ../testdata/test.qrc
