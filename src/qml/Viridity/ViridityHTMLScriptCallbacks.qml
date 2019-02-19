/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "private/ViridityHTMLScriptCallbacks.js" as Internal;

QtObject {
    id: scriptCallbacks
    property string itemName: name

    function connectAction(actionName, callback)
    {
        Internal.add(actionName, callback);
        return "DR.a('" + actionName + "', '" + name + "', '" + topLevelRenderer.targetId + "', this)";
    }

    property Connections dataReceiver: Connections {
        target: topLevelRenderer &&
                topLevelRenderer.hasOwnProperty("changeNotificatorDataBridge") ? topLevelRenderer : null

        onDataReceived: // input
        {
            if (input.itemName !== scriptCallbacks.itemName) return;
            Internal.trigger(input.action, input.params);
        }
    }
}
