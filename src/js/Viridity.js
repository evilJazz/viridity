var ConnectionMethod = {
    Auto: -1,
    LongPolling: 0,
    ServerSentEvents: 1,
    WebSockets: 2
};

// Production steps of ECMA-262, Edition 5, 15.4.4.14
// Reference: http://es5.github.io/#x15.4.4.14
if (!Array.prototype.indexOf) {
  Array.prototype.indexOf = function(searchElement, fromIndex) {

    var k;

    // 1. Let O be the result of calling ToObject passing
    //    the this value as the argument.
    if (this == null) {
      throw new TypeError('"this" is null or not defined');
    }

    var O = Object(this);

    // 2. Let lenValue be the result of calling the Get
    //    internal method of O with the argument "length".
    // 3. Let len be ToUint32(lenValue).
    var len = O.length >>> 0;

    // 4. If len is 0, return -1.
    if (len === 0) {
      return -1;
    }

    // 5. If argument fromIndex was passed let n be
    //    ToInteger(fromIndex); else let n be 0.
    var n = +fromIndex || 0;

    if (Math.abs(n) === Infinity) {
      n = 0;
    }

    // 6. If n >= len, return -1.
    if (n >= len) {
      return -1;
    }

    // 7. If n >= 0, then Let k be n.
    // 8. Else, n<0, Let k be len - abs(n).
    //    If k is less than 0, then let k be 0.
    k = Math.max(n >= 0 ? n : len - Math.abs(n), 0);

    // 9. Repeat, while k < len
    while (k < len) {
      // a. Let Pk be ToString(k).
      //   This is implicit for LHS operands of the in operator
      // b. Let kPresent be the result of calling the
      //    HasProperty internal method of O with argument Pk.
      //   This step can be combined with c
      // c. If kPresent is true, then
      //    i.  Let elementK be the result of calling the Get
      //        internal method of O with the argument ToString(k).
      //   ii.  Let same be the result of applying the
      //        Strict Equality Comparison Algorithm to
      //        searchElement and elementK.
      //  iii.  If same is true, return k.
      if (k in O && O[k] === searchElement) {
        return k;
      }
      k++;
    }
    return -1;
  };
}

