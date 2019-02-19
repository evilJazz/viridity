/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "private/ViridityHTMLHelpers.js" as Internal;

ViridityHTMLInlineJavaScript {
    id: renderer

    script: _dependencyScript()

    removeAfterExecution: true
    function _dependencyScript()
    {
        var result = "";
        var relativeDeps = [];

        if (topLevelRenderer)
        {
            var document = Internal.getHTMLDocument(renderer);
            if (document)
            {
                for (var i = 0; i < dependencies.length; ++i)
                {
                    relativeDeps.push(document.makeRelative(dependencies[i]));
                }
            }
        }

        result += "DR.requires(" + JSON.stringify(relativeDeps) + ");";

        if (runAutoAttachment)
            result += 'if (ViridityAuto) { ViridityAuto.autoAttach(); }';

        return result;
    }

    ViridityHTMLDocumentConnection {

        onAttached:
        {
            if (document.inSessionContext === renderer.inSessionContext)
                document.dependencyRegistry.register(renderer);
        }

        onDetaching:
        {
            if (document.inSessionContext === renderer.inSessionContext)
                document.dependencyRegistry.unregister(renderer);
        }
    }

    property bool runAutoAttachment: true

    property variant dependencies: []
}
