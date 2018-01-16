import QtQuick 2.0
import Viridity 1.0

ViridityDataBridge {
    id: dataBridge
    session: currentSession // context property pointing to ViriditySession instance set when setting up the QML engine + logic in AbstractViriditySessionManager::initSession
    targetId: "test"

    function onDataReceived(input)
    {
        if (input.action == "hello")
            return { text: "Hello World!", success: true };
        else if (input.action == "startTimer")
            testTimer.start();
        else if (input.action == "stopTimer")
            testTimer.stop();
        else
            return { success: false };
    }

    Timer {
        id: testTimer
        interval: 2000
        repeat: true
        triggeredOnStart: true
        onTriggered: dataBridge.sendData("Timer triggered.")
    }

    Connections {
        target: currentSession

        onAttached:
        {
            dataBridge.sendData(
                "Session is attached!",
                function(response, sessionId)
                {
                    console.log("Input received from client: " + JSON.stringify(response));
                }
            );

            console.log("Client attached.");
        }

        onInitialized:
        {
            dataBridge.sendData(
                "Session is initialized!",
                function(response, sessionId)
                {
                    console.log("Input received from client: " + JSON.stringify(response));
                }
            );

            console.log("Session initialized.");
        }

        onDetached: console.log("Client detached.")
        onDeinitializing: console.log("Session deinitializing.")
    }

    Component.onCompleted: console.log("Logic created.")
    Component.onDestruction: console.log("Killing logic.")
}
