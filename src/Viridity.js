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

    var v =
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
            if (v.inputEvents.length > 0)
            {
                var data = v.inputEvents.join("\n");
                v.inputEvents = [];
                var options =
                {
                    type: "POST",
                    url: v.fullLocation + "/display?id=" + v.sessionId,
                    async: true,
                    cache: false,
                    timeout: v.timeout,
                    data: data,

                    success: function(data)
                    {
                        setTimeout(function() { v._longPollingSendInputEvents() }, v.pause);
                    },

                    error: function(xhr, status, exception)
                    {
                        console.log("While sending input events \"" + data + "\":\n" + status + " - " + exception + "\n");

                        if (status != "timeout")
                            v.reconnect();
                        else
                            setTimeout(function() { v._longPollingSendInputEvents() }, v.pause);
                    }
                };

                $.ajax(options);
            }
            else
            {
                setTimeout(function() { v._longPollingSendInputEvents() }, v.pause);
            }
        },

        _longPollingCheckInputEventsStarted: function()
        {
            if (!v.inputEventsStarted)
            {
                v._longPollingSendInputEvents();
                v.inputEventsStarted = true;
            }
        },

        _longPollingReceiveOutputMessages: function()
        {
            var options =
            {
                type: "GET",
                url: "display?id=" + v.sessionId,

                async: true,
                cache: false,

                timeout: v.timeout,

                success: function(data)
                {
//                    console.log("Got data: " + data);
                    var lines = data.split("\n");

                    for (var i = 0, ii = lines.length; i < ii; i++)
                    {
//                        console.log("line " + i + ": " + lines[i] + "\n");
                        v.processMessage(lines[i]);
                    }

                    setTimeout(function() { v._longPollingReceiveOutputMessages() }, v.pause);
                    v._longPollingCheckInputEventsStarted();
                },
                error: function(xhr, status, exception)
                {
                    console.log("error receiving display data:\n" + status + " - " + exception + "\n");

                    if (status != "timeout")
                        v.reconnect();
                    else
                        setTimeout(function() { v._longPollingReceiveOutputMessages() }, v.pause);
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
                v.processMessage(lines[i]);
            }

            v._longPollingCheckInputEventsStarted();
        },

        processMessage: function(msg)
        {
            var paramStartIndex = msg.indexOf("(");
            var paramEndIndex = msg.indexOf(")");

            var command = msg.substring(0, paramStartIndex);
            var params = msg.substring(paramStartIndex + 1, paramEndIndex);

            if (command === "command" || command === "commandResponse")
            {
                paramStartIndex = params.indexOf(",");

                var responseId = params.substring(0, paramStartIndex);
                var input = params.substring(paramStartIndex + 1);

                if (command === "commandResponse")
                {
                    if (v.pendingCommandCallbacks.hasOwnProperty(responseId))
                    {
                        v.pendingCommandCallbacks[responseId](JSON.parse(input));
                        delete v.pendingCommandCallbacks[responseId];
                    }
                }
                else if (typeof(v.onNewCommandReceived) == "function")
                {
                    var result = v.onNewCommandReceived(JSON.parse(input));
                    v.sendMessage("commandResponse(" + responseId + "," + JSON.stringify(result) + ")");
                }

                return;
            }

            var inputParams = params.split(/[\s,]+/);

            if (command === "info")
            {
                v.sessionId = inputParams[0];
            }
        },

        sendMessage: function(msg)
        {
            if (debugVerbosity > 0)
                console.log("Sending message to server: " + msg);

            if (v.connectionMethod === ConnectionMethod.WebSockets)
            {
                if (v.socket.readyState === WebSocket.OPEN)
                    v.socket.send(msg);
                else
                    v.inputEvents.push(msg);
            }
            else if (v.connectionMethod === ConnectionMethod.LongPolling ||
                     v.connectionMethod === ConnectionMethod.ServerSentEvents)
                v.inputEvents.push(msg);
        },

        _sendQueuedMessages: function()
        {
            if (v.socket.readyState === WebSocket.OPEN)
            {
                for (var i = 0, ii = v.inputEvents.length; i < ii; i++)
                    v.sendMessage(v.inputEvents[i]);

                v.inputEvents = [];
            }
        },

        sendCommand: function(command, callback)
        {
            ++v.responseId;
            v.pendingCommandCallbacks[v.responseId] = callback;
            var message = "command(" + v.responseId + "," + JSON.stringify(command) + ")";
            v.sendMessage(message);
        },

        reconnect: function()
        {
            location.reload(); // For now.
        },

        init: function(connectionMethod)
        {
            v.connectionMethod = connectionMethod;

            if (v.connectionMethod === ConnectionMethod.Auto)
                v.connectionMethod = ConnectionMethod.WebSockets;

            if (v.connectionMethod === ConnectionMethod.WebSockets && !window.hasOwnProperty("WebSocket"))
                v.connectionMethod = ConnectionMethod.ServerSentEvents;

            if (v.connectionMethod === ConnectionMethod.ServerSentEvents && !window.hasOwnProperty("EventSource"))
                v.connectionMethod = ConnectionMethod.LongPolling;

            v.fullLocation = window.location.href.replace(/\/$/, "");
            console.log("v.fullLocation: " + v.fullLocation);

            var hostWithPath = window.location.host + window.location.pathname;
            v.location = hostWithPath.replace(/\/$/, "");

            if (v.connectionMethod === ConnectionMethod.LongPolling)
            {
                v._longPollingReceiveOutputMessages();
            }
            else if (v.connectionMethod === ConnectionMethod.ServerSentEvents)
            {
                v.eventSource = new EventSource("events");
                v.eventSource.onmessage = v._serverSentEventsMessageReceived;
                v.eventSource.onerror = v.reconnect;
            }
            else if (v.connectionMethod === ConnectionMethod.WebSockets)
            {
                v.socket = new WebSocket("ws://" + v.location + "/display");

                v.socket.onmessage = function(msg) { v.processMessage(msg.data) };
                v.socket.onopen = v._sendQueuedMessages;
                v.socket.onerror = v.reconnect;
                v.socket.onclose = v.reconnect;
            }
        }
    }

    v.init(options.connectionMethod);

    return v;
};
