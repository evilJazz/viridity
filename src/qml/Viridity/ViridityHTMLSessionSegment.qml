/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLColumnWithChangeNotificator {
    id: renderer

    property ViridityHTMLDocument targetHTMLDocument: null

    targetId: targetHTMLDocument != null ? targetHTMLDocument.targetId + "Session" : ""

    Connections {
        target: currentSession
        onAttached: renderer.sendFullContentUpdate();
    }
}
