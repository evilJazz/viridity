# General info

release {
    TARGET = viridity
} else {
    TARGET = viridityd
}

TEMPLATE = lib
VERSION = 0.0.1

CONFIG += QT
QT += network gui

# Build info

DEFINES += VIRIDITY_LIBRARY
#DESTDIR = lib

DESTDIR = ../../../
release {
    OBJECTS_DIR = build/release
    MOC_DIR = build/release
} else {
    OBJECTS_DIR = build/debug
    MOC_DIR = build/debug
}

# Install info

target.path = $$INSTALLDIR$$[QT_INSTALL_LIBS]

qmakefile.path = $$INSTALLDIR$$[QMAKE_MKSPECS]/features
qmakefile.files = pkg/viridity.prf

headers.path = $$INSTALLDIR$$[QT_INSTALL_HEADERS]/Viridity
headers.files = src/*.h \
    include/Viridity/*

INSTALLS = target qmakefile headers

include(viridity.pri)
