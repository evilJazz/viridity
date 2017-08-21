var actions = [];

function reset()
{
    actions = [];
}

function itemPrepended(item)
{
    actions.push({ action: "prepend", item: item });
}

function itemAppended(item)
{
    actions.push({ action: "append", item: item });
}

function itemInsertedAfter(item, afterItem)
{
    if (item.contentDirty) item.updateContent();
    actions.push({ action: "insertAfter", content: item.replaceMarkerForContent(), afterItemName: afterItem.name });
}

function itemRemoved(item)
{
    actions.push({ itemName: item.name, action: "remove" });
}
