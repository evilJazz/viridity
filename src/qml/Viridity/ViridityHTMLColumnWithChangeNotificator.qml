/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLColumn {
    id: renderer

    property alias targetId: changeNotificatorDataBridge.targetId
    name: targetId

    property string publishAtUrl: ""

    signal dataReceived(variant input)

    function sendFullContentUpdate()
    {
        renderer._sendContentUpdate();
    }

    property QtObject changeNotificatorDataBridge:
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
