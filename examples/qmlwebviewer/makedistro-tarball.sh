#!/bin/bash
SCRIPT_FILENAME="`cd \`dirname \"$0\"\`; pwd`/`basename \"$0\"`"
DISTRO_ROOT=$(dirname "$SCRIPT_FILENAME")

function usage()
{
    echo "Usage: $0 [--bundle-libs] [TARGETROOT] [filename of output tar.gz]"
    echo
    echo "  --bundle-libs    Bundle Qt libraries with tarball"
    echo "  TARGETROOT, e.g. /opt/qmlwebviewer"
    echo
    echo "NOTE: Required the QTDIR environment variable to be defined."
    echo
}

BUNDLE_LIBS=0
if [ "$1" == "--bundle-libs" ]; then
    BUNDLE_LIBS=1
    shift 1
fi

TARGETROOT="$1"
OUTPUTNAME="$2"

[ -z $TARGETROOT ] && usage && exit 1
[ -z $OUTPUTNAME ] && usage && exit 1

[ -z $QTDIR ] && echo "Please define QTDIR environment variable" && exit 1
[ -z $CPUCOUNT ] && CPUCOUNT=$(nproc --all)

set -e

export PATH=$QTDIR/bin:$PATH

echo "Compiling package..."

cd "$DISTRO_ROOT"
rm -Rf build

mkdir build
cd build

qmake ../*.pro -r
make -j$CPUCOUNT

echo "Creating distribution..."

mkdir dist
make install INSTALL_ROOT="$PWD/dist"

OPT_ROOT="$PWD/dist/$TARGETROOT"

function copyLibDir()
{
    [ -d "$2" ] || mkdir -p "$2" || return 1
    rsync -rltuv --exclude=*plugind.dll --exclude=*d.dll --exclude=*.pdb $@
}

function copyQtFiles()
{
    cat > "$OPT_ROOT/bin/qt.conf" << EOF
[Paths]
Prefix = ..
EOF

    copyLibDir "$QTDIR/lib/libicu*" "$OPT_ROOT/lib/" || return
    copyLibDir "$QTDIR/lib/libQt5Core.so*" "$OPT_ROOT/lib/" || return
    copyLibDir "$QTDIR/lib/libQt5DBus.so*" "$OPT_ROOT/lib/" || return 11
    copyLibDir "$QTDIR/lib/libQt5Gui.so*" "$OPT_ROOT/lib/" || return 1
    copyLibDir "$QTDIR/lib/libQt5Widgets.so*" "$OPT_ROOT/lib/" || return 1
    copyLibDir "$QTDIR/lib/libQt5Qml.so*" "$OPT_ROOT/lib/" || return 1
    copyLibDir "$QTDIR/lib/libQt5Quick.so*" "$OPT_ROOT/lib/" || return 1
    copyLibDir "$QTDIR/lib/libQt5Network.so*" "$OPT_ROOT/lib/" || return 1
    copyLibDir "$QTDIR/lib/libQt5Concurrent.so*" "$OPT_ROOT/lib/" || return 1
    copyLibDir "$QTDIR/lib/libQt5Sql.so*" "$OPT_ROOT/lib/" || return 1
    copyLibDir "$QTDIR/lib/libQt5Svg.so*" "$OPT_ROOT/lib/" || return 1
    copyLibDir "$QTDIR/lib/libQt5Multimedia.so*" "$OPT_ROOT/lib/" || return 1
    copyLibDir "$QTDIR/lib/libQt5XcbQpa.so*" "$OPT_ROOT/lib/" || return 1

    copyLibDir "$QTDIR/qml/Qt" "$OPT_ROOT/qml/" || return 1
    copyLibDir "$QTDIR/qml/QtQml" "$OPT_ROOT/qml/" || return 1
    copyLibDir "$QTDIR/qml/QtQuick" "$OPT_ROOT/qml/" || return 1
    copyLibDir "$QTDIR/qml/QtQuick.2" "$OPT_ROOT/qml/" || return 1

    copyLibDir "$QTDIR/plugins/audio" "$OPT_ROOT/plugins/" || return 1
    copyLibDir "$QTDIR/plugins/generic" "$OPT_ROOT/plugins/" || return 1
    copyLibDir "$QTDIR/plugins/imageformats" "$OPT_ROOT/plugins/" || return 1
    copyLibDir "$QTDIR/plugins/sqldrivers" "$OPT_ROOT/plugins/" || return 1
    copyLibDir "$QTDIR/plugins/xcbglintegrations" "$OPT_ROOT/plugins/" || return 1

    [ -d "$OPT_ROOT/plugins/platforms" ] || mkdir "$OPT_ROOT/plugins/platforms/" || return 1
    cp -a "$QTDIR/plugins/platforms/libqxcb.so" "$OPT_ROOT/plugins/platforms/" || return 1

    [ -d "$OPT_ROOT/plugins/bearer" ] || mkdir "$OPT_ROOT/plugins/bearer/" || return 1
    cp -a "$QTDIR/plugins/bearer/libqgenericbearer.so" "$OPT_ROOT/plugins/bearer/" || return 1

    return 0
}

[ $BUNDLE_LIBS -eq 1 ] && copyQtFiles

cd dist
tar czvf "$OUTPUTNAME" *

cd "$DISTRO_ROOT"
rm -Rf build
