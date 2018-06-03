/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "qrc:/KCL/DeferredExecution.js" as DeferredExecution
import "ViridityHTMLRepeater.js" as Private

ViridityHTMLColumn {
    id: column
    objectName: "repeater"

    childPrefix: currentSessionManager.createUniqueID().substr(0, 10)

    default property Component delegate
    property alias model: repeater.model
    property alias count: repeater.count

    property int generation: 0

    property variant _ViridityHTMLRepeater_ignoredPropertyNames: _ViridityHTMLColumn_ignoredPropertyNames.concat(["model", "count", "generation", "debug"])
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

            for (var i = 0; i < subRenderers.length; ++i)
            {
                child = subRenderers[i];
                if (!isTemplateRenderer(child))
                    newChildren.push(subRenderers[i]);
            }

            for (var i = 0; i < dummyContainer.children.length; ++i)
            {
                child = dummyContainer.children[i];
                if (child.hasOwnProperty("item") && child.item)
                {
                    child.item.parentRenderer = column;
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
            column.generation = column.generation + 1;

            for (var i = 0; i < Private.actions.length; ++i)
            {
                var action = Private.actions[i];
                action.parentName = column.identifier;

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
                itemName: column.identifier,
                parentName: column.parentRenderer ? column.parentRenderer.identifier : null,
                generation: generation,
                updates: updates
            });

            Private.reset();
        }
    }

    function _sendContentUpdate() // override
    {
        if (column.contentDirty)
            column.updateContent();

        topLevelRenderer.changeNotificatorDataBridge.sendData({
            action: "update",
            property: column.identifier,
            generation: generation,
            parentName: column.parentRenderer ? column.parentRenderer.identifier : null,
            value: column.content
        });

        _sendVisibilityStatus();
    }

    function _scheduleUpdateChildren()
    {
        beginUpdate();
        children = []; // drop all references to children
        DeferredExecution.invoke(column.updateChildren, "updateChildren", column);
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
            Private.itemInsertedAfter(item.item, dummyContainer.children[index - 1].item);

        column._scheduleUpdateChildren();
    }

    function _removeItem(index, item)
    {
        Private.itemRemoved(item.item);
        column._scheduleUpdateChildren();
    }

    // Why is this dummyContainer here?
    // Repeater can only repeat Item-based components inside an Item-derived object.
    Item {
        id: dummyContainer

        Repeater {
            id: repeater

            //onCountChanged: column.updateChildren()

            // Note: Loader.itemChanged is executed before onItemAdded...
            onItemAdded: // index, item
            {
                if (column.debug) Debug.print("Repeater ITEM ADDED-> " + index + ": " + item);
                column._addItem(index, item);
            }

            onItemRemoved: // index, item
            {
                if (!column) return;
                if (column.debug) Debug.print("Repeater ITEM REMOVED-> " + index + ": " + item);
                column._removeItem(index, item);
            }

            // Since Repeater can't natively repeat QtObjects (yet), we use Loader
            // which can instantiate QtObject-based components since QML 2.x
            // TODO: As for QML 1.x we also need to wrap the sourceComponent in an Item...
            Loader {
                sourceComponent: column.delegate

                onItemChanged:
                {
                    if (column.debug) Debug.print("Loader ITEM CHANGED: " + item);
                    if (item)
                    {
                        item.childPrefix = currentSessionManager.createUniqueID().substr(0, 10);
                        item.name = item.name + "_" + item.childPrefix;
                        item.parentRenderer = column;
                    }
                }

                Connections {
                    target: column
                    onParentRendererChanged: item.parentRenderer = column;
                    onTopLevelRendererChanged: item.topLevelRenderer = column.topLevelRenderer;
                }

                property int currentIndex: index
                property variant currentModelData: modelData
            }
        }
    }
}
