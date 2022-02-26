/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: label

    property string text
    property ViridityHTMLSegment forElement
    property string forElementIdentifier: forElement ? forElement.identifier : undefined

    contentMarkerElement: "label"
    contentMarkerAttributes: _ViridityHTMLLabel_contentMarkerAttributes
    property variant _ViridityHTMLLabel_contentMarkerAttributes: topLevelRenderer ? ({
        "for": forElementIdentifier
    }) : ({})

    templateText: '${text}'
}
