/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: textarea

    contentMarkerElement: "textarea"
    contentMarkerAttributes: _ViridityHTMLTextArea_contentMarkerAttributes
    property variant _ViridityHTMLTextArea_contentMarkerAttributes: topLevelRenderer ? ({
        "placeholder": placeholder,
        "autocomplete": "off",
        "data-vdr-textarea-targetId": topLevelRenderer.targetId
    }) : ({})

    templateText: text

    property string text
    property string placeholder
    onPlaceholderChanged:
    {
        remote.call("setPlaceholder", { placeholder: placeholder });
    }

    function _sendContentUpdate()
    {
        remote.call("setText", { text: text });
        _sendVisibilityStatus();
    }

    ViridityHTMLScriptCallbacks {
        id: remote

        function changed(params)
        {
            textarea.beginUpdate();
            textarea.text = params.text;
            textarea.endUpdate();
        }
    }

    ViridityHTMLDependencies {
        dependencies: [
            "/ViridityTextArea.js"
        ]
        runAutoAttachment: true
    }
}
