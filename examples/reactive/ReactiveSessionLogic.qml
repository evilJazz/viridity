import QtQuick 2.0
import Viridity 1.0
import KCL 1.0

ViridityDataBridge {
    session: currentSession // context property pointing to ViriditySession instance set when setting up the QML engine + logic in AbstractViriditySessionManager::initSession
    targetId: "test"

    Connections {
        target: currentSession

        onAttached: Debug.print("Client attached.");
        onInitialized: Debug.print("Session initialized.");
        onDetached: Debug.print("Client detached.")
        onDeinitializing: Debug.print("Session deinitializing.")
    }

    ViridityHTMLDocument {
        id: doc
        targetId: "myTestDocSession"
        name: "sessionSegment"

        template: globalLogic.slow
    }

    Component.onCompleted: Debug.print("Logic created.")
    Component.onDestruction: Debug.print("Killing logic.")
}
