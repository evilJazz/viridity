/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "private/ViridityHTMLScriptCallbacks.js" as Internal;
import "private/ViridityHTMLHelpers.js" as Helpers;

QtObject {
    id: scriptCallbacks
    property string itemName: name

    property bool runAutoAttachment: true
    property variant dependencies: []

    // Callback management
    function connectAction(actionName, callback, params)
    {
        if (callback === undefined)
        {
            if (scriptCallbacks.hasOwnProperty(actionName))
                callback = scriptCallbacks[actionName];
            else
                console.log("Please define a function '" + actionName + "' in your script callbacks of item " + itemName + ".");
                return "";
        }

        Internal.add(actionName, callback);
        return "DR.a('" + topLevelRenderer.targetId + "','" + itemName + "','" + actionName + "'," + JSON.stringify(params) + ")";
    }

    function call(actionName, params, callback)
    {
        var data = {
            name: itemName,
            action: actionName,
            params: params
        };

        if (!topLevelRenderer || !topLevelRenderer.hasOwnProperty("changeNotificatorDataBridge"))
        {
            console.log(scriptCallbacks + ": No topLevelRenderer set, can't send update to client: " + JSON.stringify(data));
            return false;
        }

        topLevelRenderer.changeNotificatorDataBridge.sendData(data, callback);
    }

    property Connections dataReceiver: Connections {
        target: topLevelRenderer &&
                topLevelRenderer.hasOwnProperty("changeNotificatorDataBridge") ? topLevelRenderer : null

        onDataReceived: // input
        {
            if (input.name !== scriptCallbacks.itemName) return;

            var response = undefined;

            if (input.action in scriptCallbacks)
                response = scriptCallbacks[input.action](input.params);
            else if (Internal.contains(input.action))
                response = Internal.trigger(input.action, input.params);
            else
                console.log(scriptCallbacks + ": Please implement action " + input.action + " as function.");

            if (response !== undefined)
                topLevelRenderer.changeNotificatorDataBridge.response = response;
        }
    }

    // Dependency management
    onDependenciesChanged: sendDependenciesUpdate()

    function sendDependenciesUpdate()
    {
        if (dependencies.length == 0)
            return;

        var result = "";
        var relativeDeps = [];

        if (topLevelRenderer)
        {
            var document = Helpers.getHTMLDocument(renderer);
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
        onAttached: scriptCallbacks.sendDependenciesUpdate()
    }

    property ViridityHTMLDocumentConnection documentConnection: ViridityHTMLDocumentConnection {
        onAttached:
        {
            if (document.inSessionContext === renderer.inSessionContext)
                document.dependencyRegistry.register(scriptCallbacks);
        }

        onDetaching:
        {
            if (document.inSessionContext === renderer.inSessionContext)
                document.dependencyRegistry.unregister(scriptCallbacks);
        }
    }

}
