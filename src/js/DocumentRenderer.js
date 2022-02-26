/****************************************************************************
**
** Copyright (C) 2012-2016 Andre Beckedorf, Meteora Softworks
** Contact: info@meteorasoftworks.com
**
** This file is part of Viridity
**
** $VIRIDITY_BEGIN_LICENSE:MIT$
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
** $VIRIDITY_END_LICENSE$
**
****************************************************************************/

var DR = {
    documentRenderers: [],
    dataBridges: {},

    dependencies: {},

    components: [],
    componentInstances: {},

    requires: function(depArray)
    {
        for (var i = 0; i < depArray.length; ++i)
        {
            var url = depArray[i];

            if (!DR.dependencies[url])
            {
                if (/.css/.test(url))
                {
                    DR.dependencies[url] = $('<link>')
                        .appendTo('head')
                        .attr({
                            type: 'text/css',
                            rel: 'stylesheet',
                            href: url
                        });
                }
                else if (/.js/.test(url))
                {
                    DR.dependencies[url] = $('<script>')
                        .appendTo('head')
                        .attr({
                            type: 'text/javascript',
                            src: url
                        });
                }
            }
        }
    },

    a: function(targetIdOrDataBridge, itemName, actionName, params, callback)
    {
        var dataBridge = typeof(targetIdOrDataBridge) == "string" ? DR.dataBridges[targetIdOrDataBridge] : targetIdOrDataBridge;
        if (dataBridge)
        {
            dataBridge.sendData({
                name: itemName,
                action: actionName,
                params: params
            }, callback);
        }
        else
            console.log("Target databridge '" + targetId + "' not found while sending action '" + actionName + "' from element '" + itemName + "'.");
    },

    remove: function(itemName)
    {
        $("#" + itemName).remove();
    },

    registerComponent: function(dataTypeName, classImpl, autoAttach)
    {
        DR.components.push({
            dataTypeName: dataTypeName,
            ClassImpl: classImpl
        });

        if (autoAttach)
            DR.autoAttach(dataTypeName);
    },

    autoAttach: function(dataTypeName)
    {
        for (var i = 0; i < DR.components.length; ++i)
        {
            var component = DR.components[i];

            if (dataTypeName && component.dataTypeName !== dataTypeName)
                continue;

            var dataType = "data-" + component.dataTypeName + "-targetId";

            var elements = $("[" + dataType + "]");

            elements.each(function (index)
            {
                var element = $(this);

                var isAlreadyAttached = element.attr("data-vdr-attached") == 1;
                var targetId = element.attr(dataType);

                if (!isAlreadyAttached && typeof(targetId) != "undefined")
                {
                    element.attr("data-vdr-attached", 1);

                    var itemName = element.attr("id"); // name in Viridity terminology

                    var remote = {
                        dataBridge: DR.dataBridges[targetId],
                        targetId: targetId,
                        itemName: itemName,
                        call: function(action, params, callback)
                        {
                            DR.a(remote.dataBridge, remote.itemName, action, params, callback);
                        }
                    };

                    var componentInstance = new component.ClassImpl(element, remote, itemName);
                    DR.componentInstances[itemName] = componentInstance;
                }
            });
        }
    }
};

