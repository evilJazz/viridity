import QtQuick 1.0
import KCL 1.0
import Viridity 1.0

QtObjectWithChildren {
    id: logic

    ViridityDataBridge {
        id: dataBridge1
        session: currentSession
        targetId: "data1"

        function onNewCommandReceived(input)
        {
            testTimer1.start();
            console.log("input1: " + input);
            return { action: "blah111", test: 1243234.3423 };
        }
    }

    ViridityDataBridge {
        id: dataBridge2
        session: currentSession
        targetId: "data2"

        function onNewCommandReceived(input)
        {
            testTimer2.start();
            console.log("input 2: " + input);
            return { action: "blah222", test: 1243234.3423 };
        }
    }

    Timer {
        id: testTimer1
        interval: 5000

        onTriggered:
        {
            dataBridge1.sendData(["Hello World! 111", 34589237458.23452345], function (response, sessionId)
            {
                console.log("Response from session " + sessionId + ": " + response);
            });
        }
    }

    Timer {
        id: testTimer2
        interval: 5000

        onTriggered:
        {
            dataBridge2.sendData(["Hello World! 222", 34589237458.23452345], function (response, sessionId)
            {
                console.log("Response from session " + sessionId + ": " + response);
            });
        }
    }

    Component.onCompleted:
    {
        //testTimer.start();
    }

    Component.onDestruction: console.log("Logic of session " + currentSession.id + " destroyed!")
}
