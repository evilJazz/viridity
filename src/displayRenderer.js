var Base64Binary = {
    _keyStr : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",

    /* will return a Uint8Array type */
    decodeArrayBuffer: function(input) {
                           var bytes = Math.ceil( (3*input.length) / 4.0);
                           var ab = new ArrayBuffer(bytes);
                           this.decode(input, ab);

                           return ab;
                       },

    decode: function(input, arrayBuffer) {
                //get last chars to see if are valid
                var lkey1 = this._keyStr.indexOf(input.charAt(input.length-1));
                var lkey2 = this._keyStr.indexOf(input.charAt(input.length-1));

                var bytes = Math.ceil( (3*input.length) / 4.0);
                if (lkey1 == 64) bytes--; //padding chars, so skip
                if (lkey2 == 64) bytes--; //padding chars, so skip

                var uarray;
                var chr1, chr2, chr3;
                var enc1, enc2, enc3, enc4;
                var i = 0;
                var j = 0;

                if (arrayBuffer)
                    uarray = new Uint8Array(arrayBuffer);
                else
                    uarray = new Uint8Array(bytes);

                input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");

                for (i=0; i<bytes; i+=3) {
                    //get the 3 octects in 4 ascii chars
                    enc1 = this._keyStr.indexOf(input.charAt(j++));
                    enc2 = this._keyStr.indexOf(input.charAt(j++));
                    enc3 = this._keyStr.indexOf(input.charAt(j++));
                    enc4 = this._keyStr.indexOf(input.charAt(j++));

                    chr1 = (enc1 << 2) | (enc2 >> 4);
                    chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
                    chr3 = ((enc3 & 3) << 6) | enc4;

                    uarray[i] = chr1;
                    if (enc3 != 64) uarray[i+1] = chr2;
                    if (enc4 != 64) uarray[i+2] = chr3;
                }

                return uarray;
            }
}

var ConnectionMethod = {
    Auto: -1,
    LongPolling: 0,
    ServerSentEvents: 1,
    WebSockets: 2
};

