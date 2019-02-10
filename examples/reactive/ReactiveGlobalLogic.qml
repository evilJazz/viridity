import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

QtObjectWithChildren {
    id: global

    ViridityHTMLDocument {
        id: myReactiveDoc
        targetId: "myReactiveDoc"

        templateSource: Qt.resolvedUrl("test.template.html")
        publishAtUrl: "/test.html"

        ViridityHTMLColumn {
            name: "columnSegment"

            ViridityHTMLSegment {
                name: "sessionSegmentTop" // Specified in ReactiveSessionLogic
            }

            ViridityHTMLSegment {
                id: infoSegment
                name: "infoSegment"
                templateText: "<div>Items in list: ${listModelCount}, total items created over lifetime: ${totalItemsCreated}</div>"
                property alias listModelCount: listModel.count
                property int totalItemsCreated: 0
            }

            ViridityHTMLSegment {
                name: "sessionSegmentBottom" // Specified in ReactiveSessionLogic
            }

            ViridityHTMLRepeater {
                id: htmlRepeater
                name: "itemList"
                model: listModel

                contentMarkerElement: "ul"

                ViridityHTMLSegment {
                    name: "listElement"
                    contentMarkerElement: "li"
                    templateText: "<strong>${index} -> ${title}</strong>"

                    property int index: currentIndex
                    property string title: currentModelData
                }
            }

            ListModel {
                id: listModel
                onRowsInserted: ++infoSegment.totalItemsCreated;
            }

            Timer {
                repeat: true
                interval: 500
                running: true
                onTriggered:
                {
                    if (listModel.count >= 10)
                        listModel.remove(0);

                    var index = Math.round(Math.random() * listModel.count);
                    listModel.insert(index, { title: "Value: " + Math.random() });
                }
            }
        }
    }
}
