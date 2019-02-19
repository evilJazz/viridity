/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: button

    contentMarkerElement: "button"
    contentMarkerAttributes: topLevelRenderer ? ({
        "onClick": callbacks.connectAction("clicked", button.clicked)
    }) : ({})

    templateText: '${title}';

    property string title
    signal clicked()

    ViridityHTMLScriptCallbacks {
        id: callbacks
    }
}
