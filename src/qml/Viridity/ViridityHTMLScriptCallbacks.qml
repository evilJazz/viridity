/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "private/ViridityHTMLScriptCallbacks.js" as Internal;

QtObject {
    id: scriptCallbacks
    property string itemName: name

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
}
