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

var ViridityDisplayEvents = {
    KeyboardInputs: 1 << 0,

    MousePresses: 1 << 8,
    MouseDoubleClick: 1 << 9,
    MouseMoveWhenMousePressed: 1 << 10,
    MouseHover: 1 << 11,
    MouseWheel: 1 << 12,

    Touch: 1 << 16,

    ContextMenu: 1 << 24
};

(function($)
{
    $.fn.viridity = function(viridityChannel, targetId, params, options)
    {
        var containerElement = this;

        var v = viridityChannel;

        var dr =
        {
            debugVerbosity: 0,
            debugDraw: false,

            enabledEvents:
                ViridityDisplayEvents.KeyboardInputs |
                ViridityDisplayEvents.MousePresses |
                ViridityDisplayEvents.MouseDoubleClick |
                ViridityDisplayEvents.MouseMoveWhenMousePressed |
                ViridityDisplayEvents.MouseHover |
                ViridityDisplayEvents.MouseWheel |
                ViridityDisplayEvents.Touch |
                ViridityDisplayEvents.ContextMenu,

            targetId: undefined,
            params: [],

            useBlobBuilder: false,

            canvas: 0,
            ctx: 0,

            lastFrame: 0,
            frameEndReceived: true,
            frameErrors: 0,
            pendingPatchesCount: 0,
            waitingForFullUpdate: false,
            frameCommands: [],

            keepAliveInterval: 60000,

            frontCanvas: 0,
            frontCtx: 0,

            resizeCanvas: 0,
            resizeCtx: 0,

            textInterceptor: 0,
            useTextInterceptor: false,
            showInputMethodOnFocus: true,

            ratio: 1,

            _imageDone: function()
            {
                --dr.pendingPatchesCount;
                dr._determineReadyState();
            },

            _determineReadyState: function()
            {
                if (dr.debugVerbosity > 1) console.log(dr.targetId + " -> pendingPatchesCount: " + dr.pendingPatchesCount)
                if (dr.frameEndReceived && dr.pendingPatchesCount === 0)
                {
                    if (dr.frameErrors > 0)
                    {
                        console.log("ERROR LOADING IMAGES!");
                        console.log("Requesting full update...");
                        v.sendMessage("ready()", dr.targetId);
                        dr.requestFullUpdate(true);
                    }
                    else
                    {
                        dr._flipToFront();
                        if (dr.debugDraw)
                            dr._debugDraw();

                        if (dr.debugVerbosity > 1) console.log(dr.targetId + " -> SENDING READY!!!!");

                        v.sendMessage("ready()", dr.targetId);
                    }

                    dr.frameErrors = 0;
                }
            },

            _flipToFront: function()
            {
                // Do we have a pending resize, ie. was the back canvas already resized
                // and we are waiting for the full update? In this case update the size
                // of the front canvas now.
                if (dr.frontCanvas.width !== dr.canvas.width)
                {
                    dr.frontCanvas.width = dr.canvas.width;
                    $(dr.frontCanvas).css("width", (dr.canvas.width / dr.ratio) + "px");
                }

                if (dr.frontCanvas.height !== dr.canvas.height)
                {
                    dr.frontCanvas.height = dr.canvas.height;
                    $(dr.frontCanvas).css("height", (dr.canvas.height / dr.ratio) + "px");
                }

                dr.frontCtx.clearRect(0, 0, dr.canvas.width, dr.canvas.height);
                dr.frontCtx.drawImage(dr.canvas, 0, 0);
            },

            requestFullUpdate: function(forced)
            {
                if (!v.connected)
                    return;

                if (typeof(forced) === "undefined") forced = false;
                dr.waitingForFullUpdate = true;
                v.sendMessage("requestFullUpdate(" + (forced ? "1" : "0") + ")", dr.targetId);
            },

            _sendKeepAlive: function()
            {
                if (v.connected)
                    v.sendMessage("keepAlive()", dr.targetId);
            },

            createDebugOverlay: function()
            {
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

                debugControls.appendTo(containerElement);
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

            _drawDisconnected: function ()
            {
                dr.frontCtx.fillStyle = "rgba(0, 0, 0, 0.3)";
                dr.frontCtx.fillRect(0, 0, dr.frontCanvas.width, dr.frontCanvas.height);

                dr.frontCtx.font = '11pt Helvetica';
                dr.frontCtx.textAlign = 'center';
                dr.frontCtx.fillStyle = 'white';
                dr.frontCtx.fillText("Reconnecting...", dr.frontCanvas.width / 2, dr.frontCanvas.height / 2);
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

                    if (command === "fR")
                    {
                        dr.frontCtx.fillStyle = "rgba(0, 255, 0, " + alpha + ")";
                        dr.frontCtx.fillRect(x1, y1, w, h);
                        dr.frontCtx.strokeRect(x1, y1, w, h);
                    }
                    else if (command === "mI")
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
                    else if (command === "dI")
                    {
                        dr.frontCtx.fillStyle = "rgba(255, 0, 0, " + alpha + ")";
                        dr.frontCtx.fillRect(x1, y1, w, h);
                        dr.frontCtx.strokeRect(x1, y1, w, h);
                    }
                }
            },

            _messageCallback: function(t)
            {
                var inputParams = t.params;
                var frame = inputParams[0];

                if (dr.waitingForFullUpdate && t.command !== "fullUpdate")
                    return;

                if (t.command === "fullUpdate")
                {
                    if (dr.debugVerbosity > 1) console.log(dr.targetId + " -> FULL UPDATE FRAME " + frame);

                    dr.waitingForFullUpdate = false;
                    dr.pendingPatchesCount = 0;
                }

                if (dr.lastFrame !== frame)
                {
                    if (dr.debugVerbosity > 1) console.log(dr.targetId + " -> NEW FRAME: " + dr.lastFrame + " -> " + frame);
                    dr.lastFrame = frame;

                    if (dr.pendingPatchesCount != 0)
                    {
                        console.log(dr.targetId + " -> PREVIOUS FRAME NOT COMPLETELY RENDERED!!!!! Patches left: " + dr.pendingPatchesCount);
                        dr.pendingPatchesCount = 0;
                    }

                    // This is to stop _determineReadyState() from sending ready() when image loading is quasi-synchronous, ie. base64 encoded sources.
                    dr.frameEndReceived = false;

                    if (dr.debugDraw)
                    {
                        dr._flipToFront(); // overwrite debug rects...
                        dr.frameCommands = [];
                    }
                }

                if (dr.debugVerbosity > 1) console.log(dr.targetId + " -> command: " + t.command + " params: " + JSON.stringify(inputParams));
                if (dr.debugDraw)
                {
                    var frameCmd =
                    {
                        command: t.command,
                        params: inputParams
                    }

                    dr.frameCommands.push(frameCmd);
                }

                if (t.command === "fR")
                {
                    var dstX = parseInt(inputParams[1]);
                    var dstY = parseInt(inputParams[2]);
                    var dstWidth = parseInt(inputParams[3]);
                    var dstHeight = parseInt(inputParams[4]);

                    dr.ctx.clearRect(dstX, dstY, dstWidth, dstHeight);

                    dr.ctx.fillStyle = "rgba(" + inputParams[5] + "," + inputParams[6] + "," + inputParams[7] + "," + (inputParams[8] / 255) + ")";
                    dr.ctx.fillRect(dstX, dstY, dstWidth, dstHeight);
                }
                else if (t.command === "mI")
                {
                    var srcX = parseInt(inputParams[1]);
                    var srcY = parseInt(inputParams[2]);
                    var srcWidth = parseInt(inputParams[3]);
                    var srcHeight = parseInt(inputParams[4]);

                    var dstX = parseInt(inputParams[5]);
                    var dstY = parseInt(inputParams[6]);

                    dr.ctx.clearRect(dstX, dstY, srcWidth, srcHeight);
                    dr.ctx.drawImage(dr.frontCanvas, srcX, srcY, srcWidth, srcHeight, dstX, dstY, srcWidth, srcHeight);
                }
                else if (t.command === "dI")
                {
                    ++dr.pendingPatchesCount;

                    var contentType = inputParams[6];

                    var isPackedAlpha = false;
                    if (contentType.indexOf(";pa") > -1)
                    {
                        isPackedAlpha = true;
                        contentType = contentType.replace(";pa", "");
                    }

                    var margin = parseInt(inputParams[5]);

                    var img = new Image;
                    img.onerror = function()
                    {
                        ++dr.frameErrors;
                        dr._imageDone();
                    };

                    img.onload = function()
                    {
                        if (frame !== dr.lastFrame)
                            console.log(dr.targetId + " -> ASYNCHRONOUS IMAGE!!!!! " + " frame is " + frame + ", but dr.lastFrame is " + dr.lastFrame);

                        if (dr.debugVerbosity > 1) console.log(dr.targetId + " -> frame: " + frame + " img.src: " + img.src);

                        var dstX = parseInt(inputParams[1]);
                        var dstY = parseInt(inputParams[2]);

                        var imgWidth = parseInt(inputParams[3]);
                        var imgHeight = parseInt(inputParams[4]);

                        var dstWidth = imgWidth;
                        var dstHeight = imgHeight;

                        dr.ctx.clearRect(dstX, dstY, dstWidth, dstHeight);

                        if (isPackedAlpha)
                        {
                            var alphaCanvas = document.createElement("canvas");
                            alphaCanvas.width = imgWidth;
                            alphaCanvas.height = imgHeight;

                            alphaCanvasCtx = alphaCanvas.getContext("2d");
                            alphaCanvasCtx.drawImage(img,
                                margin, imgHeight + margin * 3, imgWidth, imgHeight,
                                0, 0, imgWidth, imgHeight
                            );

                            var idata = alphaCanvasCtx.getImageData(0, 0, imgWidth, imgHeight);
                            var data = idata.data;
                            var i = data.length - 1;

                            for (; i > 0; i -= 4)
                                data[i] = 255 - data[i - 3];

                            alphaCanvasCtx.putImageData(idata, 0, 0);

                            dr.ctx.save();

                            dr.ctx.drawImage(img,
                                margin, margin, imgWidth, imgHeight,
                                dstX, dstY, dstWidth, dstHeight
                            );

                            dr.ctx.globalCompositeOperation = "xor";

                            dr.ctx.drawImage(alphaCanvas,
                                0, 0, imgWidth, imgHeight,
                                dstX, dstY, dstWidth, dstHeight
                            );

                            dr.ctx.restore();
                        }
                        else
                            dr.ctx.drawImage(img,
                                margin, margin, imgWidth, imgHeight,
                                dstX, dstY, dstWidth, dstHeight
                            );

                        if (dr.useBlobBuilder)
                            URL.revokeObjectURL(img.src);

                        dr._imageDone();
                    };

                    if (t.dataAsString(0, 3) === "fb:")
                    {
                        img.crossOrigin = "Anonymous";
                        img.src = v.fullLocation + "/" + v.sessionId + "/p/" + t.dataAsString(3);
                    }
                    else if (t.dataAsString(0, 4) === "http")
                    {
                        img.crossOrigin = "Anonymous";
                        img.src = t.dataAsString();
                    }
                    else if (dr.useBlobBuilder)
                    {
                        var blobber = new BlobBuilder;

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

                        var buf = Base64Binary.decodeArrayBuffer(t.dataAsString());

                        /* SLOW!
                        var rawData = atob(imageData);

                        var buf = new ArrayBuffer(rawData.length);
                        var view = new Uint8Array(buf);
                        for (var i = 0; i < view.length; i++)
                            view[i] = rawData.charCodeAt(i);
                        */

                        blobber.append(buf);

                        contentType = contentType.split(";")[0]; // "image/jpeg" etc. remove base64

                        var blob = blobber.getBlob(contentType);
                        var blobUrl = URL.createObjectURL(blob);
                        img.src = blobUrl;
                    }
                    else
                    {
                        if (t.dataIsBinary && contentType.indexOf("base64") === -1)
                        {
                            var bytes = t.data();
                            var blob = new Blob([bytes], {type: contentType});

                            var reader = new FileReader();
                            reader.onload = function(e)
                            {
                                img.src = e.target.result;
                            };
                            reader.readAsDataURL(blob);
                        }
                        else
                        {
                            img.src = "data:" + contentType + "," + t.dataAsString();
                        }
                    }
                }
                else if (t.command === "info")
                {
                    dr.connectionId = inputParams[0];
                    dr.frameEndReceived = true;
                }
                else if (t.command === "end")
                {
                    if (dr.debugVerbosity > 1) console.log(dr.targetId + " -> Frame end " + frame + " received...");
                    dr.frameEndReceived = true;
                    dr._determineReadyState();
                }
            },

            resize: function(width, height, force)
            {
                var scaledWidth = Math.round(width * dr.ratio);
                var scaledHeight = Math.round(height * dr.ratio);

                if (dr.canvas.width != scaledWidth || dr.canvas.height != scaledHeight || force)
                {
                    if (dr.debugVerbosity > 0)
                        console.log(dr.targetId + " -> width: " + width + " height: " + height);

                    dr._resizeCanvas(dr.canvas, dr.ctx, scaledWidth, scaledHeight);

                    // Properly clip the canvas so it does not move outside of its container...
                    $(dr.frontCanvas).css("clip", "rect(0px," + width + "px," + height + "px,0px)");

                    if (dr.debugVerbosity > 1) console.log(dr.targetId + " -> RESIZING TO " + scaledWidth + " x " + scaledHeight);

                    if (v.connected)
                        v.sendMessage("resize(" + scaledWidth + "," + scaledHeight + "," + dr.ratio + ")", dr.targetId);
                }
            },

            _resizeCanvas: function(canvas, ctx, width, height)
            {
                // Resize canvas and keep content, filling new space with 0x0.
                dr.resizeCanvas.width = width;
                dr.resizeCanvas.height = height;

                dr.resizeCtx.clearRect(0, 0, width, height);
                dr.resizeCtx.drawImage(canvas, 0, 0);

                canvas.width = width;
                canvas.height = height;

                ctx.clearRect(0, 0, width, height);
                ctx.drawImage(dr.resizeCanvas, 0, 0);
            },

            updateSize: function(force)
            {
                dr.resize(containerElement.width(), containerElement.height(), force);
            },

            _requestNewDisplay: function()
            {
                if (v.connected)
                {
                    var joinedParams = typeof(dr.params) === "array" ? dr.params.join() : dr.params;
                    v.sendMessage("newDisplay(" + joinedParams + ")", dr.targetId);
                }
            },

            _reconnectDisplay: function()
            {
                dr._requestNewDisplay();
                dr.updateSize(true);
            },

            focus: function()
            {
                $(document).focus();
                $(dr.frontCanvas).focus();

                if (dr.showInputMethodOnFocus)
                    dr.showInputMethod();
            },

            showInputMethod: function()
            {
                if (dr.textInterceptor)
                {
                    dr.textInterceptor.style.visibility = "visible";
                    dr.textInterceptor.focus();
                    //dr.textInterceptor.style.visibility = "hidden"; // Chrome obviously does not like hiding the textarea...
                }
            },

            hideInputMethod: function()
            {
                $(document).focus();
                $(dr.frontCanvas).focus();
            },

            init: function()
            {
                dr.targetId = v.registerCallback(dr._messageCallback, targetId);
                dr.params = params;

                if (typeof(options) == "object")
                {
                    if (typeof(options.useTextInterceptor) == "boolean")
                        dr.useTextInterceptor = options.useTextInterceptor;

                    if (typeof(options.showInputMethodOnFocus) == "boolean")
                        dr.showInputMethodOnFocus = options.showInputMethodOnFocus;
                }

                dr._requestNewDisplay();

                dr.canvas = document.createElement("canvas");
                dr.ctx = dr.canvas.getContext("2d");

                dr.frontCanvas = document.createElement("canvas");
                dr.frontCtx = dr.frontCanvas.getContext("2d");

                dr.resizeCanvas = document.createElement("canvas");
                dr.resizeCtx = dr.resizeCanvas.getContext("2d");

                var devicePixelRatio = window.devicePixelRatio || 1;
                var backingStoreRatio = dr.frontCtx.webkitBackingStorePixelRatio ||
                                        dr.frontCtx.mozBackingStorePixelRatio ||
                                        dr.frontCtx.msBackingStorePixelRatio ||
                                        dr.frontCtx.oBackingStorePixelRatio ||
                                        dr.frontCtx.backingStorePixelRatio || 1;

                dr.ratio = devicePixelRatio / backingStoreRatio;

                if (dr.debugVerbosity > 0)
                    console.log("devicePixelRatio: " + devicePixelRatio + " backingStoreRatio: " + backingStoreRatio + " dr.ratio: " + dr.ratio);

                containerElement.css({
                    "margin": 0,
                    "padding": 0
                });

                $(dr.frontCanvas)
                    .addClass("viridity")
                    .css({
                        "margin": 0,
                        "padding": 0,
                        "position": "absolute",
                        "outline": "none"
                    })
                    .attr("tabindex", 1); // Make canvas focusable!

                containerElement.append(dr.frontCanvas);

                // Set up default sizes...
                if (containerElement.height() === 0)
                    containerElement.height(300);

                if (containerElement.width() === 0)
                    containerElement.width(300);

                // Set up timed resize callback to resize canvas, ie. when the style size was changed
                // exogenously or indirectly.
                // Note: jQuery plugins that are using MutationObserver or other means do not work reliable.
                setInterval(dr.updateSize, 2000);

                // Set resize callback to allow triggering of a resize from jQuery...
                containerElement.resize(dr.updateSize);

                $(window).resize(function() { setTimeout(dr.updateSize, 50); });

                // Finally set size...
                dr.updateSize();

                //dr.createDebugOverlay();

                var BlobBuilder = window.BlobBuilder || window.WebKitBlobBuilder || window.MozBlobBuilder || window.MSBlobBuilder;
                var URL = window.URL || window.webKitURL;

                //dr.useBlobBuilder = BlobBuilder && URL;
                dr.useBlobBuilder = false;

                function getCanvasPos(event)
                {
                    var offset = $(dr.frontCanvas).offset();
                    var posX = (event.pageX - offset.left) * dr.ratio;
                    var posY = (event.pageY - offset.top) * dr.ratio;
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
                    if (!v.connected)
                        return;

                    var pos = getCanvasPos(event);

                    // Round coordinates, because IE is sending floats...
                    pos.x = Math.round(pos.x);
                    pos.y = Math.round(pos.y);

                    if (other)
                        v.sendMessage(type + "(" + pos.x + "," + pos.y + "," + event.which + "," + getModifiers(event) + "," + other + ")", dr.targetId);
                    else
                        v.sendMessage(type + "(" + pos.x + "," + pos.y + "," + event.which + "," + getModifiers(event) + ")", dr.targetId);

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
                    if (!v.connected || (dr.enabledEvents & ViridityDisplayEvents.KeyboardInputs) == 0)
                        return;

                    if (dr.debugVerbosity > 0)
                    {
                        console.log("Type: " + type + " event.which: " + event.which + " event.keyCode: " + event.keyCode + " event.charCode: " + event.charCode + " printableCharacter: " + printableCharacter);
                        //console.dir(event);
                    }

                    if (event.hasOwnProperty("which"))
                        v.sendMessage(type + "(" + event.which + "," + getModifiers(event) + ")", dr.targetId);
                    else
                        v.sendMessage(type + "(" + event.keyCode + "," + getModifiers(event) + ")", dr.targetId);

                    if (printableCharacter || isNonPrintableKey(event.keyCode))
                    {
                        event.stopPropagation();
                        event.preventDefault();
                    }
                }               

                $(dr.frontCanvas).mousedown(function(event)  { if (dr.enabledEvents & ViridityDisplayEvents.MousePresses) { dr.focus(); sendMouseEvent("mouseDown", event); }});
                $(dr.frontCanvas).mouseup(function(event)    { if (dr.enabledEvents & ViridityDisplayEvents.MousePresses) { sendMouseEvent("mouseUp", event); }});
                $(dr.frontCanvas).mousemove(function(event)  { if ((dr.enabledEvents & ViridityDisplayEvents.MouseHover) ||
                                                                  ((dr.enabledEvents & ViridityDisplayEvents.MouseMoveWhenMousePressed) && event.buttons)) { sendMouseEvent("mouseMove", event); }});
                $(dr.frontCanvas).mouseover(function(event)  { if (dr.enabledEvents & ViridityDisplayEvents.MouseHover) { sendMouseEvent("mouseEnter", event); }});
                $(dr.frontCanvas).mouseout(function(event)   { if (dr.enabledEvents & ViridityDisplayEvents.MouseHover) { sendMouseEvent("mouseExit", event); }});
                $(dr.frontCanvas).mouseenter(function(event) { if (dr.enabledEvents & ViridityDisplayEvents.MouseHover) { sendMouseEvent("mouseEnter", event); }});
                $(dr.frontCanvas).dblclick(function(event)   { if (dr.enabledEvents & ViridityDisplayEvents.MouseDoubleClick) { sendMouseEvent("mouseDblClick", event); }});
                $(dr.frontCanvas).bind("contextmenu", function (event) { if (dr.enabledEvents & ViridityDisplayEvents.ContextMenu) { sendMouseEvent("contextMenu", event); }});

                function handleTouchEvent(event)
                {
                    if ((dr.enabledEvents & ViridityDisplayEvents.Touch) == 0)
                        return;

                    event = event.originalEvent;

                    var touchPoints = event.changedTouches;
                    var first = touchPoints[0];

                    var type;
                    switch(event.type)
                    {
                        case "touchstart": type = "mouseDown"; break;
                        case "touchmove":  type = "mouseMove"; break;
                        case "touchend":   type = "mouseUp";   break;
                        default:           type = "mouseUp";   break;
                    }

                    var simulatedEvent = document.createEvent("MouseEvent");
                    simulatedEvent.initMouseEvent(type, true, true, window, 1,
                                                  first.screenX, first.screenY,
                                                  first.clientX, first.clientY, false,
                                                  false, false, false, 0/*left*/, null);

                    if (event.type == "touchstart")
                        sendMouseEvent("mouseMove", simulatedEvent);

                    dr.focus();
                    sendMouseEvent(type, simulatedEvent);

                    event.preventDefault();
                }

                $(dr.frontCanvas).on("touchstart", handleTouchEvent);
                $(dr.frontCanvas).on("touchmove", handleTouchEvent);
                $(dr.frontCanvas).on("touchend", handleTouchEvent);
                $(dr.frontCanvas).on("touchcancel", handleTouchEvent);

                var setUpWheelSupport = function()
                {
                    $(dr.frontCanvas).mousewheel(function(event, delta, deltaX, deltaY)
                    {
                        if (dr.enabledEvents & ViridityDisplayEvents.MouseWheel)
                            sendMouseEvent("mouseWheel", event, deltaX + "," + deltaY);
                    });
                }

                if ($.fn.mousewheel == undefined)
                    $.getScript("jquery.mousewheel.js", setUpWheelSupport);
                else
                    setUpWheelSupport();

                $(dr.frontCanvas).keydown(function(event)    { sendKeyEvent("keyDown", event, false); });
                $(dr.frontCanvas).keypress(function(event)   { sendKeyEvent("keyPress", event, true); });
                $(dr.frontCanvas).keyup(function(event)      { sendKeyEvent("keyUp", event, false); });

                var isiOS = /iPad|iPhone|iPod/.test(navigator.userAgent) && !window.MSStream;
                var isAndroid = /Android/.test(navigator.userAgent) && !window.MSStream;

                if (isiOS || isAndroid || dr.useTextInterceptor)
                {
                    // Why does this code exist?
                    // Key events on mobile browsers are a mess. A real stinking mess.
                    // In order to offer rudimentary support for text input on mobile browsers
                    // we inject a textarea and use it to intercept typed text
                    // and by differential analysis we send new single-char or multi-char
                    // text changes to the server. This code does its best to prevent multiple
                    // prefix insertions caused by autocompletion et al.

                    function createTextInterceptor()
                    {
                        var textInterceptor = document.createElement("textarea");
                        $(textInterceptor)
                            .addClass("viridity")
                            .css({
                                "min-width": 0,
                                "min-height": 0,
                                "margin-top": 100,
                                "margin-left": -4000,
                                "width": 20,
                                "height": 20,
                                "z-index": -1000,
                                "opacity": "0.001",
                                "position": "fixed",
                                "visibility": "hidden"
                            })

                        containerElement.append(textInterceptor);
                        return textInterceptor;
                    }

                    var changingText = false;
                    var sentinelTextForBackspace = "- ";
                    var lastText = "";
                    var lastTextRaw = sentinelTextForBackspace;

                    function moveCursorToEnd(textInterceptor)
                    {
                        if ((dr.enabledEvents & ViridityDisplayEvents.KeyboardInputs) == 0)
                            return;

                        if (textInterceptor.setSelectionRange)
                        {
                            var len = sentinelTextForBackspace.length * 2;
                            textInterceptor.setSelectionRange(len, len);
                        }
                    }

                    function handleTextInterceptorChange()
                    {
                        if (changingText | (dr.enabledEvents & ViridityDisplayEvents.KeyboardInputs) == 0)
                            return;

                        changingText = true;
                        try
                        {
                            var newTextRaw = dr.textInterceptor.value;

                            if (dr.debugVerbosity > 0)
                                console.log("newTextRaw: >" + newTextRaw + "<  lastTextRaw: >" + lastTextRaw + "<");

                            if (newTextRaw == lastTextRaw)
                            {
                                if (dr.debugVerbosity > 0)
                                    console.log("No change in text.");

                                return;
                            }

                            var maxCompareLength = Math.min(lastTextRaw.length, newTextRaw.length);
                            var firstChangeIndex = maxCompareLength;

                            for (var i = 0; i < maxCompareLength; ++i)
                            {
                                if (lastTextRaw[i] != newTextRaw[i])
                                {
                                    firstChangeIndex = i;
                                    break;
                                }
                            }

                            var backspacesRequired = lastTextRaw.length - firstChangeIndex;
                            var appendsRequired = newTextRaw.length - firstChangeIndex;

                            if (dr.debugVerbosity > 0)
                                console.log("backspacesRequired: " + backspacesRequired + " appendsRequired: " + appendsRequired);

                            if (backspacesRequired > 0)
                            {
                                if (dr.debugVerbosity > 0)
                                    console.log("Text length reduced by " + backspacesRequired + ", sending backspace key events.");

                                var event = {
                                    which: 8, // backspace key
                                    shiftKey: false,
                                    ctrlKey: false,
                                    altKey: false,
                                    metaKey: false
                                };

                                for (var i = 0; i < backspacesRequired; ++i)
                                {
                                    sendKeyEvent("keyDown", event, false);
                                    sendKeyEvent("keyPress", event, false);
                                    sendKeyEvent("keyUp", event, false);
                                }
                            }

                            if (appendsRequired)
                            {
                                var newText = newTextRaw.substr(firstChangeIndex);

                                if (dr.debugVerbosity > 0)
                                    console.log("Text change: >" + newText + "<");

                                // as per https://ecmanaut.blogspot.de/2006/07/encoding-decoding-utf8-in-javascript.html
                                function encodeUtf8(s)
                                {
                                    return unescape(encodeURIComponent(s));
                                }

                                var newTextBase64 = btoa(encodeUtf8(newText));
                                v.sendMessage("text(" + newTextBase64 + ")", dr.targetId);
                            }

                            var prefixRaw = lastTextRaw.substr(0, newTextRaw.length);

                            if (newTextRaw.length < sentinelTextForBackspace.length)
                            {
                                $(dr.textInterceptor).val(sentinelTextForBackspace);
                                lastTextRaw = sentinelTextForBackspace;
                                lastText = "";
                            }
                            else
                            {
                                lastTextRaw = newTextRaw;
                                lastText = newText;
                            }
                        }
                        finally
                        {
                            changingText = false;
                        }
                    }

                    function initializeTextInterceptor(textInterceptor)
                    {
                        textInterceptor.value = sentinelTextForBackspace;
                        moveCursorToEnd(textInterceptor);
                        $(textInterceptor).bind('input propertychange', handleTextInterceptorChange);
                    }

                    dr.textInterceptor = createTextInterceptor();
                    initializeTextInterceptor(dr.textInterceptor);
                }

                // Finally set up keep alive...
                setInterval(dr._sendKeepAlive, dr.keepAliveInterval);

                v.on("sessionDisconnected", dr._drawDisconnected);
                v.on("sessionStart", dr._reconnectDisplay);
                v.on("sessionReattached", dr._reconnectDisplay);
            }
        }

        if (containerElement.find(".viridity").length > 0)
        {
            console.log("Other viridity display already instantiated for " + targetId + ". Returning null. Your code will fail.");
            dr = null;
        }
        else
            dr.init();

        return dr;
    };
})(jQuery);
