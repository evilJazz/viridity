var callbacks = {};

function add(actionName, callback)
{
    callbacks[actionName] = callback;
}

function trigger(actionName, params)
{
    callbacks[actionName](params);
}
