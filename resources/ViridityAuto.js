var ViridityChannel = null;

(function($)
{
    $(document).ready(function($) {
        ViridityChannel = new Viridity({ connectionMethod: ConnectionMethod.Auto, sessionId: $.cookie("vsessionId") });

        ViridityChannel.on("sessionStart", function(sessionId)
        {
            $.cookie("vsessionId", sessionId); // Save session id in cookie so we can re-attach again...
        });

        ViridityChannel.on("sessionReattached", function(sessionId)
        {
            $.cookie("vsessionId", sessionId); // Save session id to cookie in case it changed...
        });

        if (typeof(DocumentRenderer) != "undefined")
        {
            DocumentRendererGlobal.autoAttach();
        }
    });
})(jQuery);
