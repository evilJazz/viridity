/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLRepeater {
    id: select

    contentMarkerElement: "select"
    contentMarkerAttributes: _ViridityHTMLInput_contentMarkerAttributes
    property variant _ViridityHTMLInput_contentMarkerAttributes: topLevelRenderer ? ({
        "autocomplete": "off",
        "multiple": multiSelect ? true : undefined,
        "data-vdr-select-targetId": topLevelRenderer.targetId
    }) : ({})

    property bool escapeText: true

    property bool multiSelect: false
    property int selectedIndex: -1
    onSelectedIndexChanged:
    {
        remote.call("setSelected", { index: selectedIndex });
    }

    delegate:
        ViridityHTMLSegment {
            name: "selectOption"
            contentMarkerElement: "option"
            contentMarkerAttributes: _ViridityHTMLSelectOption_contentMarkerAttributes
            property variant _ViridityHTMLSelectOption_contentMarkerAttributes: {
                "value": currentIndex,
                "selected": currentIndex === selectedIndex ? true : undefined
            }

            templateText: currentModelData
        }

    property ViridityHTMLScriptCallbacks remote:
        ViridityHTMLScriptCallbacks {
            id: remote

            function changed(params)
            {
                select.beginUpdate();
                select.selectedIndex = params.index;
                select.endUpdate();
            }
        }

    property ViridityHTMLDependencies dependencies:
        ViridityHTMLDependencies {
            dependencies: [
                "/ViriditySelect.js"
            ]
            runAutoAttachment: true
        }
}
