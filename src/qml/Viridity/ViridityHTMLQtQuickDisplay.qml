/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLColumn {
    id: renderer

    default property alias displayItem: display.displayItem

    property int width: 0
    property int height: 0

    property alias autoSize: display.autoSize
    property alias encoderSettings: display.encoderSettings
    property alias comparerSettings: display.comparerSettings

    property string _styleString: (width > 0 ? "width:" + width + "px;" : "") + (height > 0 ? "height:" + height + "px;": "")

    contentMarkerElement: "div"
    contentMarkerAttributes: _ViridityHTMLQtQuickDisplay_contentMarkerAttributes
    property variant _ViridityHTMLQtQuickDisplay_contentMarkerAttributes: ({
        "data-vdisp-targetId": renderer.name,
        "style": renderer._styleString
    })

    ViridityHTMLDependencies {
        dependencies: [
            "/DisplayRenderer.js"
        ]
        runAutoAttachment: true
    }

    property ViridityDisplay display: ViridityDisplay {
        id: display
        targetId: renderer.name
        autoSize: true
    }
}
