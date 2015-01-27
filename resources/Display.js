var pendingCommands = {};

function sendCommand(command, callback, displayId)
{
    if (typeof(displayId) == "undefined")
        displayId = "";

    var responseId = CommandBridge.sendCommand(JSON.stringify(command), displayId);
    pendingCommands[responseId] = callback;
}

CommandBridge.responseReceived.connect(function(responseId, response, displayId)
{
    if (pendingCommands.hasOwnProperty(responseId))
    {
        pendingCommands[responseId](JSON.parse(response), displayId);
        delete pendingCommands[responseId];
    }
});
