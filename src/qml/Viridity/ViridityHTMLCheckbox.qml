/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLInput {
    id: input

    type: "checkbox"

    contentMarkerAttributes: _ViridityHTMLCheckbox_contentMarkerAttributes
    property variant _ViridityHTMLCheckbox_contentMarkerAttributes: topLevelRenderer ? ({
        "checked": checked ? "checked" : undefined
    }) : ({})

    property bool checked: false

    BidirectionalBinding {
        leftTarget: input
        leftPropertyName: "checked"

        rightTarget: input
        rightPropertyName: "value"
    }
}
