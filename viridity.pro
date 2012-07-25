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

# Project files

OTHER_FILES += \
    src/displayRenderer.js

HEADERS += \
    src/moveanalyzer.h \
    src/imagecomparer.h \
    src/graphicsscenewebcontrolcommandinterpreter.h \
    src/graphicsscenewebcontrol.h \
    src/graphicssceneobserver.h \
    src/graphicsscenebufferrenderer.h \
    src/viridity_global.h \
    src/private/debug.h \
    src/private/qtestspontaneevent.h \
    src/private/synchronizedscenechangedhandler.h \
    src/private/synchronizedscenerenderer.h

SOURCES += \
    src/moveanalyzer.cpp \
    src/imagecomparer.cpp \
    src/graphicsscenewebcontrolcommandinterpreter.cpp \
    src/graphicsscenewebcontrol.cpp \
    src/graphicssceneobserver.cpp \
    src/graphicsscenebufferrenderer.cpp \
    src/private/debug.cpp \
    src/private/synchronizedscenechangedhandler.cpp \
    src/private/synchronizedscenerenderer.cpp

RESOURCES += \
    resources/webresources.qrc


