import QtQuick 1.0

Flickable {
    clip: true

    height: 400
    width: 400

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
