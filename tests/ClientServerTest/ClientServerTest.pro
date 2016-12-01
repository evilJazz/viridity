#-------------------------------------------------
#
# Project created by QtCreator 2016-11-28T14:00:34
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = clientservertest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

include(../../viridity-static.pri)

SOURCES += clientservertest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
