import QtQuick 1.0
import KCL 1.0
import Viridity 1.0

import "ViridityDataBridge.js" as Private

QtObjectWithChildren {
    id: root

    property alias session: bridge.session
    property alias targetId: bridge.targetId

    function sendData(data, callback, sessionId)
    {
        if (typeof(sessionId) == "undefined")
            sessionId = "";

        var responseId = bridge.sendData(JSON.stringify(data), sessionId);

        if (typeof(callback) == "function")
            Private.pendingResponseCallbacks[responseId] = callback;
    }

    function onDataReceived(input)
    {
        return "";
    }

    NativeViridityDataBridge {
        id: bridge

        onDataReceived: // responseId, input
        {
            bridge.response = JSON.stringify(root.onDataReceived(JSON.parse(input)));
        }

        onResponseReceived: // responseId, response, sessionId
        {
            if (Private.pendingResponseCallbacks.hasOwnProperty(responseId))
            {
                if (typeof(Private.pendingResponseCallbacks[responseId]) == "function")
                    Private.pendingResponseCallbacks[responseId](JSON.parse(response), sessionId);

                delete Private.pendingResponseCallbacks[responseId];
            }
        }
    }
}
