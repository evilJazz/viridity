import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

QtObjectWithChildren {
    id: global

    ViridityHTMLDocument {
        id: myReactiveDoc
        targetId: "myReactiveDoc"

        templateSource: Qt.resolvedUrl("test.template.html")

        title: "Viridity Reactive Example"

        publishAtUrl: "/index.html"

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
                    templateText: "<strong>${index} -> ${title} - ${name}</strong>"

                    property int index: currentIndex
                    property string title: currentModelData
                }
            }

            Rectangle {
                id: testRectangle
                color: "red"
                width: 100
                height: 200

                Rectangle {
                    id: rect
                    width: 111
                    height: 200
                    color: "gray"

                    x: 80
                    y: 200
                }

                MouseArea {
                    id: mouseArea

                    anchors.fill: parent

                    property variant startX
                    property variant startY

                    property variant mousePressX
                    property variant mousePressY

                    onPressed:
                    {
                        forceActiveFocus();

                        mousePressX = mouse.x;
                        mousePressY = mouse.y;

                        startX = rect.x;
                        startY = rect.y;
                    }

                    onPositionChanged:
                    {
                        if (pressed)
                        {
                            rect.x = startX - (mousePressX - mouse.x);
                            rect.y = startY - (mousePressY - mouse.y);
                        }
                    }
                }
            }

            ListModel {
                id: listModel

                function appendData(data)
                {
                    append({ title: data });
                    ++infoSegment.totalItemsCreated;
                }

                function insertData(index, data)
                {
                    insert(index, { title: data });
                    ++infoSegment.totalItemsCreated;
                }
            }

            Timer {
                id: autoAddTimer
                repeat: true
                interval: 500
                //running: true
                onTriggered:
                {
                    if (listModel.count >= 10)
                        listModel.remove(0);

                    var index = Math.round(Math.random() * listModel.count);
                    listModel.insertData(index, "Value: " + Math.random());
                }
            }
        }
    }
}
