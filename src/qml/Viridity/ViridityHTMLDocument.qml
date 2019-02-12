/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLColumnWithChangeNotificator {
    id: renderer

    property string publishAtUrl: ""

    property QtObject requestHandler:
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
}