(function($)
{
    $.fn.viridity = function(options)
    {
        var containerElement = this;

        var debugVerbosity = 0;
        var debugDraw = 0;

        var originHref = window.location.href;

        var dr =
        {
            connectionMethod: ConnectionMethod.Auto,

            socket: 0,

            eventSource: 0,

            canvas: 0,
            ctx: 0,

            lastFrame: 0,
            frameEndReceived: true,
            frameImageCount: 0,
            frameCommands: [],

            frontCanvas: 0,
            frontCtx: 0,

            location: "",
            fullLocation: "",

            timeout: 120000,
            //timeout: 2000,
            pause: 1000 / 30 /*fps*/, // Sets frequency for GET and POST when long polling
            connectionId: "",

            inputEvents: [],
            inputEventsStarted: false,

            onNewCommandReceived: undefined,

            _imageDone: function()
            {
                --dr.frameImageCount;
                dr._determineReadyState();
            },

            _determineReadyState: function()
            {
                if (debugVerbosity > 1) console.log("_imageDone: " + dr.frameImageCount)
                if (dr.frameEndReceived && dr.frameImageCount == 0)
                {
                    dr._flipToFront();
                    if (debugDraw)
                        dr._debugDraw();

                    if (debugVerbosity > 1) console.log("SENDING READY!!!!");

                    dr.sendMessage("ready()", true);
                }
            },

            _flipToFront: function()
            {
                dr.frontCanvas.width = dr.canvas.width;
                dr.frontCanvas.height = dr.canvas.height;
                dr.frontCtx.drawImage(dr.canvas, 0, 0);
            },

            requestFullUpdate: function()
            {
                dr.sendMessage("requestFullUpdate()", true);
            },

            createDebugOverlay: function()
            {
                var container = $(containerElement);

                var debugControls = $("<div/>")
                    .css("position", "absolute")
                    .css("left", 0)
                    .css("top", 0)
                    .css("padding", 10)
                    .css("background", "rgba(255, 255, 255, 0.9)")
                    .css("z-index", 2);

                debugControls.append("<button>Clear Canvas</button>").click(dr.debugClearCanvas);
                debugControls.append("<button>Request full frame</button>").click(dr.requestFullUpdate);

                var debugDrawCheckbox = $("<input type='checkbox'/>");
                debugDrawCheckbox.change(function() { dr.setDebugDrawEnabled(this.checked); });
                debugDrawCheckbox.appendTo(debugControls);

                debugControls.append("<label> Debug drawing </label>");

                debugControls.appendTo(container);
            },

            debugClearCanvas: function()
            {
                // Store the current transformation matrix
                dr.ctx.save();

                // Use the identity matrix while clearing the canvas
                dr.ctx.setTransform(1, 0, 0, 1, 0, 0);
                dr.ctx.clearRect(0, 0, dr.frontCanvas.width, dr.frontCanvas.height);

                // Restore the transform
                dr.ctx.restore();
            },

            debugDrawEnabled: function()
            {
                return debugDraw;
            },

            setDebugDrawEnabled: function(value)
            {
                debugDraw = value;
            },

            _debugDraw: function()
            {
                var alpha = 0.2;

                for (var i = 0; i < dr.frameCommands.length; ++i)
                {
                    var frameCmd = dr.frameCommands[i];
                    var command = frameCmd.command;
                    var inputParams = frameCmd.params;

                    dr.frontCtx.strokeStyle = "rgba(0, 0, 0, 0.8)";
                    dr.frontCtx.lineWidth = 0.2;

                    var x1 = parseInt(inputParams[1]);
                    var y1 = parseInt(inputParams[2]);
                    var w = parseInt(inputParams[3]);
                    var h = parseInt(inputParams[4]);

                    if (command === "fillRect")
                    {
                        dr.frontCtx.fillStyle = "rgba(0, 255, 0, " + alpha + ")";
                        dr.frontCtx.fillRect(x1, y1, w, h);
                        dr.frontCtx.strokeRect(x1, y1, w, h);
                    }
                    else if (command === "moveImage")
                    {
                        var x2 = parseInt(inputParams[5]);
                        var y2 = parseInt(inputParams[6]);

                        var centerX1 = x1 + w / 2;
                        var centerY1 = y1 + h / 2;

                        var centerX2 = x2 + w / 2;
                        var centerY2 = y2 + h / 2;

                        // Draw source rect
                        dr.frontCtx.fillStyle = "rgba(255, 0, 255, " + alpha + ")";
                        dr.frontCtx.fillRect(x1, y1, w, h);
                        dr.frontCtx.strokeRect(x1, y1, w, h);

                        dr.frontCtx.moveTo(x1, y1)
                        dr.frontCtx.lineTo(x1 + w, y1 + h);
                        dr.frontCtx.moveTo(x1 + w, y1)
                        dr.frontCtx.lineTo(x1, y1 + h);
                        dr.frontCtx.stroke();

                        // Draw destination rect
                        dr.frontCtx.fillStyle = "rgba(0, 0, 255, " + alpha + ")";
                        dr.frontCtx.fillRect(x2, y2, w, h);
                        dr.frontCtx.strokeRect(x2, y2, w, h);

                        dr.frontCtx.moveTo(x2, y2)
                        dr.frontCtx.lineTo(x2 + w, y2 + h);
                        dr.frontCtx.moveTo(x2 + w, y2)
                        dr.frontCtx.lineTo(x2, y2 + h);
                        dr.frontCtx.stroke();

                        // Draw vector
                        dr.frontCtx.strokeStyle = "rgba(255, 255, 255, 1)";
                        dr.frontCtx.fillStyle = "rgba(255, 255, 255, 1)";

                        // Draw vector line butt
                        dr.frontCtx.beginPath();
                        dr.frontCtx.arc(centerX1, centerY1, 2, 0, 2 * Math.PI, false);
                        dr.frontCtx.fill();
                        dr.frontCtx.stroke();

                        // Draw vector line with arrow
                        dr.frontCtx.beginPath();
                        dr.frontCtx.lineWidth = 1;

                        function drawArrow(context, fromx, fromy, tox, toy)
                        {
                            var headlen = 5;   // length of head in pixels
                            var angle = Math.atan2(toy - fromy, tox - fromx);
                            context.moveTo(fromx, fromy);
                            context.lineTo(tox, toy);
                            context.lineTo(tox - headlen * Math.cos(angle - Math.PI / 6), toy - headlen * Math.sin(angle - Math.PI / 6));
                            context.moveTo(tox, toy);
                            context.lineTo(tox - headlen * Math.cos(angle + Math.PI / 6), toy - headlen * Math.sin(angle + Math.PI / 6));
                        }

                        drawArrow(dr.frontCtx, centerX1, centerY1, centerX2, centerY2);
                        dr.frontCtx.stroke();
                    }
                    else if (command === "drawImage")
                    {
                        dr.frontCtx.fillStyle = "rgba(255, 0, 0, " + alpha + ")";
                        dr.frontCtx.fillRect(x1, y1, w, h);
                        dr.frontCtx.strokeRect(x1, y1, w, h);
                    }
                }
            },

            _longPollingSendInputEvents: function()
            {
                if (dr.inputEvents.length > 0)
                {
                    var data = dr.inputEvents.join("\n");
                    dr.inputEvents = [];
                    var options =
                    {
                        type: "POST",
                        url: dr.fullLocation + "/display?id=" + dr.connectionId,
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
                    url: "display?id=" + dr.connectionId,

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

            processPlainMessage: function(data, useBlobBuilder)
            {
                var msg = {};
                msg["data"] = data;
                dr.processMessage(msg, useBlobBuilder);
            },

            processMessage: function(msg, useBlobBuilder)
            {
                var paramStartIndex = msg.data.indexOf("(");
                var paramEndIndex = msg.data.indexOf(")");

                var command = msg.data.substring(0, paramStartIndex);
                var params = msg.data.substring(paramStartIndex + 1, paramEndIndex);
                var inputParams = params.split(/[\s,]+/);

                var frame = inputParams[0];

                if (dr.lastFrame !== frame)
                {
                    if (debugVerbosity > 1) console.log("NEW FRAME: " + dr.lastFrame + " -> " + frame);
                    dr.lastFrame = frame;

                    if (dr.frameImageCount != 0)
                    {
                        console.log("PREVIOUS FRAME NOT COMPLETELY RENDERED!!!!! Patches left: " + dr.frameImageCount);
                    }

                    // This is to stop _determineReadyState() from sending ready() when image loading is quasi-synchronous, ie. base64 encoded sources.
                    dr.frameEndReceived = false;

                    if (debugDraw)
                    {
                        dr._flipToFront(); // overwrite debug rects...
                        dr.frameCommands = [];
                    }
                }

                if (debugVerbosity > 1) console.log("command: " + command + " params: " + JSON.stringify(inputParams));
                if (debugDraw)
                {
                    var frameCmd =
                    {
                        command: command,
                        params: inputParams
                    }

                    dr.frameCommands.push(frameCmd);
                }

                if (command === "fillRect")
                {
                    dr.ctx.fillStyle = inputParams[5]
                    dr.ctx.fillRect(
                                inputParams[1], inputParams[2], inputParams[3], inputParams[4]
                                );
                }
                else if (command === "moveImage")
                {
                    dr.ctx.drawImage(
                                dr.frontCanvas,
                                inputParams[1], inputParams[2], inputParams[3], inputParams[4],
                                inputParams[5], inputParams[6], inputParams[3], inputParams[4]
                                );
                }
                else if (command === "drawImage")
                {
                    ++dr.frameImageCount;

                    var img = new Image;
                    img.onload = function()
                    {
                        if (frame !== dr.lastFrame)
                            console.log("ASYNCHRONOUS IMAGE!!!!! " + " frame is " + frame + ", but dr.lastFrame is " + dr.lastFrame);

                        if (debugVerbosity > 1) console.log("frame: " + frame + " img.src: " + img.src);

                        dr.ctx.clearRect(inputParams[1], inputParams[2], inputParams[3], inputParams[4])
                        dr.ctx.drawImage(img, inputParams[1], inputParams[2]);

                        if (useBlobBuilder)
                            URL.revokeObjectURL(img.src);

                        dr._imageDone();
                    };

                    var imageData = msg.data.slice(paramEndIndex + 2);

                    if (imageData.substring(0,3) === "fb:")
                    {
                        img.src = imageData.substring(3);
                    }
                    else if (imageData.substring(0,4) === "http")
                    {
                        img.src = imageData;
                    }
                    else if (useBlobBuilder)
                    {
                        var blobber = new BlobBuilder;

                        var buf = Base64Binary.decodeArrayBuffer(imageData);

                        /*
                        var rawData = atob(imageData);

                        var buf = new ArrayBuffer(rawData.length);
                        var view = new Uint8Array(buf);
                        for (var i = 0; i < view.length; i++)
                            view[i] = rawData.charCodeAt(i);
                        */

                        blobber.append(buf);

                        var contentType = inputParams[5].split(";")[0]; // "image/jpeg" etc. remove base64
                        //console.log("contentType: " + contentType);

                        var blob = blobber.getBlob(contentType);
                        var blobUrl = URL.createObjectURL(blob);
                        img.src = blobUrl;
                    }
                    else
                        img.src = "data:" + inputParams[5] + "," + imageData;
                }
                else if (command === "info")
                {
                    dr.connectionId = inputParams[0];
                    dr.frameEndReceived = true;
                }
                else if (command === "command")
                {
                    var responseId = inputParams[0];
                    var input = inputParams[1];
                    if (typeof(dr.onNewCommandReceived) == "function")
                    {
                        var result = dr.onNewCommandReceived(input);
                        dr.sendMessage("commandResponse(" + responseId + "," + result + ")");
                    }
                }
                else if (command === "end")
                {
                    if (debugVerbosity > 1) console.log("Frame end " + frame + " received...");
                    dr.frameEndReceived = true;
                    dr._determineReadyState();
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
                var options =
                {
                    type: "POST",
                    url: dr.fullLocation + "/command?id=" + dr.connectionId,
                    async: true,
                    timeout: dr.timeout,
                    data: command,

                    success: function(data)
                    {
                        console.log("command " + command + "answered with " + data);                       
                        if (typeof(callback) == "function")
                            callback(data);
                    },

                    error: function(xhr, status, exception)
                    {
                        console.log("error while sending command:\n" + status + " - " + exception + "\n");
                    }
                };

                $.ajax(options);
            },

            reconnect: function()
            {
                location.reload(); // For now.
            },

            resize: function(width, height)
            {
                if (dr.frontCanvas.width != width || dr.frontCanvas.height != height)
                {
                    console.log("width: " + width + " height: " + height);
                    $(dr.frontCanvas).css("width", width);
                    $(dr.frontCanvas).css("height", height);
                    dr.frontCanvas.width = dr.canvas.width = width;
                    dr.frontCanvas.height = dr.canvas.height = height;

                    dr.sendMessage("resize(" + width + "," + height + ")");

                    dr.requestFullUpdate();
                }
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

                dr.canvas = document.createElement("canvas");
                dr.ctx = dr.canvas.getContext("2d");

                dr.frontCanvas = document.getElementById("canvas");
                dr.frontCanvas = document.createElement("canvas");
                dr.frontCtx = dr.frontCanvas.getContext("2d");

                if (dr.canvas.width != 1024 || dr.canvas.height != 768)
                {
                    dr.frontCanvas.width = dr.canvas.width = 1024;
                    dr.frontCanvas.height = dr.canvas.height = 768;
                }

                $(containerElement).append(dr.frontCanvas);
                $(containerElement).resize(function()
                {
                    var container = $(this);
                    dr.resize(container.width(), container.height());
                });

                //dr.createDebugOverlay();

                var BlobBuilder = window.BlobBuilder || window.WebKitBlobBuilder || window.MozBlobBuilder || window.MSBlobBuilder;
                var URL = window.URL || window.webKitURL;
                var useBlobBuilder = BlobBuilder && URL;
                //var useBlobBuilder = false;

                function getCanvasPos(event)
                {
                    var offset = $(dr.frontCanvas).offset();
                    var posX = event.pageX - offset.left;
                    var posY = event.pageY - offset.top;
                    return { x: posX, y: posY };
                }

                function getModifiers(event)
                {
                    var modifiers = 0;

                    if (event.hasOwnProperty("shiftKey") && event.shiftKey)
                        modifiers |= 0x02000000;

                    if (event.hasOwnProperty("ctrlKey") && event.ctrlKey)
                        modifiers |= 0x04000000;

                    if (event.hasOwnProperty("altKey") && event.altKey)
                        modifiers |= 0x08000000;

                    if (event.hasOwnProperty("metaKey") && event.metaKey)
                        modifiers |= 0x10000000;

                    return modifiers;
                }

                function sendMouseEvent(type, event, other)
                {
                    var pos = getCanvasPos(event);

                    // Round coordinates, because IE is sending floats...
                    pos.x = Math.round(pos.x);
                    pos.y = Math.round(pos.y);

                    if (other)
                        dr.sendMessage(type + "(" + pos.x + "," + pos.y + "," + event.which + "," + getModifiers(event) + "," + other + ")");
                    else
                        dr.sendMessage(type + "(" + pos.x + "," + pos.y + "," + event.which + "," + getModifiers(event) + ")");

                    event.stopPropagation();
                    event.preventDefault();
                }

                function isNonPrintableKey(keyCode)
                {
                    return keyCode !== 32 && // Space
                           keyCode !== 0 && // Firefox sends 0 for umlauts in keyDown/keyUp
                           (
                               keyCode < 48 || // All control key below key "0"
                               keyCode === 91 || // Win key
                               keyCode === 93 || // Menu key
                               (keyCode >= 112 && keyCode <= 123) || // Function keys
                               //keyCode === 144 || // NumLock
                               keyCode === 145 // ScollLock
                           );
                }

                function sendKeyEvent(type, event, printableCharacter)
                {
                    if (debugVerbosity > 0)
                    {
                        console.log("Type: " + type + " event.which: " + event.which + " event.keyCode: " + event.keyCode + " event.charCode: " + event.charCode + " printableCharacter: " + printableCharacter);
                        //console.dir(event);
                    }

                    if (event.hasOwnProperty("which"))
                        dr.sendMessage(type + "(" + event.which + "," + getModifiers(event) + ")");
                    else
                        dr.sendMessage(type + "(" + event.keyCode + "," + getModifiers(event) + ")");

                    if (printableCharacter || isNonPrintableKey(event.keyCode))
                    {
                        event.stopPropagation();
                        event.preventDefault();
                    }
                }

                function focusCanvas()
                {
                    $(document).focus();
                    $(dr.frontCanvas).focus();
                }

                $(dr.frontCanvas).attr("tabindex", 0); // Make canvas focusable!

                $(dr.frontCanvas).mousedown(function(event)  { focusCanvas(); sendMouseEvent("mouseDown", event); });
                $(dr.frontCanvas).mouseup(function(event)    { sendMouseEvent("mouseUp", event); });
                $(dr.frontCanvas).mousemove(function(event)  { sendMouseEvent("mouseMove", event); });
                $(dr.frontCanvas).mouseover(function(event)  { sendMouseEvent("mouseEnter", event); });
                $(dr.frontCanvas).mouseout(function(event)   { sendMouseEvent("mouseExit", event); });
                $(dr.frontCanvas).mouseenter(function(event) { sendMouseEvent("mouseEnter", event); });
                $(dr.frontCanvas).dblclick(function(event)   { sendMouseEvent("mouseDblClick", event); });

                $(dr.frontCanvas).mousewheel(function(event, delta, deltaX, deltaY) { sendMouseEvent("mouseWheel", event, deltaX + "," + deltaY); });

                $(dr.frontCanvas).keydown(function(event)    { sendKeyEvent("keyDown", event, false); });
                $(dr.frontCanvas).keypress(function(event)   { sendKeyEvent("keyPress", event, true); });
                $(dr.frontCanvas).keyup(function(event)      { sendKeyEvent("keyUp", event, false); });

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

                    dr.socket.onmessage = function(msg) { dr.processMessage(msg, useBlobBuilder) };
                    dr.socket.onopen = dr._sendQueuedMessages;
                    dr.socket.onerror = dr.reconnect;
                    dr.socket.onclose = dr.reconnect;
                }
            }
        }

        dr.init(options.connectionMethod);

        return dr;
    };
})(jQuery);
