var pendingCommandCallbacks = {};

function sendCommand(command, callback, displayId)
{
    if (typeof(displayId) == "undefined")
        displayId = "";

    var responseId = CommandBridge.sendCommand(JSON.stringify(command), displayId);
    pendingCommandCallbacks[responseId] = callback;
}

CommandBridge.responseReceived.connect(function(responseId, response, displayId)
{
    if (pendingCommandCallbacks.hasOwnProperty(responseId))
    {
        pendingCommandCallbacks[responseId](JSON.parse(response), displayId);
        delete pendingCommandCallbacks[responseId];
    }
});

var onNewCommandReceived = undefined;

CommandBridge.commandReceived.connect(function(responseId, input)
{
    if (typeof(onNewCommandReceived) == "function")
    {
        CommandBridge.response = JSON.stringify(onNewCommandReceived(JSON.parse(input)));
    }
});
