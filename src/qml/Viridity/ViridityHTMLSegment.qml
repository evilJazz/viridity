/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

NativeTemplateRenderer {
    id: renderer

    renderDelay: -1 // Disable auto-updating

    property bool inSessionContext: typeof(currentSession) != "undefined"

    property string propertyMarkerElement: "span"
    property variant propertyMarkerAttributes: ({})

    property string contentMarkerElement: "div"
    property variant contentMarkerAttributes: ({})

    property bool visible: true
    onVisibleChanged: _sendVisibilityStatus()

    function formatAttributes(attrs, forContent)
    {
        var result = "";

        // Copy the input attrs to our private copy
        // because otherwise we might work on a reference to a property...
        var pattrs = {};
        for (var key in attrs)
            pattrs[key] = attrs[key];

        if (forContent && !visible)
        {
            if (pattrs.hasOwnProperty("style"))
                pattrs["style"] += attrs["style"] + ";display: none;";
            else
                pattrs["style"] = "display: none;";
        }

        for (var key in pattrs)
            result += " " + key + "=\"" + pattrs[key] + "\"";

        return result;
    }

    function replaceMarkerForProperty(propertyName)
    {
        return _ViridityHTMLSegment_replaceMarkerForProperty(propertyName);
    }

    function _ViridityHTMLSegment_replaceMarkerForProperty(propertyName)
    {
        if (propertyName !== "targetId")
        {
            var id = renderer.identifier + propertyName;
            var attrs = formatAttributes(getPropertyMarkerAttributes(), false);

            return '<' + propertyMarkerElement + ' id="' + id + '"' + attrs + '>' +
                _TemplateRenderer_replaceMarkerForProperty(propertyName) +
                '</' + propertyMarkerElement + '>\n';
        }
        else
            return _TemplateRenderer_replaceMarkerForProperty(propertyName);
    }

    function replaceMarkerForContent()
    {
        return _ViridityHTMLSegment_replaceMarkerForContent();
    }

    function _ViridityHTMLSegment_replaceMarkerForContent()
    {
        var id = renderer.identifier;
        var attrs = formatAttributes(getContentMarkerAttributes(), true);

        return '<' + contentMarkerElement + ' id="' + id + '"' + attrs + '>' +
                _TemplateRenderer_replaceMarkerForContent() +
                '</' + contentMarkerElement + '>\n';
    }

    property QtObject connectionsTopLevelRenderer:
        Connections {
            target: topLevelRenderer &&
                    topLevelRenderer.hasOwnProperty("changeNotificatorDataBridge") ? topLevelRenderer : null

            onDataReceived: // input
            {
                if (input.action === "contentUpdate")
                {
                    if (input.itemName === renderer.identifier)
                        _sendContentUpdate();
                }
            }
        }

    property QtObject connectionsRenderer:
        Connections {
            target: topLevelRenderer &&
                    topLevelRenderer.hasOwnProperty("changeNotificatorDataBridge") ? renderer : null

            onPropertyChanged: renderer._sendPropertyUpdate(propertyName)
            onTemplateTextChanged: renderer._handleTemplateChanged();
        }

    // Overrideable functions
    function _ViridityHTMLSegment_getPropertyMarkerAttributes()
    {
       return propertyMarkerAttributes;
    }

    function getPropertyMarkerAttributes()
    {
        return _ViridityHTMLSegment_getPropertyMarkerAttributes();
    }

    function _ViridityHTMLSegment_getContentMarkerAttributes()
    {
        return contentMarkerAttributes;
    }

    function getContentMarkerAttributes()
    {
        return _ViridityHTMLSegment_getContentMarkerAttributes();
    }

    function _ViridityHTMLSegment_handleTemplateChanged()
    {
        _sendContentUpdate();
    }

    function _handleTemplateChanged()
    {
        _ViridityHTMLSegment_handleTemplateChanged();
    }

    function _ViridityHTMLSegment_sendContentUpdate()
    {
        if (renderer.contentDirty)
            renderer.updateContent();

        topLevelRenderer.changeNotificatorDataBridge.sendData({
            action: "update",
            property: renderer.identifier,
            value: renderer.content
        });

        _sendVisibilityStatus();
    }

    function _sendContentUpdate()
    {
        _ViridityHTMLSegment_sendContentUpdate();
    }

    function _ViridityHTMLSegment_sendPropertyUpdate(propertyName)
    {
        if (isMarkerUsedInTemplate(propertyName))
        {
            topLevelRenderer.changeNotificatorDataBridge.sendData({
                action: "update",
                property: renderer.identifier + propertyName,
                parentName: (renderer.parentRenderer ? renderer.parentRenderer.identifier : null),
                value: renderer[propertyName]
            });
        }
    }

    function _sendPropertyUpdate(propertyName)
    {
        _ViridityHTMLSegment_sendPropertyUpdate(propertyName);
    }

    function _ViridityHTMLSegment_sendVisibilityStatus()
    {
        topLevelRenderer.changeNotificatorDataBridge.sendData({
            action: visible ? "show" : "hide",
            itemName: renderer.identifier
        });
    }

    function _sendVisibilityStatus()
    {
        _ViridityHTMLSegment_sendVisibilityStatus()
    }
}
