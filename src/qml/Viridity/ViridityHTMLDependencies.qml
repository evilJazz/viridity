/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "private/ViridityHTMLHelpers.js" as Internal;

QtObject {
    id: deps

    property bool runAutoAttachment: true
    property variant dependencies: []

    onDependenciesChanged: sendDependenciesUpdate()

    function sendDependenciesUpdate()
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

        topLevelRenderer.changeNotificatorDataBridge.sendData({
            action: "requireDependencies",
            dependencies: relativeDeps,
            runAutoAttachment: runAutoAttachment
        });
    }

    property Connections sessionConnections: Connections {
        target: renderer.inSessionContext ? currentSession : null
        onAttached: deps.sendDependenciesUpdate()
    }

    property ViridityHTMLDocumentConnection documentConnection: ViridityHTMLDocumentConnection {
        onAttached:
        {
            if (document.inSessionContext === renderer.inSessionContext)
                document.dependencyRegistry.register(deps);
        }

        onDetaching:
        {
            if (document.inSessionContext === renderer.inSessionContext)
                document.dependencyRegistry.unregister(deps);
        }
    }
}
