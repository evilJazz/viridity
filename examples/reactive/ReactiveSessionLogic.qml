import QtQuick 2.0
import Viridity 1.0
import KCL 1.0

QtObjectWithChildren {

    ViridityHTMLSessionSegment {
        targetHTMLDocument: myReactiveDoc
        name: "sessionSegmentTop"

        ViridityHTMLButton {
            name: "buttonClear"
            title: "Clear List"
            onClicked: listModel.clear()
        }

        ViridityHTMLButton {
            name: "buttonAppend"
            title: "Append new item"
            onClicked:
            {
                listModel.appendData("This is sparta! " + (new Date).getTime());
                ++sessionInfoSegment.totalItemsCreatedInSession;
            }
        }

        ViridityHTMLButton {
            name: "buttonPrepend"
            title: "Prepend new item"
            onClicked:
            {
                listModel.insertData(0, "This is sparta! " + (new Date).getTime());
                ++sessionInfoSegment.totalItemsCreatedInSession;
            }
        }

        ViridityHTMLButton {
            name: "buttonDelete"
            title: "Remove first item"
            onClicked: listModel.remove(0);
        }

        ViridityHTMLColumn {
            ViridityHTMLButton {
                name: "dumpStructure"
                title: "Dump structure"
                onClicked: myReactiveDoc.dumpRendererStructure();
            }

            ViridityHTMLButton {
                name: "detachListModel"
                title: "Detach Model"
                onClicked: htmlRepeater.model = null;
            }

            ViridityHTMLButton {
                name: "attachListModel"
                title: "Attach Model"
                onClicked: htmlRepeater.model = listModel;
            }
        }

        ViridityHTMLColumn {
            ViridityHTMLButton {
                name: "enableTimer"
                title: "Start auto-add items"
                onClicked: autoAddTimer.running = true;
                visible: !autoAddTimer.running
            }

            ViridityHTMLButton {
                name: "disableTimer"
                title: "Stop auto-add items"
                onClicked: autoAddTimer.running = false;
                visible: autoAddTimer.running
            }
        }
    }

    ViridityHTMLSessionSegment {
        targetHTMLDocument: myReactiveDoc
        name: "sessionSegmentBottom"

        ViridityHTMLSegment {
            name: "sessionInfoSegment"
            id: sessionInfoSegment
            templateText: "Items created in this session: ${totalItemsCreatedInSession}"

            property int totalItemsCreatedInSession: 0
        }

        ViridityHTMLQtQuickDisplay {
            name: "qtQuickDisplay"
            width: 300
            height: 300
            displayItem: testRectangle
        }
    }

}
