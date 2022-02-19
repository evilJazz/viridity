import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick.Controls 2.0

import Viridity 1.0
import KCL 1.0

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    CheckBox {
        id: btnEnableWebserver
        text: "Enable webserver"

        BidirectionalBinding {
            leftTarget: btnEnableWebserver
            leftPropertyName: "checked"
            rightTarget: webServer
            rightPropertyName: "enabled"
        }
    }

    ViridityWebServer {
        id: webServer

        bindAddress: "127.0.0.1"
        port: 8089

        ViridityHTMLDocument {
            id: myReactiveDoc
            targetId: "myReactiveDoc"

            templateSource: Qt.resolvedUrl("test.template.html")

            title: "Viridity Mixed Example"

            publishAtUrl: "/index.html"

            ViridityHTMLSegment {
                id: globalClickSegment
                name: "counter"
                templateText: "The button was clicked \${buttonClickCount} times overall."
                property int buttonClickCount: 0
            }

             ViridityHTMLSegment {
                name: "sessionSegment" // Specified in sessionLogicComponent
            }
        }

        sessionLogicComponent:
            ViridityHTMLSessionSegment {
                targetHTMLDocument: myReactiveDoc
                name: "sessionSegment"

                ViridityHTMLSegment {
                    id: clickSegment
                    templateText: "You clicked \${buttonClickCount} times in this session."
                    property int buttonClickCount: 0
                }

                ViridityHTMLButton {
                    title: "CLICK ME!"
                    onClicked:
                    {
                        ++clickSegment.buttonClickCount;
                        ++globalClickSegment.buttonClickCount;
                    }
                }
            }

        onOpened: console.log("Webserver now listening on " + bindAddress + ":" + port)
        onClosed: console.log("Webserver shut down.");
    }
}
