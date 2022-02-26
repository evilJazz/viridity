/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: input

    contentMarkerElement: "input"
    contentMarkerAttributes: _ViridityHTMLInput_contentMarkerAttributes
    property variant _ViridityHTMLInput_contentMarkerAttributes: topLevelRenderer ? ({
        "placeholder": placeholder,
        "autocomplete": "off",
        "type": type,
        "value": value,
        "data-vdr-input-targetId": topLevelRenderer.targetId
    }) : ({})

    templateText: ""

    property string type: "text"
    property variant value: ""
    onValueChanged:
    {
        _sendContentUpdate();
    }

    Connections {
        target: typeof(currentSession) !== "undefined" ? currentSession : null
        onAttached: _sendContentUpdate();
    }

    property string placeholder
    onPlaceholderChanged:
    {
        remote.call("setPlaceholder", { placeholder: placeholder });
    }

    function _sendContentUpdate()
    {
        remote.call("setValue", { value: value });
        _sendVisibilityStatus();
    }

    ViridityHTMLScriptCallbacks {
        id: remote

        function changed(params)
        {
            input.beginUpdate();
            input.value = params.value;
            input.endUpdate();
        }
    }

    ViridityHTMLDependencies {
        dependencies: [
            "/ViridityInput.js"
        ]
        runAutoAttachment: true
    }
}
