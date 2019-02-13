/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {

    property variant _ViridityHTMLColumn_ignoredPropertyNames: _TemplateRenderer_ignoredPropertyNames.concat(["columnTemplate", "blockUpdates"])
    ignoredPropertyNames: _ViridityHTMLColumn_ignoredPropertyNames

    templateText: columnTemplate
    property string columnTemplate

    onSubRenderersChanged: if (!blockUpdates) _updateColumnTemplate(subRenderers)

    function _updateColumnTemplate(children)
    {
        if (blockUpdates) return;

        var newTemplate = "";

        for (var i = 0; i < subRenderers.length; ++i)
        {
            var child = subRenderers[i];
            newTemplate += "${" + child.name + "}";
        }

        columnTemplate = newTemplate;
    }

    property bool blockUpdates: false

    function beginUpdate()
    {
        blockUpdates = true;
    }

    function endUpdate()
    {
        blockUpdates = false;
        _updateColumnTemplate(children)
    }
}
