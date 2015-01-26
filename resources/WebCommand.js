var openCommands = {};

function sendCommand(command, callback)
{
    var responseId = CommandBridge.sendCommand(command);
    openCommands[responseId] = callback;
}

CommandBridge.responseReceived.connect(function(responseId, response)
{
    if (openCommands.hasOwnProperty(responseId))
    {
        openCommands[responseId](response);
        delete openCommands[responseId];
    }
});
