var ConnectionMethod = {
    Auto: -1,
    LongPolling: 0,
    ServerSentEvents: 1,
    WebSockets: 2
};

var Viridity = function(options)
{
    var debugVerbosity = 0;
    var originHref = window.location.href;

    var dr =
    {
        connectionMethod: ConnectionMethod.Auto,

        socket: 0,

        eventSource: 0,

        location: "",
        fullLocation: "",

        timeout: 120000,
        pause: 1000 / 30 /*fps*/, // Sets frequency for GET and POST when long polling
        sessionId: "",

        inputEvents: [],
        inputEventsStarted: false,

        onNewCommandReceived: undefined,

        responseId: 0,
        pendingCommandCallbacks: {},

        _longPollingSendInputEvents: function()
        {
            if (dr.inputEvents.length > 0)
            {
                var data = dr.inputEvents.join("\n");
                dr.inputEvents = [];
                var options =
                {
                    type: "POST",
                    url: dr.fullLocation + "/display?id=" + dr.sessionId,
                    async: true,
                    cache: false,
                    timeout: dr.timeout,
                    data: data,

                    success: function(data)
                    {
                        setTimeout(function() { dr._longPollingSendInputEvents() }, dr.pause);
                    },

                    error: function(xhr, status, exception)
                    {
                        console.log("While sending input events \"" + data + "\":\n" + status + " - " + exception + "\n");

                        if (status != "timeout")
                            dr.reconnect();
                        else
                            setTimeout(function() { dr._longPollingSendInputEvents() }, dr.pause);
                    }
                };

                $.ajax(options);
            }
            else
            {
                setTimeout(function() { dr._longPollingSendInputEvents() }, dr.pause);
            }
        },

        _longPollingCheckInputEventsStarted: function()
        {
            if (!dr.inputEventsStarted)
            {
                dr._longPollingSendInputEvents();
                dr.inputEventsStarted = true;
            }
        },

        _longPollingReceiveOutputMessages: function()
        {
            var options =
            {
                type: "GET",
                url: "display?id=" + dr.sessionId,

                async: true,
                cache: false,

                timeout: dr.timeout,

                success: function(data)
                {
//                    console.log("Got data: " + data);
                    var lines = data.split("\n");

                    for (var i = 0, ii = lines.length; i < ii; i++)
                    {
//                        console.log("line " + i + ": " + lines[i] + "\n");
                        dr.processPlainMessage(lines[i]);
                    }

                    setTimeout(function() { dr._longPollingReceiveOutputMessages() }, dr.pause);
                    dr._longPollingCheckInputEventsStarted();
                },
                error: function(xhr, status, exception)
                {
                    console.log("error receiving display data:\n" + status + " - " + exception + "\n");

                    if (status != "timeout")
                        dr.reconnect();
                    else
                        setTimeout(function() { dr._longPollingReceiveOutputMessages() }, dr.pause);
                }
            };

            $.ajax(options);
        },

        _serverSentEventsMessageReceived: function(e)
        {
            var lines = e.data.split("\n");

            for (var i = 0, ii = lines.length; i < ii; i++)
            {
                //console.log("line " + i + ": " + lines[i] + "\n");
                dr.processPlainMessage(lines[i]);
            }

            dr._longPollingCheckInputEventsStarted();
        },

        processPlainMessage: function(data)
        {
            var msg = {};
            msg["data"] = data;
            dr.processMessage(msg);
        },

        processMessage: function(msg)
        {
            var paramStartIndex = msg.data.indexOf("(");
            var paramEndIndex = msg.data.indexOf(")");

            var command = msg.data.substring(0, paramStartIndex);
            var params = msg.data.substring(paramStartIndex + 1, paramEndIndex);

            if (command === "command" || command === "commandResponse")
            {
                paramStartIndex = params.indexOf(",");

                var responseId = params.substring(0, paramStartIndex);
                var input = params.substring(paramStartIndex + 1);

                if (command === "commandResponse")
                {
                    if (dr.pendingCommandCallbacks.hasOwnProperty(responseId))
                    {
                        dr.pendingCommandCallbacks[responseId](JSON.parse(input));
                        delete dr.pendingCommandCallbacks[responseId];
                    }
                }
                else if (typeof(dr.onNewCommandReceived) == "function")
                {
                    var result = dr.onNewCommandReceived(JSON.parse(input));
                    dr.sendMessage("commandResponse(" + responseId + "," + JSON.stringify(result) + ")");
                }

                return;
            }

            var inputParams = params.split(/[\s,]+/);

            if (command === "info")
            {
                dr.sessionId = inputParams[0];
            }
        },

        sendMessage: function(msg)
        {
            if (debugVerbosity > 0)
                console.log("Sending message to server: " + msg);

            if (dr.connectionMethod === ConnectionMethod.WebSockets)
            {
                if (dr.socket.readyState === WebSocket.OPEN)
                    dr.socket.send(msg);
                else
                    dr.inputEvents.push(msg);
            }
            else if (dr.connectionMethod === ConnectionMethod.LongPolling ||
                     dr.connectionMethod === ConnectionMethod.ServerSentEvents)
                dr.inputEvents.push(msg);
        },

        _sendQueuedMessages: function()
        {
            if (dr.socket.readyState === WebSocket.OPEN)
            {
                for (var i = 0, ii = dr.inputEvents.length; i < ii; i++)
                    dr.sendMessage(dr.inputEvents[i]);

                dr.inputEvents = [];
            }
        },

        sendCommand: function(command, callback)
        {
            ++dr.responseId;
            dr.pendingCommandCallbacks[dr.responseId] = callback;
            var message = "command(" + dr.responseId + "," + JSON.stringify(command) + ")";
            dr.sendMessage(message);
        },

        reconnect: function()
        {
            location.reload(); // For now.
        },

        init: function(connectionMethod)
        {
            dr.connectionMethod = connectionMethod;

            if (dr.connectionMethod === ConnectionMethod.Auto)
                dr.connectionMethod = ConnectionMethod.WebSockets;

            if (dr.connectionMethod === ConnectionMethod.WebSockets && !window.hasOwnProperty("WebSocket"))
                dr.connectionMethod = ConnectionMethod.ServerSentEvents;

            if (dr.connectionMethod === ConnectionMethod.ServerSentEvents && !window.hasOwnProperty("EventSource"))
                dr.connectionMethod = ConnectionMethod.LongPolling;

            dr.fullLocation = window.location.href.replace(/\/$/, "");
            console.log("dr.fullLocation: " + dr.fullLocation);

            var hostWithPath = window.location.host + window.location.pathname;
            dr.location = hostWithPath.replace(/\/$/, "");

            if (dr.connectionMethod === ConnectionMethod.LongPolling)
            {
                dr._longPollingReceiveOutputMessages();
            }
            else if (dr.connectionMethod === ConnectionMethod.ServerSentEvents)
            {
                dr.eventSource = new EventSource("events");
                dr.eventSource.onmessage = dr._serverSentEventsMessageReceived;
                dr.eventSource.onerror = dr.reconnect;
            }
            else if (dr.connectionMethod === ConnectionMethod.WebSockets)
            {
                dr.socket = new WebSocket("ws://" + dr.location + "/display");

                dr.socket.onmessage = function(msg) { dr.processMessage(msg) };
                dr.socket.onopen = dr._sendQueuedMessages;
                dr.socket.onerror = dr.reconnect;
                dr.socket.onclose = dr.reconnect;
            }
        }
    }

    dr.init(options.connectionMethod);

    return dr;
};
