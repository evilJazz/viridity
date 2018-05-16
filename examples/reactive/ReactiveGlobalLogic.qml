import QtQuick 2.0
import KCL 1.0
import Viridity 1.0

QtObjectWithChildren {
    id: global

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: global.test = global.test + 1;
    }

    property int test: 20

    Timer {
        interval: 5000
        running: true
        repeat: true
        onTriggered: global.slow = global.slow + 1;
    }

    property int slow: 1

    ViridityHTMLDocument {
        id: testDoc
        targetId: "myTestDoc"

        templateSource: Qt.resolvedUrl("test.template.html")
        publishAtUrl: "/test.html"

        property int test: global.test
        property string test2: '<font color="#F00">' + (test + Math.random()) + '</font>'

        ViridityHTMLColumn {
            name: "segment1"

            ViridityHTMLSegment {
                name: "segment2"
                template: "<button>${blah}</button>"
                property string blah: global.test + 10
            }

            ViridityHTMLSegment {
                name: "sessionSegment"
            }

            ViridityHTMLButton {
                name: "buttonClear"
                title: "Clear List"
                onClicked: listModel.clear()
            }

            ViridityHTMLButton {
                name: "buttonAdd"
                title: "Add new item"
                onClicked: listModel.append({ title: "This is sparta!" });
            }

            ViridityHTMLButton {
                name: "buttonDelete"
                title: "Remove first item"
                onClicked: listModel.remove(0);
            }

            ViridityHTMLRepeater {
                id: htmlRepeater
                name: "repeaterTest"
                model: listModel

                contentMarkerElement: "ul"

                ViridityHTMLSegment {
                    name: "listElement"
                    contentMarkerElement: "li"
                    template: "<strong>${slow} -> ${title}</strong>"

                    property int slow: currentIndex
                    property string title: currentModelData
                }
            }

            ListModel {
                id: listModel
            }

            Timer {
                repeat: true
                interval: 500
                running: true
                onTriggered:
                {
                    if (listModel.count > 10)
                        listModel.remove(0);

                    var index = Math.round(Math.random() * listModel.count);
                    listModel.insert(index, { title: "Value: " + Math.random() });
                }
            }
        }
    }
}
