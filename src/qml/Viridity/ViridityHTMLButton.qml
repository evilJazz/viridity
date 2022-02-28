/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: button

    property string title
    signal clicked()

    contentMarkerElement: "button"
    contentMarkerAttributes: _ViridityHTMLButton_contentMarkerAttributes
    property variant _ViridityHTMLButton_contentMarkerAttributes: topLevelRenderer ? ({
        "onClick": remote.connectAction("clicked", button.clicked)
    }) : ({})

    templateText: '${title}'

    ViridityHTMLRemote {
        id: remote
        runAutoAttachment: false
    }
}
