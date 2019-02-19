/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "qrc:/KCL/DeferredExecution.js" as DeferredExecution
import "private/ViridityHTMLRepeater.js" as Private

ViridityHTMLColumn {
    id: columnRenderer
    objectName: "repeater"

    childPrefix: createUniqueID()

    default property Component delegate

    //property QtObject repeater: repeaterContainer.repeater //@QtQuick1: Required to see and access the repeater

    property alias model: repeater.model
    property alias count: repeater.count

    property int generation: 0

    property variant _ViridityHTMLRepeater_ignoredPropertyNames: _ViridityHTMLColumn_ignoredPropertyNames.concat(["model", "count", "generation", "debug", "itemRepeater", "repeaterContainer"])
    ignoredPropertyNames: _ViridityHTMLRepeater_ignoredPropertyNames

    property bool debug: false

    function updateChildren()
    {
        try
        {
            if (debug) Debug.print("BEGINNING UPDATE");
            beginUpdate();
            if (debug)
            {
                Debug.print("BEGIN UPDATE");
                Debug.print("UPDATE ITEMS -> NEW COUNT: " + count);
                topLevelRenderer.dumpRendererStructure();
            }

            var newChildren = [];

            var child;

            // Add external sub renderers
            for (var i = 0; i < subRenderers.length; ++i)
            {
                child = subRenderers[i];
                newChildren.push(subRenderers[i]);
            }

            // The order of how child items are added changed between Qt Quick 1 and 2, so account for that...
            //for (var i = 0; i < repeaterContainer.children.length; ++i) //@QtQuick1
            for (var i = repeaterContainer.children.length - 1; i >= 0; --i) //@QtQuick2
            {
                child = repeaterContainer.children[i];
                if (child.hasOwnProperty("item") && child.item)
                {
                    child.item.parentRenderer = columnRenderer;
                    newChildren.push(child.item);
                }
            }

            children = newChildren;
            if (debug) Debug.print("UPDATE ITEMS -> DONE: " + count);
        }
        finally
        {
            if (debug) Debug.print("ENDING UPDATE");
            endUpdate();
            if (debug) Debug.print("END UPDATE");
        }

        _sendDifferentialUpdates();
    }

    function _sendDifferentialUpdates()
    {
        if (Private.actions.length > 0)
        {
            var updates = [];
            columnRenderer.generation = columnRenderer.generation + 1;

            for (var i = 0; i < Private.actions.length; ++i)
            {
                var action = Private.actions[i];
                action.parentName = columnRenderer.identifier;

                // Does this action contain an item pointer?
                // We need to grab the content of the item and remove the item pointer from the action...
                if (action.hasOwnProperty("item"))
                {
                    if (!isTemplateRenderer(action.item))
                        continue;

                    if (action.item.contentDirty)
                        action.item.updateContent();

                    action.content = action.item.replaceMarkerForContent();
                    action.itemName = action.item.identifier;
                    delete action.item;
                }

                if (action.hasOwnProperty("afterItem"))
                {
                    if (!isTemplateRenderer(action.afterItem))
                        continue;

                    action.afterItemName = action.afterItem.identifier;
                    delete action.afterItem;
                }

               updates.push(action);
            }

            if (debug) Debug.print("updates: " + JSON.stringify(updates, null, "  "));

            topLevelRenderer.changeNotificatorDataBridge.sendData({
                action: "updateChildren",
                itemName: columnRenderer.identifier,
                parentName: columnRenderer.parentRenderer ? columnRenderer.parentRenderer.identifier : null,
                generation: generation,
                updates: updates
            });

            Private.reset();
        }
    }

    function _sendContentUpdate() // override
    {
        if (columnRenderer.contentDirty)
            columnRenderer.updateContent();

        topLevelRenderer.changeNotificatorDataBridge.sendData({
            action: "update",
            property: columnRenderer.identifier,
            generation: generation,
            parentName: columnRenderer.parentRenderer ? columnRenderer.parentRenderer.identifier : null,
            value: columnRenderer.content
        });

        _sendVisibilityStatus();
    }

    function _scheduleUpdateChildren()
    {
        beginUpdate();
        children = []; // drop all references to children
        DeferredExecution.invoke(columnRenderer.updateChildren, "updateChildren", columnRenderer);
    }

    function _handleTemplateChanged() // override
    {
        // Override default handling of sending content updates
        // We do differential updates above.
    }

    function getContentMarkerAttributes()
    {
        var attrs = _ViridityHTMLSegment_getContentMarkerAttributes();
        attrs["generation"] = generation;
        return attrs;
    }

    function _addItem(index, item)
    {
        if (index == 0)
            Private.itemPrepended(item.item);
        else if (index == repeater.count - 1)
            Private.itemAppended(item.item);
        else
            Private.itemInsertedAfter(item.item, repeaterContainer.children[index - 1].item);

        columnRenderer._scheduleUpdateChildren();
    }

    function _removeItem(index, item)
    {
        Private.itemRemoved(item.item);
        columnRenderer._scheduleUpdateChildren();
    }

    // Why is this repeaterContainer here?
    // Qt Quick's Repeater can only repeat Item-based components inside an Item-derived object.

    //property variant repeaterContainer: //@QtQuick1: Required to see and access the repeater
        Item {
            id: repeaterContainer

            //property alias repeater: repeater //@QtQuick1: Required to see and access the repeater
            Repeater {
                id: repeater

                model: columnRenderer.model

                onItemAdded: // index, item
                {
                    if (columnRenderer.debug) Debug.print("Repeater ITEM ADDED-> " + index + ": " + item);
                    columnRenderer._addItem(index, item);
                }

                onItemRemoved: // index, item
                {
                    if (!columnRenderer) return;
                    if (columnRenderer.debug) Debug.print("Repeater ITEM REMOVED-> " + index + ": " + item);
                    columnRenderer._removeItem(index, item);
                }

                Item {
                    id: itemWrapper

                    property int currentIndex: index
                    property variant currentModelData: modelData

                    property QtObject item: null

                    Component.onCompleted:
                    {
                        // Instantiate object and set context object to itemWrapper, so we can access currentIndex and currentModelData.
                        item = EngineUtils.createObjectWithContextObject(columnRenderer.delegate, itemWrapper);

                        item.childPrefix = columnRenderer.createUniqueID();
                        item.name = item.name + "_" + item.childPrefix;
                        item.parentRenderer = columnRenderer;
                    }

                    Component.onDestruction:
                    {
                        if (item)
                            item.destroy();
                    }
                }
            }
        }
}
