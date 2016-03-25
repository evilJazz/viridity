import QtQuick 1.0
import KCL 1.0
import Viridity 1.0

import "ViridityDataBridge.js" as Private

QtObjectWithChildren {
    id: root

    /** type:ViriditySession The associated session instance */
    property alias session: bridge.session

    /** type:string Specifies the target id on the Viridity bus. The remote handler has to listen to this target id. */
    property alias targetId: bridge.targetId

    /**
     * Send data to a specific session or broadcast and wait for the callback to return with the answer.
     * @param type:variant data Data send from the target via the bridge. Can be any JS data type deserializable from JSON.
     * @param type:function callback Callback to be called with the answer from the remote handler. Can be null.
     * @param type:string sessionId ID of a specific session or undefined to broadcast to all sessions.
     */

    function sendData(data, callback, sessionId)
    {
        if (typeof(sessionId) == "undefined")
            sessionId = "";

        var responseId = bridge.sendData(JSON.stringify(data), sessionId);

        if (typeof(callback) == "function")
            Private.pendingResponseCallbacks[responseId] = callback;
    }

    /**
     * Called when data is received via the Viridity Data Bridge channel from a specific target in this session. Override this function to implement your own message handler.
     * @param type:variant input Data send from the target via the bridge. Can be any JS data type deserializable from JSON.
     * @return type:variant Return data to be sent via the bridge to the target receiver. Can be any JS data type serializable to JSON.
     */

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
