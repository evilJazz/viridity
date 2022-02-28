/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "qrc:/KCL/StringHelpers.js" as StringHelpers;

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
    property bool escapeText: true
    property string placeholder
    onPlaceholderChanged:
    {
        remote.call("setPlaceholder", { placeholder: placeholder });
    }

    function _sendContentUpdate()
    {
        remote.call("setText", { text: escapeText ? text.unescapeHtml() : text });
        _sendVisibilityStatus();
    }

    ViridityHTMLRemote {
        id: remote

        dependencies: [ "/ViridityTextArea.js" ]
        runAutoAttachment: true

        function changed(params)
        {
            textarea.beginUpdate();
            textarea.text = escapeText ? params.text.escapeHtml() : params.text;
            textarea.endUpdate();
        }
    }
}
