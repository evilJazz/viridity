/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityRequestHandler {
    id: requestHandler

    property NativeTemplateRenderer document
    property string mimeType: "text/plain"
    property string charset: "utf-8"

    contentCachingEnabled: true

    cachedContentValid: !document.contentDirty
    handlesUrl: document.publishAtUrl.length > 0 ? "^" + document.publishAtUrl : ""

    function doesHandleRequest(request)
    {
        return false; // default, if publishAtUrl is empty...
    }

    function handleRequest(request, response)
    {
        if (document.contentDirty)
            document.updateContent();

        response.addHeader("Content-Type", mimeType + "; charset=" + charset);
        response.writeHead(200);
        response.end(document.content);
    }
}
