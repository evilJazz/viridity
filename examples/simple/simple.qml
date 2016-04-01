import QtQuick 2.0
import Viridity 1.0

ViridityDataBridge {
    session: currentSession // context property pointing to ViriditySession instance set when setting up the QML engine + logic in AbstractViriditySessionManager::initSession
    targetId: "test"

    function onDataReceived(input)
    {
        if (input.action == "hello")
            return { text: "Hello World!", success: true };
        else
            return { success: false };
    }

    Connections {
        target: currentSession

        onAttached:
        {
            sendData(
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
            sendData(
                "Session is initialized!",
                function(response, sessionId)
                {
                    console.log("Input received from client: " + JSON.stringify(response));
                }
            );

            console.log("Session initialized.");
        }

        onDetached:
        {
            console.log("Client detached.");
        }

        onDeinitializing:
        {
            console.log("Session deinitializing.");
        }
    }

    Component.onCompleted: console.log("Logic created.");
    Component.onDestruction: console.log("Killing logic.");
}
