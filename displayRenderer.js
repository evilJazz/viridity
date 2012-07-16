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
    var dr =
    {
        socket: 0,
        canvas: 0,
        ctx: 0,

        requestFullUpdate: function()
        {
            dr.socket.send("requestFullUpdate()");
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

        init: function()
        {
            dr.socket = new WebSocket('ws://192.168.1.1:8080/display');
            dr.canvas = document.getElementById('canvas');
            dr.ctx = dr.canvas.getContext("2d");

            var BlobBuilder = window.BlobBuilder || window.WebKitBlobBuilder || window.MozBlobBuilder || window.MSBlobBuilder;
            var URL = window.URL || window.webKitURL;
            //var useBlobBuilder = BlobBuilder && URL;
            var useBlobBuilder = false;

            if (dr.canvas.width != 1024 || dr.canvas.height != 768)
            {
                dr.canvas.width = 1024;
                dr.canvas.height = 768;
            }

            function getCanvasPos(event)
            {
                var offset = $(dr.canvas).offset();
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

                if (other)
                    dr.socket.send(type + "(" + pos.x + "," + pos.y + "," + event.which + "," + getModifiers(event) + "," + other + ")");
                else
                    dr.socket.send(type + "(" + pos.x + "," + pos.y + "," + event.which + "," + getModifiers(event) + ")");

                event.stopPropagation();
                event.preventDefault();
            }

            function sendKeyEvent(type, event)
            {
                if (event.hasOwnProperty("which"))
                    dr.socket.send(type + "(" + event.which + "," + getModifiers(event) + ")");
                else
                    dr.socket.send(type + "(" + pos.x + "," + pos.y + ")");

                event.stopPropagation();
                event.preventDefault();
            }

            $(dr.canvas).mousedown(function(event)  { sendMouseEvent("mouseDown", event) });
            $(dr.canvas).mouseup(function(event)    { sendMouseEvent("mouseUp", event) });
            $(dr.canvas).mousemove(function(event)  { sendMouseEvent("mouseMove", event) });
            $(dr.canvas).mouseover(function(event)  { sendMouseEvent("mouseEnter", event) });
            $(dr.canvas).mouseout(function(event)   { sendMouseEvent("mouseExit", event) });
            $(dr.canvas).mouseenter(function(event) { sendMouseEvent("mouseEnter", event) });
            $(dr.canvas).dblclick(function(event)   { sendMouseEvent("mouseDblClick", event) });

            $(dr.canvas).mousewheel(function(event, delta, deltaX, deltaY) { sendMouseEvent("mouseWheel", event, deltaX + "," + deltaY); });

            $(document).keydown(function(event)    { sendKeyEvent("keyDown", event) });
            $(document).keypress(function(event)   { sendKeyEvent("keyPress", event) });
            $(document).keyup(function(event)      { sendKeyEvent("keyUp", event) });

            dr.socket.onmessage = function(msg)
                    {
                        var paramStartIndex = msg.data.indexOf("(");
                        var paramEndIndex = msg.data.indexOf(")");

                        var command = msg.data.substring(0, paramStartIndex);
                        var params = msg.data.substring(paramStartIndex + 1, paramEndIndex);
                        //console.log("command: " + command);
                        //console.log("params: " + params);

                        var inputParams = params.split(/[\s,]+/);

                        //console.log("split params: " + JSON.stringify(inputParams));

                        var frame = inputParams[0];

                        if (command === "drawImage")
                        {
                            var img = new Image;
                            img.onload = function()
                                    {
                                        dr.ctx.clearRect(inputParams[1], inputParams[2], inputParams[3], inputParams[4])
                                        dr.ctx.drawImage(img, inputParams[1], inputParams[2]);

                                        console.log("frame: " + frame + " img.src: " + img.src);

                                        if (useBlobBuilder)
                                            URL.revokeObjectURL(img.src);
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
                        }
                    };
        }
    }

    return dr;
}

var displayRenderer = new DisplayRenderer();
