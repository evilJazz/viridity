import QtQuick 2.2

Row {
    id: root
    property bool checked: false
    property alias text: text.text

    spacing: 5

    Rectangle {
        width: height
        height: text.height

        Rectangle {
            anchors.fill: parent
            anchors.margins: 4
            color: "black"
            visible: root.checked
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.checked = !root.checked
        }
    }

    Text {
        id: text
    }
}

