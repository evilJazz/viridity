/* Work in progress */

import QtQuick 2.2
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {

    property variant _ViridityHTMLColumn_ignoredPropertyNames: _TemplateRenderer_ignoredPropertyNames.concat(["columnTemplate", "blockUpdates"])
    ignoredPropertyNames: _ViridityHTMLColumn_ignoredPropertyNames

    template: columnTemplate
    property string columnTemplate

    onChildrenChanged: if (!blockUpdates) _updateColumnTemplate(children)

    function _updateColumnTemplate(children)
    {
        if (blockUpdates) return;

        var newTemplate = "";

        for (var i = 0; i < children.length; ++i)
        {
            var child = children[i];
            if (isTemplateRenderer(child))
            {
                newTemplate += "${" + child.name + "}";
            }
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
