QT += quick

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

contains(QT_VERSION, ^4\\..*): CONFIG += viridity_declarative viridity_qtquick1
contains(QT_VERSION, ^5\\..*): CONFIG += viridity_declarative viridity_qtquick2
CONFIG += viridity_module_display
CONFIG  += kcl_enable_ccache

include(../../viridity-static.pri)

# Rewrite QML files to proper version...
viridity_qtquick1: qmlPreprocessFolder($$PWD, @QtQuick1, 1.0)
viridity_qtquick2: qmlPreprocessFolder($$PWD, @QtQuick2, 2.0)

DEFINES+=DEBUG
