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
            name: "buttonAdd"
            title: "Add new item"
            onClicked:
            {
                listModel.append({ title: "This is sparta! " + (new Date).getTime() });
                ++sessionInfoSegment.totalItemsCreatedInSession;
            }
        }

        ViridityHTMLButton {
            name: "buttonDelete"
            title: "Remove first item"
            onClicked: listModel.remove(0);
        }
    }

    ViridityHTMLSessionSegment {
        targetHTMLDocument: myReactiveDoc
        name: "sessionSegmentBottom"

        ViridityHTMLSegment {
            id: sessionInfoSegment
            templateText: "<div>Items created in this session: ${totalItemsCreatedInSession}</div>"

            property int totalItemsCreatedInSession: 0
        }
    }

}
