/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: textarea

    contentMarkerElement: "textarea"
    contentMarkerAttributes: topLevelRenderer ? ({
        "placeholder": placeholder,
        "onChange": callbacks.connectAction("changed", callbacks.changed)
    }) : ({})

    templateText: '${text}'

    property string text
    property string placeholder

    ViridityHTMLScriptCallbacks {
        id: callbacks

        function changed(params)
        {

        }
    }
}
