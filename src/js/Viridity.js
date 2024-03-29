/****************************************************************************
**
** Copyright (C) 2012-2016 Andre Beckedorf, Meteora Softworks
** Contact: info@meteorasoftworks.com
**
** This file is part of Viridity
**
** $VIRIDITY_BEGIN_LICENSE:MIT$
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
** $VIRIDITY_END_LICENSE$
**
****************************************************************************/

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

if (!Uint8Array.prototype.convertToString)
{
    Uint8Array.prototype.convertToString = function()
    {
        try
        {
            var encodedString = String.fromCharCode.apply(null, this);
            var decodedString = decodeURIComponent(escape(encodedString));
            return decodedString;
        }
        catch (e)
        {
            return "";
        }
    }
}

if (!Uint8Array.prototype.slice)
{
    Uint8Array.prototype.slice = function(startIndex, endIndex)
    {
        // FF is picky about the second parameter, don't simplify and pass undefined!
        // subarray will return zero length view for undefined endIndex...
        if (typeof(endIndex) == "undefined")
            return this.subarray(startIndex);
        else
            return this.subarray(startIndex, endIndex);

        //var startOffset = this.byteOffset + startIndex;
        //var length = typeof(endIndex) == "undefined" ? undefined : endIndex - startIndex;
        //return new Uint8Array(this.buffer, startOffset, length);
    }
}

if (!Uint8Array.prototype.indexOfChar)
{
    Uint8Array.prototype.indexOfChar = function(searchElement, fromIndex)
    {
        if (searchElement.length === 0)
            return -1;

        if (typeof(searchElement) == "string")
            searchElement = searchElement.charCodeAt(0);

        return Array.prototype.indexOf.call(this, searchElement, fromIndex);
    }
}

if (!String.prototype.indexOfChar)
{
    String.prototype.indexOfChar = function(searchElement, fromIndex)
    {
        return this.indexOf(searchElement, fromIndex);
    }
}

