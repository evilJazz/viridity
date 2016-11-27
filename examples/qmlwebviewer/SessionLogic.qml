import QtQuick 2.2
import Viridity 1.0

ViridityDisplay {
    targetId: "main"
    autoSize: true

    Loader {
        source: qmlSource
    }
}
