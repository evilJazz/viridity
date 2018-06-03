/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLSegment {
    id: renderer

    property alias targetId: changeNotificatorDataBridge.targetId
    name: targetId

    property string publishAtUrl: ""

    signal dataReceived(variant input)

    ViridityRequestHandler {
        id: requestHandler

        contentCachingEnabled: true

        cachedContentValid: !renderer.contentDirty
        handlesUrl: renderer.publishAtUrl.length > 0 ? "^" + renderer.publishAtUrl : ""

        function doesHandleRequest(request)
        {
            return false; // default, if publishAtUrl is empty...
        }

        function handleRequest(request, response)
        {
            if (renderer.contentDirty)
                renderer.updateContent();

            response.writeHead(200);
            response.end(renderer.content);
        }
    }

    property alias changeNotificatorDataBridge: changeNotificatorDataBridge
    ViridityDataBridge {
        id: changeNotificatorDataBridge
        sessionManager: typeof(currentSessionManager) != "undefined" ? currentSessionManager : null
        session: typeof(currentSession) != "undefined" ? currentSession : null

        function onDataReceived(input)
        {
            dataReceived(input);
            return true;
        }
    }
}
