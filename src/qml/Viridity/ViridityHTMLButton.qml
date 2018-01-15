/* Work in progress */

import QtQuick 2.2
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: button

    name: "button"
    contentMarkerElement: "button"
    contentMarkerAttributes: topLevelTemplateRenderer ? ({
        onclick: "DR.a('clicked', '" + name + "', '" + topLevelTemplateRenderer.targetId + "', this)"
    }) : ({})

    template: '${title}';

    property string title
    signal clicked()

    Connections {
        target: topLevelTemplateRenderer &&
                topLevelTemplateRenderer.hasOwnProperty("changeNotificatorDataBridge") ? topLevelTemplateRenderer : null

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
