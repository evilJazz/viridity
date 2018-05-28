/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: button

    name: "button"
    contentMarkerElement: "button"
    contentMarkerAttributes: topLevelRenderer ? ({
        onclick: "DR.a('clicked', '" + name + "', '" + topLevelRenderer.targetId + "', this)"
    }) : ({})

    templateText: '${title}';

    property string title
    signal clicked()

    Connections {
        target: topLevelRenderer &&
                topLevelRenderer.hasOwnProperty("changeNotificatorDataBridge") ? topLevelRenderer : null

        onDataReceived: // input
        {
            if (input.action === "clicked" &&
                input.itemName === button.name)
            {
                button.clicked();
            }
        }
    }
}
