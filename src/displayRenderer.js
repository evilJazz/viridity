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

var DisplayRenderer = function() {

    var debugVerbosity = 0;
    var debugDraw = 0;

    var dr =
    {
        useLongPolling: true,

        socket: 0,
        canvas: 0,
        ctx: 0,

        lastFrame: 0,
        frameImageCount: 0,
        frameCommands: [],

        frontCanvas: 0,
        frontCtx: 0,

        location: "",

        timeout: 2000,
        pause: 50,
        connectionId: "",

        inputEvents: [],

        _imageDone: function()
        {
            --dr.frameImageCount;
            dr._determineReadyState();
        },

        _determineReadyState: function()
        {
            if (debugVerbosity > 1) console.log("_imageDone: " + dr.frameImageCount)
            if (dr.frameImageCount === 0)
            {
                dr._flipToFront();
                if (debugDraw)
                    dr.debugDraw();

                if (debugVerbosity > 1) console.log("Sending ready...");
                if (!dr.useLongPolling)
                    dr.socket.send("ready()");
                else
                    dr.inputEvents.push("ready()");
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
            if (!dr.useLongPolling)
                dr.socket.send("requestFullUpdate()");
            else
                dr.inputEvents.push("requestFullUpdate()");
        },

        debugClearCanvas: function()
        {
            // Store the current transformation matrix
            dr.ctx.save();

            // Use the identity matrix while clearing the canvas
            dr.ctx.setTransform(1, 0, 0, 1, 0, 0);
            dr.ctx.clearRect(0, 0, canvas.width, canvas.height);

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

        debugDraw: function()
        {
            var alpha = 0.5;

            for (var i = 0; i < dr.frameCommands.length; ++i)
            {
                var frameCmd = dr.frameCommands[i];
                var command = frameCmd.command;
                var inputParams = frameCmd.params;

                dr.frontCtx.strokeStyle = "rgba(0, 0, 0, 0.8)";
                dr.frontCtx.lineWidth = 0.2;

                if (command === "fillRect")
                {
                    dr.frontCtx.fillStyle = "rgba(0, 255, 0, " + alpha + ")";
                    dr.frontCtx.fillRect(inputParams[1], inputParams[2], inputParams[3], inputParams[4]);
                    dr.frontCtx.strokeRect(inputParams[1], inputParams[2], inputParams[3], inputParams[4]);
                }
                else if (command === "moveImage")
                {
                    dr.frontCtx.fillStyle = "rgba(0, 0, 255, " + alpha + ")";
                    dr.frontCtx.fillRect(inputParams[5], inputParams[6], inputParams[3], inputParams[4]);
                    dr.frontCtx.strokeRect(inputParams[5], inputParams[6], inputParams[3], inputParams[4]);
                }
                else if (command === "drawImage")
                {
                    dr.frontCtx.fillStyle = "rgba(255, 0, 0, " + alpha + ")";
                    dr.frontCtx.fillRect(inputParams[1], inputParams[2], inputParams[3], inputParams[4]);
                    dr.frontCtx.strokeRect(inputParams[1], inputParams[2], inputParams[3], inputParams[4]);
                }
            }
        },

        sendInputEvents: function()
        {
            if (dr.inputEvents.length > 0)
            {
                var data = dr.inputEvents.splice(0).join("\n");
                var options =
                {
                    type: "POST",
                    url: "display?id=" + dr.connectionId,
                    async: false,
                    cache: false,
                    timeout: dr.timeout,
                    data: data,

                    success: function(data)
                    {
                        setTimeout(function() { dr.sendInputEvents() }, dr.pause);
                    },

                    error: function(xhr, status, exception)
                    {
                        console.log("error while sending input events \"" + data + "\":\n" + status + " (" + exception + ")");
                        setTimeout(function() { dr.sendInputEvents() }, dr.pause);
                    }
                };

                $.ajax(options);
            }
            else
            {
                setTimeout(function() { dr.sendInputEvents() }, dr.pause);
            }
        },

        receiveOutputMessages: function()
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

                    setTimeout(function() { dr.receiveOutputMessages() }, dr.pause);
                },
                error: function(xhr, status, exception)
                {
                    console.log("error: " + status + " (" + exception + ")");
                    setTimeout(function() { dr.receiveOutputMessages() }, dr.pause);
                }
            };

            $.ajax(options);
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
                if (debugVerbosity) console.log("NEW FRAME: " + dr.lastFrame + " -> " + frame);
                dr.lastFrame = frame;

                if (debugDraw)
                {
                    dr._flipToFront(); // overwrite debug rects...
                    dr.frameCommands = [];
                }
            }

            if (debugVerbosity) console.log("command: " + command + " params: " + JSON.stringify(inputParams));
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
                $("#connectionId").text(inputParams[0]);
                dr.connectionId = inputParams[0];
            }
            else if (command === "end")
            {
                if (debugVerbosity) console.log("Frame end " + frame + " received...");
                dr._determineReadyState();
            }
        },

        sendCommand: function(command)
        {
            var options =
            {
                type: "POST",
                url: "command?id=" + dr.connectionId,
                async: true,
                timeout: dr.timeout,
                data: command,

                success: function(data)
                {
                    console.log("command " + command + "answered with " + data);
                    $("#commandResponse").show().text(data).fadeOut(2000);
                },

                error: function(xhr, status, exception)
                {
                    console.log("error while sending command: " + status + " (" + exception + ")");
                    $("#commandResponse").show().text(status + " " + exception).fadeOut(2000);
                }
            };

            $.ajax(options);
        },

        init: function(useLongPolling)
        {
            useLongPolling = useLongPolling || false;
            dr.useLongPolling = useLongPolling;

            var hostWithPath = window.location.host + window.location.pathname;
            hostWithPath = hostWithPath.replace(/\/$/, "");
            dr.location = hostWithPath;

            if (!dr.useLongPolling)
                dr.socket = new WebSocket("ws://" + dr.location + "/display");

            dr.canvas = document.getElementById("canvasBack");
            dr.ctx = dr.canvas.getContext("2d");

            dr.frontCanvas = document.getElementById("canvas");
            dr.frontCtx = dr.frontCanvas.getContext("2d");

            var BlobBuilder = window.BlobBuilder || window.WebKitBlobBuilder || window.MozBlobBuilder || window.MSBlobBuilder;
            var URL = window.URL || window.webKitURL;
            var useBlobBuilder = BlobBuilder && URL;
            //var useBlobBuilder = false;

            if (dr.canvas.width != 1024 || dr.canvas.height != 768)
            {
                dr.frontCanvas.width = dr.canvas.width = 1024;
                dr.frontCanvas.height = dr.canvas.height = 768;
            }

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
                var pos = getCanvasPos(event)

                if (!dr.useLongPolling)
                {
                    if (other)
                        dr.socket.send(type + "(" + pos.x + "," + pos.y + "," + event.which + "," + getModifiers(event) + "," + other + ")");
                    else
                        dr.socket.send(type + "(" + pos.x + "," + pos.y + "," + event.which + "," + getModifiers(event) + ")");
                }
                else
                {
                    if (other)
                        dr.inputEvents.push(type + "(" + pos.x + "," + pos.y + "," + event.which + "," + getModifiers(event) + "," + other + ")");
                    else
                        dr.inputEvents.push(type + "(" + pos.x + "," + pos.y + "," + event.which + "," + getModifiers(event) + ")");
                }

                event.stopPropagation();
                event.preventDefault();
            }

            function sendKeyEvent(type, event)
            {
                if (!dr.useLongPolling)
                {
                    if (event.hasOwnProperty("which"))
                        dr.socket.send(type + "(" + event.which + "," + getModifiers(event) + ")");
                    else
                        dr.socket.send(type + "(" + pos.x + "," + pos.y + ")");
                }
                else
                {
                    if (event.hasOwnProperty("which"))
                        dr.inputEvents.push(type + "(" + event.which + "," + getModifiers(event) + ")");
                    else
                        dr.inputEvents.push(type + "(" + pos.x + "," + pos.y + ")");
                }

                event.stopPropagation();
                event.preventDefault();
            }

            $(dr.frontCanvas).mousedown(function(event)  { sendMouseEvent("mouseDown", event) });
            $(dr.frontCanvas).mouseup(function(event)    { sendMouseEvent("mouseUp", event) });
            $(dr.frontCanvas).mousemove(function(event)  { sendMouseEvent("mouseMove", event) });
            $(dr.frontCanvas).mouseover(function(event)  { sendMouseEvent("mouseEnter", event) });
            $(dr.frontCanvas).mouseout(function(event)   { sendMouseEvent("mouseExit", event) });
            $(dr.frontCanvas).mouseenter(function(event) { sendMouseEvent("mouseEnter", event) });
            $(dr.frontCanvas).dblclick(function(event)   { sendMouseEvent("mouseDblClick", event) });

            $(dr.frontCanvas).mousewheel(function(event, delta, deltaX, deltaY) { sendMouseEvent("mouseWheel", event, deltaX + "," + deltaY); });

            $(document).keydown(function(event)    { sendKeyEvent("keyDown", event) });
            $(document).keypress(function(event)   { sendKeyEvent("keyPress", event) });
            $(document).keyup(function(event)      { sendKeyEvent("keyUp", event) });

            if (dr.useLongPolling)
            {
                $(document).ready(function() { dr.receiveOutputMessages() });
                $(document).ready(function() { dr.sendInputEvents() });
            }
            else
            {
                dr.socket.onmessage = function(msg) { dr.processMessage(msg, useBlobBuilder) };
            }
        }
    }

    return dr;
}

var displayRenderer = new DisplayRenderer();
