function ViriditySelect(element, remote, itemName)
{
    this.actions =
    {
        setSelected: function(params)
        {
            element.children("option").prop('selected', false);
            $(element.children("option")[params.index]).prop('selected', true);
        },
    }

    element.change(function()
    {
         var index = element.children("option:selected").index();
         remote.call("changed", { index: index });
    });
}

DR.registerComponent("vdr-select", ViriditySelect, true);
