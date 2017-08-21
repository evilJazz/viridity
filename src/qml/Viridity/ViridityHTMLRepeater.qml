/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "qrc:/KCL/DeferredExecution.js" as DeferredExecution
import "ViridityHTMLRepeater.js" as Private

ViridityHTMLColumn {
    id: column
    objectName: "repeater"

    default property Component delegate
    property alias model: repeater.model
    property alias count: repeater.count

    property int generation: 0

    property variant _ViridityHTMLRepeater_ignoredPropertyNames: _ViridityHTMLColumn_ignoredPropertyNames.concat(["model", "count", "generation"])
    ignoredPropertyNames: _ViridityHTMLRepeater_ignoredPropertyNames

    property bool debug: false

    function updateChildren()
    {
        try
        {
            if (debug) console.log("BEGINNING UPDATE");
            beginUpdate();
            if (debug) console.log("BEGIN UPDATE");

            if (debug) console.log("UPDATE ITEMS -> NEW COUNT: " + count);

            var newChildren = [];

            for (var i = 0; i < children.length; ++i)
            {
                var child = children[i];
                if (!isTemplateRenderer(child))
                    newChildren.push(children[i]);
            }

            for (var i = 0; i < dummyContainer.children.length; ++i)
            {
                var child = dummyContainer.children[i];
                if (child.hasOwnProperty("item") && child.item)
                {
                    child.item.topLevelTemplateRenderer = column.topLevelTemplateRenderer;
                    newChildren.push(child.item);
                }
            }

            children = newChildren;
            if (debug) console.log("UPDATE ITEMS -> DONE: " + count);
        }
        finally
        {
            if (debug) console.log("ENDING UPDATE");
            endUpdate();
            if (debug) console.log("END UPDATE");
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
                action.parentName = column.name;

                if (action.item)
                {
                    if (action.item.contentDirty)
                        action.item.updateContent();

                    action.content = action.item.replaceMarkerForContent();
                    delete action.item;
                }

                updates.push(action);
            }

            if (debug) console.log("updates: " + JSON.stringify(updates, null, "  "));

            topLevelTemplateRenderer.changeNotificatorDataBridge.sendData({
                action: "updateChildren",
                itemName: column.name,
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

        topLevelTemplateRenderer.changeNotificatorDataBridge.sendData({
            action: "update",
            property: column.name,
            generation: generation,
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

            onItemAdded: // index, item
            {
                if (column.debug) console.log("Repeater ITEM ADDED-> " + index + ": " + item);
                column._addItem(index, item);
            }

            onItemRemoved: // index, item
            {
                if (column.debug) console.log("Repeater ITEM REMOVED-> " + index + ": " + item);
                column._removeItem(index, item);
            }

            // Since Repeater can't natively repeat QtObjects (yet), we use Loader
            // which can instantiate QtObject-based components since QML 2.x
            // TODO: As for QML 1.x we also need to wrap the sourceComponent in an Item...
            Loader {
                sourceComponent: column.delegate

                onItemChanged:
                {
                    if (item)
                        item.name = item.name + "_" + currentSessionManager.createUniqueID().substr(0, 10)
                }

                Connections {
                    target: column
                    onTopLevelTemplateRendererChanged: item.topLevelTemplateRenderer = column.topLevelTemplateRenderer;
                }

                property int currentIndex: index
                property variant currentModelData: modelData
            }
        }
    }
}
