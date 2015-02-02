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

            if (t.targetId == c.targetId &&
                (t.command === "data" || t.command === "dataResponse"))
            {
                var paramStartIndex = t.params.indexOf(",");

                var responseId = t.params.substring(0, paramStartIndex);
                var input = t.params.substring(paramStartIndex + 1);

                if (t.command === "dataResponse")
                {
                    if (c.pendingResponseCallbacks.hasOwnProperty(responseId))
                    {
                        c.pendingResponseCallbacks[responseId](JSON.parse(input));
                        delete c.pendingResponseCallbacks[responseId];
                        processed = true;
                    }
                }
                else if (typeof(c.onNewCommandReceived) == "function")
                {
                    var result = c.onNewCommandReceived(JSON.parse(input));
                    v.sendMessage("dataResponse(" + responseId + "," + JSON.stringify(result) + ")", c.targetId);
                    processed = true;
                }
            }

            return processed;
        },

        sendCommand: function(command, callback)
        {
            ++c.responseId;
            c.pendingResponseCallbacks[c.responseId] = callback;
            var message = "data(" + c.responseId + "," + JSON.stringify(command) + ")";
            v.sendMessage(message, c.targetId);
        }
    }

    c.targetId = v.registerCallback(c._messageCallback, id);

    return c;
}
