/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: renderer

    contentMarkerElement: "script"
    contentMarkerAttributes: _ViridityHTMLInlineJavaScript_contentMarkerAttributes
    property variant _ViridityHTMLInlineJavaScript_contentMarkerAttributes: ({ "type": "text/javascript" })

    templateText: script + _removeAfterExecutionString

    property string script
    property alias scriptSource: renderer.templateSource

    property bool removeAfterExecution: false
    property string _removeAfterExecutionString: (removeAfterExecution ? "\n{ DR.remove('${name}'); }" : "")

    function replaceMarkerForProperty(propertyName)
    {
        // Do not create HTML code for the properties...
        return _TemplateRenderer_replaceMarkerForProperty(propertyName);
    }
}
