/* Work in progress */

import QtQuick 2.2
import KCL 1.0
import Viridity 1.0

TemplateRenderer {
    id: renderer

    renderDelay: -1

    property string name

    property string propertyMarkerElement: "span"
    property variant propertyMarkerAttributes: ({})

    property string contentMarkerElement: "div"
    property variant contentMarkerAttributes: ({})

    property bool visible: true
    onVisibleChanged: _sendVisibilityStatus()

    function formatAttributes(attrs)
    {
        var result = "";

        // Copy the input attrs to our private copy
        // because otherwise we might work on a reference to a property...
        var pattrs = {};
        for (var key in attrs)
            pattrs[key] = attrs[key];

        if (!visible)
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
        if (propertyName !== "targetId")
        {
            var id = renderer.name + propertyName;
            var attrs = formatAttributes(getPropertyMarkerAttributes());

            return '<' + propertyMarkerElement + ' id="' + id + '"' + attrs + '>' +
                _TemplateRenderer_replaceMarkerForProperty(propertyName) +
                '</' + propertyMarkerElement + '>\n';
        }
        else
            return _TemplateRenderer_replaceMarkerForProperty(propertyName);
    }

    function replaceMarkerForContent()
    {
        var id = renderer.name;
        var attrs = formatAttributes(getContentMarkerAttributes());

        return '<' + contentMarkerElement + ' id="' + id + '"' + attrs + '>' +
                _TemplateRenderer_replaceMarkerForContent() +
                '</' + contentMarkerElement + '>\n';
    }

    Connections {
        target: topLevelTemplateRenderer &&
                topLevelTemplateRenderer.hasOwnProperty("changeNotificatorDataBridge") ? topLevelTemplateRenderer : null

        onDataReceived: // input
        {
            if (input.action === "contentUpdate")
            {
                if (input.itemName === renderer.name)
                    _sendContentUpdate();
            }
        }
    }

    Connections {
        target: topLevelTemplateRenderer &&
                topLevelTemplateRenderer.hasOwnProperty("changeNotificatorDataBridge") ? renderer : null

        onPropertyChanged: renderer._sendPropertyUpdate(propertyName)
        onTemplateChanged: renderer._handleTemplateChanged();
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

        topLevelTemplateRenderer.changeNotificatorDataBridge.sendData({
            action: "update",
            property: renderer.name,
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
        topLevelTemplateRenderer.changeNotificatorDataBridge.sendData({
            action: "update",
            property: renderer.name + propertyName,
            value: renderer[propertyName]
        });
    }

    function _sendPropertyUpdate(propertyName)
    {
        _ViridityHTMLSegment_sendPropertyUpdate(propertyName);
    }

    function _ViridityHTMLSegment_sendVisibilityStatus()
    {
        topLevelTemplateRenderer.changeNotificatorDataBridge.sendData({
            action: visible ? "show" : "hide",
            itemName: renderer.name
        });
    }

    function _sendVisibilityStatus()
    {
        _ViridityHTMLSegment_sendVisibilityStatus()
    }
}
