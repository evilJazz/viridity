/* Work in progress */

import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

ViridityHTMLColumnWithChangeNotificator {
    id: renderer

    //property QtObject targetHTMLDocument: null //@QtQuick1
    property ViridityHTMLDocument targetHTMLDocument: null //@QtQuick2

    targetId: targetHTMLDocument != null ? targetHTMLDocument.targetId + "Session" : ""

    Connections {
        target: renderer.inSessionContext ? currentSession : null
        onAttached: renderer.sendFullContentUpdate();
    }

    Component.onCompleted: renderer.sendFullContentUpdate();
}
