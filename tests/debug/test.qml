import QtQuick 1.0
import KCL 1.0

import "qrc:/webcontrol/Display.js" as Display

QtObjectWithChildren {
    id: logic

    Timer {
        id: testTimer
        interval: 5000

        onTriggered:
        {
            Display.sendCommand(["Hello World!", 34589237458.23452345], function (response, displayId)
            {
                console.log("Response from display " + displayId + ": " + response);
            });
        }
    }

    Component.onCompleted:
    {
        Display.onNewCommandReceived = function(input)
        {
            testTimer.start();
            console.log("input: " + input);
            return { action: "blah", test: 1243234.3423 };
        }

        //testTimer.start();

        /*
        Display.sendCommand(["Hello World! " + checked, checked], function (response, displayId)
        {
            console.log("Response from display " + displayId + ": " + response);
        });
        */
    }

    Component.onDestruction: console.log("GOODBYE WORLD!")
}