String.prototype.removeLeadingSlash = function()
{
    return this.replace(/^\//, "");
}

String.prototype.removeTrailingSlash = function()
{
    return this.replace(/\/$/, "");
}

var ViridityHelper = {
    parseOriginUri: function(originUri)
    {
        var parser;
        if (typeof(originUri) !== "undefined")
        {
            parser = document.createElement('a');
            parser.href = originUri;
        }
        else
            parser = window.location;

        var pathnameNoFilename = String(parser.pathname);
        pathnameNoFilename = pathnameNoFilename.substring(0, pathnameNoFilename.lastIndexOf("/")).removeTrailingSlash();

        var hostWithPath = parser.host.removeTrailingSlash();
        if (pathnameNoFilename.length > 0)
            hostWithPath += "/" + pathnameNoFilename.removeLeadingSlash();

        return {
            location: hostWithPath,
            fullLocation: parser.protocol + "//" + hostWithPath
        }
    },

    getCurrentScriptUri: function()
    {
        // Read from the document where we loaded this script from...
        var allScripts = document.getElementsByTagName('script');
        var lastLoadedScript = allScripts[allScripts.length - 1]; // Note: We can do this because scripts are guaranteed to be loaded and executed in order unless loaded async.
        return lastLoadedScript.src;
    }
}

var Viridity = function(options)
{
    var v =
    {
        debugVerbosity: 0,

        connectionMethod: ConnectionMethod.Auto,
        useBinaryProtocol: true,

        socket: null,

        eventSource: null,

        location: "",
        fullLocation: "",

        timeout: 120000,
        pause: 1000 / 30 /*fps*/, // Sets frequency for GET and POST when long polling
        sessionId: "",
        connected: false,

        inputEvents: [],
        inputEventsStarted: false,

        autoDowngrade: true,
        reconnectFirstTry: true,
        reconnectTriesBeforeDowngrade: 10,

        reconnectTries: 0,
        reconnecting: false,
        reconnectTimer: null,
        reconnectInterval: 500,
        reconnectMaxInterval: 10000,

        nextFreeTargetId: 0,
        targetCallbacks: {},

        supportsBinaryWebSockets: (function()
        {
            if (!"WebSocket" in window)
                return false;

            if ("binaryType" in WebSocket.prototype) // Works in FF and IE11
                return true;

            try // Dirty fallback...
            {
                var wstest = new WebSocket((location.protocol == "https:" ? "wss" : "ws") + "://.");
                if (typeof(wstest.binaryType) !== "undefined")
                    return true;
                else
                    return false;
            }
            catch (e)
            {
                return false;
            }
            finally
            {
                if (wstest)
                    wstest.close();

                wstest = null;
            }
        })(),

        callbacks: {
            sessionStart: [],
            sessionReattached: [],
            sessionInUse: [],
            sessionDisconnected: []
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
            if (v.debugVerbosity > 1)
                console.log("Now triggering event: " + eventName + " params: " + JSON.stringify(params));

            var callbacks = v.callbacks[eventName];

            for (var i = 0; i < callbacks.length; ++i)
            {
                try
                {
                    callbacks[i](params);
                }
                catch (e)
                {
                    console.log("Viridity: Error occurred while executing callback " + callbacks[i] + " for event " + eventName + ": " + e);
                }
            }
        },

        _longPollingSendInputEvents: function()
        {
            if (!v.connected)
                return;

            if (v.inputEvents.length > 0)
            {
                var data = v.inputEvents.join("\n");
                v.inputEvents = [];

                if (v.debugVerbosity > 1)
                    console.log("Now posting messages to server:\n" + data);

                var options =
                {
                    type: "POST",
                    url: v.fullLocation + "/" + v.sessionId + "/v",
                    async: true,
                    cache: false,
                    timeout: v.timeout,
                    data: data,
                    crossDomain: true,
                    xhrFields: {
                        withCredentials: true
                    },

                    success: function(data)
                    {
                        setTimeout(function() { v._longPollingSendInputEvents() }, v.pause);
                    },

                    error: function(xhr, status, exception)
                    {
                        console.log("While sending input events \"" + data + "\":\n" + status + " - " + exception + "\n");

                        if (status != "timeout")
                            v._handleDisconnect();
                        else
                            setTimeout(function() { v._longPollingSendInputEvents() }, v.pause);
                    }
                };

                jQuery.ajax(options);
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
            var param = !v.inputEventsStarted || !v.connected ? "?a=init" : "";

            var options =
            {
                type: "GET",
                url: v.fullLocation + "/" + v.sessionId + "/v" + param,
                async: true,
                cache: false,
                timeout: v.timeout,
                crossDomain: true,
                xhrFields: {
                    withCredentials: true
                },

                success: function(data)
                {
//                    console.log("Got data: " + data);
                    var lines = data.split("\n");

                    for (var i = 0, ii = lines.length; i < ii; i++)
                    {
//                        console.log("line " + i + ": " + lines[i] + "\n");
                        v.processMessage(lines[i]);
                    }

                    if (v.connected)
                    {
                        setTimeout(function() { v._longPollingReceiveOutputMessages() }, v.pause);
                        v._longPollingCheckInputEventsStarted();
                    }
                },
                error: function(xhr, status, exception)
                {
                    console.log("error receiving data:\n" + status + " - " + exception + "\n");

                    if (status != "timeout")
                        v._handleDisconnect();
                    else
                        setTimeout(function() { v._longPollingReceiveOutputMessages() }, v.pause);
                }
            };

            jQuery.ajax(options);
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
            var isBinary = typeof(msg) != "string";

            var msgView = isBinary ? new Uint8Array(msg) : msg;

            var paramStartIndex = msgView.indexOfChar("(");

            var bm = msgView.slice(0, paramStartIndex);
            var indexOfMarker = bm.indexOfChar(">");

            var targetId = undefined;

            if (indexOfMarker > -1)
            {
                targetId = msgView.slice(0, indexOfMarker);
                msgView = msgView.slice(indexOfMarker + 1);
                paramStartIndex -= indexOfMarker + 1;
            }

            var paramEndIndex = msgView.indexOfChar(")");
            var dataStartIndex = paramEndIndex + 2;

            var command = msgView.slice(0, paramStartIndex);
            var params = msgView.slice(paramStartIndex + 1, paramEndIndex);

            if (isBinary)
            {
                if (targetId)
                    targetId = targetId.convertToString();

                command = command.convertToString();
                params = params.convertToString();
            }

            var splitParams = params.split(/[\s,]+/);

            var t = {
                message: msgView,
                targetId: targetId,
                paramStartIndex: paramStartIndex,
                paramEndIndex: paramEndIndex,
                dataStartIndex: dataStartIndex,
                command: command,
                params: splitParams,
                data: function(start, end)
                {
                    start = t.dataStartIndex + (typeof(start) == "number" ? start : 0);
                    end = typeof(end) == "number" ? t.dataStartIndex + end : undefined;
                    return t.message.slice(start, end);
                },
                dataAsString: function(start, end) { return !isBinary ? t.data(start, end) : t.data(start, end).convertToString(); },
                dataIsBinary: isBinary
            }

            if (v.debugVerbosity > 3)
                console.log("t: " + JSON.stringify(t, null, " "));

            return t;
        },

        processMessage: function(msg)
        {
            var t = v.parseMessage(msg);
            var processed = false;

            if (v.debugVerbosity > 2)
                console.log("Received message from server: " + t.targetId + ">" + t.message.convertToString());

            if (typeof(t.targetId) !== "undefined" &&
                t.targetId in v.targetCallbacks)
            {
                processed = v.targetCallbacks[t.targetId](t);
            }

            var inputParams = t.params;

            if (t.command === "info")
            {
                v.sessionId = inputParams[0];
                v.connected = true;
                v._triggerCallback("sessionStart", v.sessionId);
            }
            else if (t.command === "reattached")
            {
                v.sessionId = inputParams[0];
                v.connected = true;
                v._triggerCallback("sessionReattached", v.sessionId);
            }
            else if (t.command === "inuse")
            {
                v.sessionId = inputParams[0];
                v.connected = false;
                v._triggerCallback("sessionInUse", v.sessionId);
            }
            else if (t.command === "ping")
            {
                v.sendMessage("pong()");
            }

            if (v.reconnecting)
            {
                clearTimeout(v.reconnectTimer);
                v.reconnecting = false;
                v.reconnectTries = 0;
            }
        },

        sendMessage: function(msg, targetId)
        {
            if (typeof(targetId) !== "undefined" && targetId !== "")
                msg = targetId + ">" + msg;

            if (v.debugVerbosity > 0)
                console.log("Sending message to server: " + msg);

            if (v.connectionMethod === ConnectionMethod.WebSockets)
            {
                if (v.socket && v.socket.readyState === WebSocket.OPEN)
                    v.socket.send(msg);
                else
                    v.inputEvents.push(msg);
            }
            else if (v.connectionMethod === ConnectionMethod.LongPolling ||
                     v.connectionMethod === ConnectionMethod.ServerSentEvents)
                v.inputEvents.push(msg);
            else
                v.inputEvents.push(msg);
        },

        _webSocketsSendQueuedMessages: function()
        {
            if (v.socket && v.socket.readyState === WebSocket.OPEN)
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

        _validateConnectionMethodFor: function(connectionMethod)
        {
            if (connectionMethod === ConnectionMethod.Auto)
                connectionMethod = ConnectionMethod.WebSockets;

            if (connectionMethod === ConnectionMethod.WebSockets && !("WebSocket" in window))
                connectionMethod = ConnectionMethod.ServerSentEvents;

            if (connectionMethod === ConnectionMethod.ServerSentEvents && !("EventSource" in window))
                connectionMethod = ConnectionMethod.LongPolling;

            return connectionMethod;
        },

        _getDowngradedConnectionMethodFor: function(connectionMethod)
        {
            if (connectionMethod === ConnectionMethod.Auto)
                connectionMethod = ConnectionMethod.WebSockets;

            if (connectionMethod > ConnectionMethod.LongPolling)
                --connectionMethod;

            return v._validateConnectionMethodFor(connectionMethod);
        },

        _handleDisconnect: function()
        {
            clearTimeout(v.reconnectTimer);

            if (v.debugVerbosity > 1)
                console.log("Disconnected. Retry no. " + (v.reconnectTries + 1));

            if (v.autoDowngrade &&
                (v.reconnectFirstTry ||
                 (v.reconnectTries > 0 && v.connectionMethod !== ConnectionMethod.LongPolling && v.reconnectTries % v.reconnectTriesBeforeDowngrade == 0)
                )
               )
            {
                var newConnectionMethod = v._getDowngradedConnectionMethodFor(v.connectionMethod);

                if (v.debugVerbosity > 1)
                    console.log("Downgrading connection from " + v.connectionMethod + " to " + newConnectionMethod);

                v.connectionMethod = newConnectionMethod;
            }

            if (!v.reconnecting)
            {
                if (v.debugVerbosity > 1)
                    console.log("Starting reconnection...");

                v.reconnectTries = 0;
                v.inputEventsStarted = false;
                v._triggerCallback("sessionDisconnected", v.sessionId);
                v.connected = false;
                v.reconnecting = true;
            }

            ++v.reconnectTries;
            v.reconnectFirstTry = false;

            var newTimeout = Math.min(v.reconnectTries * v.reconnectInterval, v.reconnectMaxInterval);

            if (v.debugVerbosity > 1)
                console.log("Setting new reconnection time out to " + newTimeout + " ms.");

            v.reconnectTimer = setTimeout(v.connect, newTimeout);
        },

        _ensureWebSocketIsClosed: function()
        {
            if (v.socket) // Explicitly close previous connection to avoid FF from reconnecting again...
            try
            {
                v.socket.onclose = function () {};

                if (v.socket.readyState !== WebSocket.CLOSED)
                    v.socket.close();
            }
            catch (e)
            {
                if (v.debugVerbosity > 1)
                    console.log("_ensureWebSocketIsClosed: " + e);
            }

            v.socket = null;
        },

        _ensureEventSourceIsClosed: function()
        {
            // Explicitly close previous connection to avoid browser initiated reconnect...
            if (v.eventSource)
            try
            {
                v.eventSource.close();
            }
            catch (e)
            {
                if (v.debugVerbosity > 1)
                    console.log("_ensureEventSourceIsClosed: " + e);
            }

            v.eventSource = null;
        },

        connect: function(options)
        {
            if (v.debugVerbosity > 0)
                console.log("Connecting to server: " + JSON.stringify(options));

            if (typeof(options) == "object")
                v.init(options);

            if (v.connectionMethod === ConnectionMethod.LongPolling)
            {
                v._longPollingReceiveOutputMessages();
            }
            else if (v.connectionMethod === ConnectionMethod.ServerSentEvents)
            {
                v._ensureEventSourceIsClosed();

                v.eventSource = new EventSource(v.fullLocation + "/" + v.sessionId + "/v/ev");
                v.eventSource.onmessage = v._serverSentEventsMessageReceived;
                v.eventSource.onerror = function() { v._ensureEventSourceIsClosed(); v._handleDisconnect(); };
            }
            else if (v.connectionMethod === ConnectionMethod.WebSockets)
            {
                var ws = (v.fullLocation.indexOf("https:") > -1 ? "wss:" : "ws:");

                v._ensureWebSocketIsClosed();

                var add = ws + "//" + v.location + "/" + v.sessionId + "/v/ws";

                try
                {
                    if (v.useBinaryProtocol && v.supportsBinaryWebSockets)
                    {
                        v.socket = new WebSocket(add + "b");
                        v.socket.binaryType = "arraybuffer";
                    }
                    else
                        v.socket = new WebSocket(add);

                    v.socket.onmessage = function(msg) { v.processMessage(msg.data) };
                    v.socket.onopen = v._webSocketsSendQueuedMessages;
                    v.socket.onerror = function() { v._ensureWebSocketIsClosed(); v._handleDisconnect(); }
                    v.socket.onclose = function() { v._ensureWebSocketIsClosed(); v._handleDisconnect(); }
                }
                catch (e)
                {
                    if (v.debugVerbosity > 1)
                        console.log("Error on connect: " + e);

                    v._handleDisconnect();
                }
            }
        },

        init: function(options)
        {
            if (typeof(options.autoDowngrade) !== "undefined")
                v.autoDowngrade = options.autoDowngrade;

            if (typeof(options.debugVerbosity) !== "undefined")
                v.debugVerbosity = options.debugVerbosity;

            if (typeof(options.useBinaryProtocol) !== "undefined")
                v.useBinaryProtocol = options.useBinaryProtocol;

            if (v.debugVerbosity > 0)
                console.log("Initializing: " + JSON.stringify(options));

            v.connectionMethod = v._validateConnectionMethodFor(options.connectionMethod);

            if (typeof(options.sessionId) != "undefined" && options.sessionId != "")
                v.sessionId = options.sessionId;

            var l = ViridityHelper.parseOriginUri(options.originUri);

            v.location = l.location;
            v.fullLocation = l.fullLocation;

            if (v.debugVerbosity > 0)
            {
                console.log("v.location: " + v.location);
                console.log("v.fullLocation: " + v.fullLocation);
            }
        }
    }

    if (typeof(options.autoConnect) == "undefined" || options.autoConnect)
        v.connect(options);
    else
        v.init(options);

    return v;
};
