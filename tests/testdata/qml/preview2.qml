import QtQuick 1.0

Item {
    Flickable {
        anchors.fill: parent
        clip: true

        contentWidth: contentImage.width
        contentHeight: contentImage.height

        Image {
            id: contentImage
            source: "../images/lena.png"

            opacity: 0.6

            Rectangle {
                anchors.centerIn: parent
                width: 100
                height: 100
                color: "blue"
            }

            smooth: true

            transform: Rotation { origin.x: contentImage.width / 2; origin.y: contentImage.height / 2; axis { x: 0.5; y: 1; z: 0.2 } angle: 54 }
        }

        Connections {
            target: fileUploadHandler

            onNewFilesUploaded: // files
            {
                contentImage.source = FsUtils.urlFromLocalPath(files[0].tempFileName);
                console.log(JSON.stringify(files));
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 100
        height: 100
        color: "red"
    }

    Component.onDestruction: Debug.print("Instance of preview2.qml destroyed!")
}
