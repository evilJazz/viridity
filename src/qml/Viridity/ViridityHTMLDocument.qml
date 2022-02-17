/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "private"

ViridityHTMLColumnWithChangeNotificator {
    id: document

    property bool isHTMLDocument: true

    property string publishAtUrl: ""

    property string title: ""

    property variant dependencies: []

    property QtObject defaults:
        QtObject {
            property variant dependencies: [
                "https://code.jquery.com/jquery-1.11.2.min.js",
                "/Viridity.js",
                "/ViridityAuto.js",
                "/DataBridge.js",
                "/DocumentRenderer.js"
            ]
        }

    property QtObject dependencyRegistry:
        ViridityHTMLDocumentDependencyRegistry {
            id: dependencyRegistry

            Component.onCompleted:
            {
                register(defaults); // Register document.defaults.dependencies
                register(document); // Register document.dependencies
            }
        }

    property QtObject requestHandler:
        ViridityDocumentRequestHandler {
            document: document
            mimeType: "text/html"
            charset: "utf-8"
        }

    function replaceMarkerForProperty(propertyName)
    {
        return _ViridityHTMLDocument_replaceMarkerForProperty(propertyName);
    }

    function _ViridityHTMLDocument_replaceMarkerForProperty(propertyName)
    {
        if (propertyName !== "title")
            return _ViridityHTMLSegment_replaceMarkerForProperty(propertyName)
        else
            return _TemplateRenderer_replaceMarkerForProperty(propertyName);
    }

    // Template methods:

    function insertDependencies(type)
    {
        return dependencyRegistry.getHTMLSnippet();
    }

    function makeRelative(url)
    {
        var base = FsUtils.extractPath(publishAtUrl);
        return FsUtils.makeRelativeFilePath(base, url);
    }
}
