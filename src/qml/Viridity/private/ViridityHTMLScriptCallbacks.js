var callbacks = {};

function add(actionName, callback)
{
    callbacks[actionName] = callback;
}

function contains(actionName)
{
    return actionName in callbacks;
}

function trigger(actionName, params)
{
    return callbacks[actionName](params);
}
