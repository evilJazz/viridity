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
    }
}
