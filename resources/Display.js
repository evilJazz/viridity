var pendingCommands = {};

function sendCommand(command, callback, displayId)
{
    if (typeof(displayId) == "undefined")
        displayId = "";

    var responseId = CommandBridge.sendCommand(command, displayId);
    pendingCommands[responseId] = callback;
}

CommandBridge.responseReceived.connect(function(responseId, response, displayId)
{
    if (pendingCommands.hasOwnProperty(responseId))
    {
        pendingCommands[responseId](response, displayId);
        delete pendingCommands[responseId];
    }
});
