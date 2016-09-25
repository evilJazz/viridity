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

var DataBridge = function(viridityChannel, id)
{
    var v = viridityChannel;

    var c =
    {
        targetId: undefined,

        onDataReceived: undefined,

        responseId: 0,
        pendingResponseCallbacks: {},

        _messageCallback: function(t)
        {
            var processed = false;

            if (t.targetId === c.targetId &&
                (t.command === "data" || t.command === "dataResponse"))
            {
                var responseId = t.params[0];
                var input = t.dataAsString();

                if (t.command === "dataResponse")
                {
                    if (c.pendingResponseCallbacks.hasOwnProperty(responseId))
                    {
                        if (typeof(c.pendingResponseCallbacks[responseId]) == "function")
                            c.pendingResponseCallbacks[responseId](JSON.parse(input));

                        delete c.pendingResponseCallbacks[responseId];
                        processed = true;
                    }
                }
                else if (typeof(c.onDataReceived) == "function")
                {
                    var result = c.onDataReceived(JSON.parse(input));
                    v.sendMessage("dataResponse(" + responseId + "):" + JSON.stringify(result), c.targetId);
                    processed = true;
                }
            }

            return processed;
        },

        sendData: function(data, callback)
        {
            ++c.responseId;

            if (typeof(callback) == "function")
                c.pendingResponseCallbacks[c.responseId] = callback;

            var message = "data(" + c.responseId + "):" + JSON.stringify(data);
            v.sendMessage(message, c.targetId);
        }
    }

    c.targetId = v.registerCallback(c._messageCallback, id);

    return c;
}
