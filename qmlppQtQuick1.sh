#!/bin/sh
3rdparty/kcl/qmlpp/src/qmlpp.sh -i -d "@QtQuick1" -q 1.1 tests/testdata/qml/ examples
echo "QML and JS files rewritten for QtQuick 1."
