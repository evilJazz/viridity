import QtQuick 1.0

Item {
    width: 1024
    height: 768

    Rectangle {
        width: 111
        height: 200
        color: "red"

        SequentialAnimation on x {
            loops: Animation.Infinite
            PropertyAnimation { to: 900; duration: 2000; easing.type: Easing.InOutQuad }
            PropertyAnimation { to: 100; duration: 2000; easing.type: Easing.InOutQuad }
        }

        SequentialAnimation on y {
            loops: Animation.Infinite
            PropertyAnimation { to: 100; duration: 2300; easing.type: Easing.InOutQuad }
            PropertyAnimation { to: 500; duration: 2300; easing.type: Easing.InOutQuad }
        }
    }
}
