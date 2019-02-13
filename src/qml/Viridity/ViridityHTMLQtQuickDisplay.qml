/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLColumn {
    id: renderer

    default property alias displayItem: display.displayItem

    property int width: 0
    property int height: 0

    property string _styleString: (width > 0 ? "width:" + width + "px;" : "") + (height > 0 ? "height:" + height + "px;": "")

    contentMarkerElement: "div"
    contentMarkerAttributes: ({
        "data-vdisp-targetId": renderer.name,
        "style": renderer._styleString
    })

    ViridityHTMLInlineJavaScript {
        script: 'if (ViridityAuto) { ViridityAuto.autoAttach(); }'
        removeAfterExecution: true
    }

    ViridityDisplay {
        id: display
        targetId: renderer.name
        autoSize: true
    }
}
