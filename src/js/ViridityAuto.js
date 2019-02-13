var ViridityChannel = null;

var ViridityAuto = {

    debugVerbosity: 0,
    alreadyAttached: false,

    options: {
        connectionMethod: ConnectionMethod.Auto,
        useBinaryProtocol: true,
        debugVerbosity: 0
    },

    callbacks: {
        autoAttach: []
    },

    on: function(eventName, callback)
    {
        if (eventName in ViridityAuto.callbacks)
            ViridityAuto.callbacks[eventName].push(callback);

        if (ViridityAuto.alreadyAttached && eventName === "autoAttach")
            callback();
    },

    off: function(eventName, callback)
    {
        if (eventName in ViridityAuto.callbacks)
        {
            var index = ViridityAuto.callbacks[eventName].indexOf(callback);
            if (index > -1)
                ViridityAuto.callbacks[eventName].splice(index, 1);
        }
    },

    _triggerCallback: function(eventName, params)
    {
        if (ViridityAuto.debugVerbosity > 1)
            console.log("Now triggering event: " + eventName + " params: " + JSON.stringify(params));

        var callbacks = ViridityAuto.callbacks[eventName];

        for (var i = 0; i < callbacks.length; ++i)
        {
            try
            {
                callbacks[i](params);
            }
            catch (e)
            {
                console.log("Viridity: Error occurred while executing callback " + callbacks[i] + " for event " + eventName + ": " + e);
            }
        }
    },

    autoAttach: function()
    {
        if (!ViridityAuto.alreadyAttached)
        {
        var useCookies = typeof($.cookie) != "undefined";

        if (useCookies)
            options.sessionId = $.cookie("vsessionId");

        ViridityChannel = new Viridity(ViridityAuto.options);

        if (useCookies)
        {
            ViridityChannel.on("sessionStart", function(sessionId)
            {
                $.cookie("vsessionId", sessionId); // Save session id in cookie so we can re-attach again...
            });

            ViridityChannel.on("sessionReattached", function(sessionId)
            {
                $.cookie("vsessionId", sessionId); // Save session id to cookie in case it changed...
            });
        }

            ViridityAuto.alreadyAttached = true;
        }

        ViridityAuto._triggerCallback("autoAttach");
    }
};

(function($)
{
    $(document).ready(ViridityAuto.autoAttach);
})(jQuery);
