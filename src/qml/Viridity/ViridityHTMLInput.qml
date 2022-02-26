/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "qrc:/KCL/StringHelpers.js" as StringHelpers;

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

    property alias text: input.value
    property bool escapeText: true

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
        var safeValue = typeof(value) == "string" && escapeText ? value.unescapeHtml() : value;
        remote.call("setValue", { value: safeValue });
        _sendVisibilityStatus();
    }

    ViridityHTMLScriptCallbacks {
        id: remote

        function changed(params)
        {
            input.beginUpdate();
            var safeValue = typeof(params.value) == "string" && escapeText ? params.value.escapeHtml() : params.value;
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