var Viridity = function(options)
{
    var debugVerbosity = 0;

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

        nextFreeTargetId: 0,
        targetCallbacks: {},

        callbacks: {
            sessionStart: [],
            sessionReattached: [],
            sessionInUse: []
        },

        on: function(eventName, callback)
        {
            if (eventName in v.callbacks)
                v.callbacks[eventName].push(callback);
        },

        off: function(eventName, callback)
        {
            if (eventName in v.callbacks)
            {
                var index = v.callbacks[eventName].indexOf(callback);
                if (index > -1)
                    v.callbacks[eventName].splice(index, 1);
            }
        },

        _triggerCallback: function(eventName, params)
        {
            var callbacks = v.callbacks[eventName];

            for (var i = 0; i < callbacks.length; ++i)
                callbacks[i](params);
        },

        _longPollingSendInputEvents: function()
        {
            if (v.inputEvents.length > 0)
            {
                var data = v.inputEvents.join("\n");
                v.inputEvents = [];

                if (debugVerbosity > 1)
                    console.log("Now posting messages to server:\n" + data);

                var options =
                {
                    type: "POST",
                    url: v.fullLocation + "/" + v.sessionId + "/v",
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
            var param = !v.inputEventsStarted ? "?a=init" : "";

            var options =
            {
                type: "GET",
                url: v.fullLocation + "/" + v.sessionId + "/v" + param,

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

        parseMessage: function(msg)
        {
            var paramStartIndex = msg.indexOf("(");

            var bm = msg.substring(0, paramStartIndex);
            var indexOfMarker = bm.indexOf(">");

            var targetId = undefined;

            if (indexOfMarker > -1)
            {
                targetId = msg.substring(0, indexOfMarker);
                msg = msg.substr(indexOfMarker + 1);
                paramStartIndex -= indexOfMarker + 1;
            }

            var paramEndIndex = msg.indexOf(")");

            var command = msg.substring(0, paramStartIndex);
            var params = msg.substring(paramStartIndex + 1, paramEndIndex);

            return {
                message: msg,
                targetId: targetId,
                paramStartIndex: paramStartIndex,
                paramEndIndex: paramEndIndex,
                command: command,
                params: params
            }
        },

        processMessage: function(msg)
        {
            var t = v.parseMessage(msg);
            var processed = false;

            if (debugVerbosity > 0)
                console.log("Received message from server: " + msg);

            if (typeof(t.targetId) !== "undefined" &&
                v.targetCallbacks.hasOwnProperty(t.targetId))
            {
                processed = v.targetCallbacks[t.targetId](t);
            }

            var inputParams = t.params.split(/[\s,]+/);

            if (t.command === "info")
            {
                v.sessionId = inputParams[0];
                v._triggerCallback("sessionStart", v.sessionId);
            }
            else if (t.command === "reattached")
            {
                v.sessionId = inputParams[0];
                v._triggerCallback("sessionReattached", v.sessionId);
            }
            else if (t.command === "inuse")
            {
                v.sessionId = inputParams[0];
                v._triggerCallback("sessionInUse", v.sessionId);
            }
        },

        sendMessage: function(msg, targetId)
        {
            if (typeof(targetId) !== "undefined" && targetId !== "")
                msg = targetId + ">" + msg;

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

        registerCallback: function(callback, targetId)
        {
            if (typeof(targetId) == "undefined")
            {
                targetId = v.nextFreeTargetId;
                ++v.nextFreeTargetId;
            }

            v.targetCallbacks[targetId] = callback;
            return targetId;
        },

        getCallback: function(targetId)
        {
            if (targetId in v.targetCallbacks)
                return v.targetCallbacks[targetId];
            else
                return false;
        },

        unregisterCallback: function(targetId)
        {
            if (targetId in v.targetCallbacks)
            {
                delete v.targetCallbacks[targetId];
                return true;
            }
            else
                return false;
        },

        reconnect: function()
        {
            location.reload(); // For now.
        },

        init: function(connectionMethod, sessionId, originUri)
        {
            v.connectionMethod = connectionMethod;

            if (v.connectionMethod === ConnectionMethod.Auto)
                v.connectionMethod = ConnectionMethod.WebSockets;

            if (v.connectionMethod === ConnectionMethod.WebSockets && !window.hasOwnProperty("WebSocket"))
                v.connectionMethod = ConnectionMethod.ServerSentEvents;

            if (v.connectionMethod === ConnectionMethod.ServerSentEvents && !window.hasOwnProperty("EventSource"))
                v.connectionMethod = ConnectionMethod.LongPolling;

            if (typeof(sessionId) != "undefined" && sessionId != "")
                v.sessionId = sessionId;

            var parser;
            if (typeof(originUri) !== "undefined")
            {
                parser = document.createElement('a');
                parser.href = originUri;
            }
            else
                parser = window.location;

            var pathnameNoFilename = String(parser.pathname);
            pathnameNoFilename = pathnameNoFilename.substring(0, pathnameNoFilename.lastIndexOf("/"));

            var hostWithPath = parser.host + pathnameNoFilename;
            v.location = hostWithPath.replace(/\/$/, "");
            v.fullLocation = parser.protocol + "//" + v.location;

            console.log("v.location: " + v.location);
            console.log("v.fullLocation: " + v.fullLocation);

            if (v.connectionMethod === ConnectionMethod.LongPolling)
            {
                v._longPollingReceiveOutputMessages();
            }
            else if (v.connectionMethod === ConnectionMethod.ServerSentEvents)
            {
                v.eventSource = new EventSource(v.fullLocation + "/" + v.sessionId + "/v/ev");
                v.eventSource.onmessage = v._serverSentEventsMessageReceived;
                v.eventSource.onerror = v.reconnect;
            }
            else if (v.connectionMethod === ConnectionMethod.WebSockets)
            {
                var ws = (v.fullLocation.indexOf("https:") > -1 ? "wss:" : "ws:");

                v.socket = new WebSocket(ws + "//" + v.location + "/" + v.sessionId + "/v/ws");

                v.socket.onmessage = function(msg) { v.processMessage(msg.data) };
                v.socket.onopen = v._sendQueuedMessages;
                v.socket.onerror = v.reconnect;
                v.socket.onclose = v.reconnect;
            }
        }
    }

    v.init(options.connectionMethod, options.sessionId, options.originUri);

    return v;
};
