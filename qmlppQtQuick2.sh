#!/bin/sh
3rdparty/kcl/qmlpp/src/qmlpp.sh -i -d "@QtQuick2" -q 2.2 src/qml/ tests/testdata/qml/ examples
echo "QML and JS files rewritten for QtQuick 2."
