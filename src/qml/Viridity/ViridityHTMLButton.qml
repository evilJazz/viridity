/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: button

    name: "button"
    contentMarkerElement: "button"
    contentMarkerAttributes: ({
        onclick: "documentRenderer.trigger(this, '" + name + "')"
    })

    template: '${title}';

    property string title
    signal clicked()

    Connections {
        target: topLevelTemplateRenderer &&
                topLevelTemplateRenderer.hasOwnProperty("changeNotificatorDataBridge") ? topLevelTemplateRenderer : null

        onDataReceived: // input
        {
            if (input.action === "clicked" &&
                input.itemName === renderer.name)
            {
                button.clicked();
            }
        }
    }
}
