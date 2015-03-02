var DataBridge = function(viridityChannel, id)
{
    var v = viridityChannel;

    var c =
    {
        targetId: undefined,

        onNewCommandReceived: undefined,

        responseId: 0,
        pendingResponseCallbacks: {},

        _messageCallback: function(t)
        {
            var processed = false;

            if (t.targetId === c.targetId &&
                (t.command === "data" || t.command === "dataResponse"))
            {
                var paramStartIndex = t.params.indexOf(",");

                var responseId = t.params.substring(0, paramStartIndex);
                var input = t.params.substring(paramStartIndex + 1);

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
                else if (typeof(c.onNewDataReceived) == "function")
                {
                    var result = c.onNewDataReceived(JSON.parse(input));
                    v.sendMessage("dataResponse(" + responseId + "," + JSON.stringify(result) + ")", c.targetId);
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

            var message = "data(" + c.responseId + "," + JSON.stringify(data) + ")";
            v.sendMessage(message, c.targetId);
        }
    }

    c.targetId = v.registerCallback(c._messageCallback, id);

    return c;
}
