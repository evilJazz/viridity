function ViridityInput(element, remote, itemName)
{
    var isCheckBox = element.attr("type") == "checkbox";

    this.actions =
    {
        setValue: function(params)
        {
            if (isCheckBox)
                element.prop('checked', params.value);
            else
                element.val(params.value);
        },

        setPlaceholder: function(params)
        {
            element.attr("placeholder", params.placeholder);
        }
    }

    element.change(function()
    {
        if (isCheckBox)
            remote.call("changed", { value: element.prop('checked') });
        else
            remote.call("changed", { value: element.val() });
    });

    element.on('input', function()
    {
        remote.call("changed", { value: element.val() });
    });
}

DR.registerComponent("vdr-input", ViridityInput, true);
