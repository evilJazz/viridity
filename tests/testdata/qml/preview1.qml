import QtQuick 1.0

Image {
    id: image
    source: "../images/lena.png"
    fillMode: Image.PreserveAspectFit
    smooth: true

    Connections {
        target: fileUploadHandler

        onNewFilesUploaded: // files
        {
            image.source = FsUtils.urlFromLocalPath(files[0].tempFileName);
            console.log(JSON.stringify(files));
        }
    }

    Component.onDestruction: Debug.print("Instance of preview1.qml destroyed!")
}
