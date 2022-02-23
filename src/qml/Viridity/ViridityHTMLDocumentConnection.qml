/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

import "private/ViridityHTMLHelpers.js" as Internal;

QtObject {
    id: documentConnection

    property QtObject document: null
    signal attached()
    signal detaching()

    property Connections _connections: Connections {
        target: renderer
        onTopLevelRendererChanged: documentConnection._updateAttachment()
    }

    function _updateAttachment()
    {
        var newDocument = Internal.getHTMLDocument(renderer);

        if (document)
        {
            detaching();
            document = null;
        }

        if (newDocument)
        {
            document = newDocument;
            attached();
        }
    }

    Component.onCompleted:
    {
        _updateAttachment();
    }

    Component.onDestruction:
    {
        if (document)
            detaching();
    }
}
