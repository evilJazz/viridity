/****************************************************************************
**
** Copyright (C) 2012-2016 Andre Beckedorf, Meteora Softworks
** Contact: info@meteorasoftworks.com
**
** This file is part of Viridity
**
** $VIRIDITY_BEGIN_LICENSE:COMMERCIAL_AGPL$
**
** This library is licensed under either a separately available commercial
** license or the GNU Affero General Public License Version 3.0,
** published 19 November 2007.
**
** See https://www.gnu.org/licenses/agpl-3.0.html or LICENSE-agpl-3.0.txt for
** details.
**
** If you wish to use and distribute the Viridity library in your commercial
** product without making your sourcecode available to the public, please
** contact us for a commercial license at info@meteorasoftworks.com
**
** $VIRIDITY_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import Viridity 1.0

import "ViridityDataBridge.js" as Private

/*!
    The ViridityDataBridge declarative/QML component provides an easy way to send and receive data on the Viridity communication channel in the context of a ViriditySession and a target.
    It is a wrapper around the ViridityNativeDataBridge class adding support for JavaScript function callbacks in the ViridityDataBridge::sendData method.

    In your server-side QML code use something along the lines of:

    \code{.qml}
    ViridityDataBridge {
        session: currentSession // context property pointing to ViriditySession instance set when setting up the QML engine + logic in AbstractViriditySessionManager::initSession
        targetId: "test"

        function onDataReceived(input)
        {
            if (input.action == "hello")
                return { text: "Hello World!", success: true };
            else
                return { success: false };
        }

        Component.onCompleted:
        {
            sendData(
                "Server is here!",
                function(response)
                {
                    console.log("Input received from client: " + JSON.stringify(response));
                },
                session.id
            );
        }
    }
    \endcode

    Then on your client add something like this JavaScript code to your HTML page (requires jQuery + jQuery Cookie):

    \code{.js}
    var channel = new Viridity({ connectionMethod: ConnectionMethod.WebSockets, sessionId: $.cookie("vsessionId") });

    var initSession = function(sessionId)
    {
        $.cookie("vsessionId", sessionId);

        var dataBridge = new DataBridge(channel, "test");

        dataBridge.onDataReceived = function(input)
        {
            console.dir(input);
            alert("Input received from server: " + JSON.stringify(input));
            return "Client is here!";
        }

        // Attach to some HTML5 button with id "testButton"...
        $("#testButton").click(function()
        {
            dataBridge.sendData({ action: "hello" }, function(response)
            {
                console.dir(response);
                if (response.success)
                    alert("Response received from server: " + response.text + "\nas JSON: " + JSON.stringify(response));
                else
                    alert("Server indicated an error.");
            });
        });
    };

    channel.on("sessionStart", initSession);
    channel.on("sessionReattached", initSession);

    channel.on("sessionInUse", function(sessionId)
    {
        alert("This session is already in use in another tab or window.");
    });

    \endcode
*/

QtObject {
    id: root

    /** type:ViriditySession The associated session instance */
    property QtObject session: bridge.session

    /** type:string Specifies the target id on the Viridity communication channel. The remote handler has to listen to this target id. */
    property alias targetId: bridge.targetId

    /**
     * Send data to a specific session or broadcast to all sessions implementing the same logic and wait for the callback to return with the answer.
     * @param type:variant data Data send from the target via the bridge. Can be any JS data type deserializable from JSON.
     * @param type:function callback Callback of signature function(response, sessionId) to be called with the response from the remote handler's onDataReceived handler. Can be null.
     * @param type:string sessionId ID of a specific session or undefined to broadcast to all sessions implementing the same logic.
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
     * @return type:variant Return data to be sent via the bridge to the target receiver's callback. Can be any JS data type serializable to JSON.
     */

    function onDataReceived(input)
    {
        return "";
    }

    default property alias children: root.internalChildren
    property list<QtObject> internalChildren: [
        ViridityNativeDataBridge {
            id: bridge

            session: root.session == currentSession ? currentSession.nativeSession : root.session

            onDataReceived: // responseId, input
            {
                var response = root.onDataReceived(JSON.parse(input));

                if (typeof(response) == "undefined")
                    bridge.response = "null";
                else
                    bridge.response = JSON.stringify(response);
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
    ]
}
