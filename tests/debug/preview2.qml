import QtQuick 1.0

Item {
    Flickable {
        anchors.fill: parent
        clip: true

        contentWidth: contentImage.width
        contentHeight: contentImage.height

        Image {
            id: contentImage
            source: "qrc:/testimages/lena.png"

            opacity: 0.6

            Rectangle {
                anchors.centerIn: parent
                width: 100
                height: 100
                color: "blue"
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 100
        height: 100
        color: "red"
    }
}