var DocumentRenderer = function(viridityChannel, id)
{
    var v = viridityChannel;

    var c =
    {
        dataBridgeGlobal: undefined,
        dataBridgeSession: undefined,

        requestContentUpdate: function(dataBridge, itemName)
        {
            dataBridge.sendData({
                action: "contentUpdate",
                itemName: itemName
            });
        },


        actions:
        {
            update: function(dataBridge, input)
            {
                var item = $("#" + input.property);

                if (item.length == 0)
                {
                    c.requestContentUpdate(dataBridge, input.parentName);
                }
                else
                {
                    item.html(input.value);
                    if (input.hasOwnProperty("generation"))
                        item.attr("generation", input.generation);
                }
            },

            show: function(dataBridge, input)
            {
                var parentItem = $("#" + input.itemName);
                if (parentItem.length == 1)
                    parentItem.show();
            },

            hide: function(dataBridge, input)
            {
                var parentItem = $("#" + input.itemName);
                if (parentItem.length == 1)
                    parentItem.hide();
            },

            requireDependencies: function(dataBridge, input)
            {
                DR.requires(input.dependencies);

                if (input.runAutoAttachment && typeof(ViridityAuto) != "undefined")
                    ViridityAuto.autoAttach();
            },

            updateChildren: function(dataBridge, input)
            {
                var parentItem = $("#" + input.itemName);

                var parentGeneration = parseInt(parentItem.attr("generation"));

                // Check if parent's generation is predecessor of new generation...
                if (parentItem.length == 0 ||
                    parentGeneration + 1 != input.generation)
                {
                    c.requestContentUpdate(dataBridge, input.itemName);
                }
                else
                {
                    for (var i = 0; i < input.updates.length; ++i)
                    {
                        var update = input.updates[i];

                        if (update.action == "remove")
                        {
                            var target = $("#" + update.itemName);
                            if (target.length == 1)
                                target.remove();
                            else
                            {
                                if (v.debugVerbosity > 2)
                                    console.log("updateChildren " + i + " remove -> " + update.itemName + " not found.");

                                c.requestContentUpdate(dataBridge, update.parentName);
                                break;
                            }
                        }
                        else if (update.action == "prepend")
                        {
                            var parent = $("#" + update.parentName);
                            if (parent.length == 1)
                                parent.prepend(update.content);
                            else
                            {
                                if (v.debugVerbosity > 2)
                                    console.log("updateChildren " + i + " prepend -> " + update.parentName + " not found.");

                                c.requestContentUpdate(dataBridge, update.parentName);
                                break;
                            }
                        }
                        else if (update.action == "append")
                        {
                            var parent = $("#" + update.parentName);
                            if (parent.length == 1)
                                parent.append(update.content);
                            else
                            {
                                if (v.debugVerbosity > 2)
                                    console.log("updateChildren " + i + " append -> " + update.parentName + " not found.");

                                c.requestContentUpdate(dataBridge, update.parentName);
                                break;
                            }
                        }
                        else if (update.action == "insertAfter")
                        {
                            var afterItem = $("#" + update.afterItemName);
                            if (afterItem.length == 1)
                                $(update.content).insertAfter(afterItem);
                            else
                            {
                                if (v.debugVerbosity > 2)
                                    console.log("updateChildren " + i + " insertAfter -> " + update.afterItemName + " not found.");

                                c.requestContentUpdate(dataBridge, update.parentName);
                                break;
                            }
                        }
                    }

                    parentItem.attr("generation", input.generation);
                }
            }
        },

        handleDataReceived: function(dataBridge, input)
        {
            if (input.action in c.actions)
                return c.actions[input.action](dataBridge, input);

            if (input.name in DR.componentInstances)
            {
                var componentInstance = DR.componentInstances[input.name];

                if ("actions" in componentInstance)
                {
                    if (input.action in componentInstance.actions)
                        return componentInstance.actions[input.action](input.params);
                }
            }

            return true;
        }
    };

    // Data bridge for the global logic
    c.dataBridgeGlobal = new DataBridge(viridityChannel, id);
    c.dataBridgeGlobal.onDataReceived = function(input) { return c.handleDataReceived(c.dataBridgeGlobal, input); };

    viridityChannel.on("sessionStart", c.dataBridgeGlobal.subscribe);
    viridityChannel.on("sessionReattached", c.dataBridgeGlobal.subscribe);

    // Data bridge for the session logic
    var sessionTargetId = id + "Session";
    c.dataBridgeSession = new DataBridge(viridityChannel, sessionTargetId);
    c.dataBridgeSession.onDataReceived = function(input) { return c.handleDataReceived(c.dataBridgeSession, input); };

    DR.documentRenderers.push(c);
    DR.dataBridges[id] = c.dataBridgeGlobal;
    DR.dataBridges[sessionTargetId] = c.dataBridgeSession;

    return c;
}

var DocumentRendererGlobal = {
    autoAttach: function(channel)
    {
        var element = $("body");

        var isAlreadyAttached = element.attr("data-vdr-attached") == 1;
        var targetId = element.attr("data-vdr-targetId");

        if (!isAlreadyAttached && typeof(targetId) != "undefined")
        {
            element.attr("data-vdr-attached", 1);

            if (typeof(channel) == "undefined" && typeof(ViridityChannel) != "undefined")
                channel = ViridityChannel;

            var dr = new DocumentRenderer(channel, targetId);
        }
    }
}

if (typeof(ViridityAuto) != "undefined")
{
    ViridityAuto.on("autoAttach", DocumentRendererGlobal.autoAttach);
    ViridityAuto.on("autoAttach", DR.autoAttach);
}
