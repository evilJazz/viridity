function ViridityTextArea(element, remote, itemName)
{
    this.actions =
    {
        setText: function(params)
        {
            element.val(params.text);
        },

        setPlaceholder: function(params)
        {
            element.attr("placeholder", params.placeholder);
        }
    }

    element.change(function()
    {
        remote.call("changed", { text: element.val() });
    });
}

DR.registerComponent("vdr-textarea", ViridityTextArea, true);
