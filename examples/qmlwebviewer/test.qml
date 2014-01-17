import QtQuick 1.0

Item {
    width: 1024
    height: 768
    focus: true

    Rectangle {
        id: rect
        width: 111
        height: 200
        color: "gray"


        /*
        SequentialAnimation on x {
            loops: Animation.Infinite
            PropertyAnimation { to: 900; duration: 20000; easing.type: Easing.InOutQuad }
            PropertyAnimation { to: 100; duration: 20000; easing.type: Easing.InOutQuad }
        }

        SequentialAnimation on y {
            loops: Animation.Infinite
            PropertyAnimation { to: 100; duration: 23000; easing.type: Easing.InOutQuad }
            PropertyAnimation { to: 500; duration: 23000; easing.type: Easing.InOutQuad }
        }
        */
    }
/*
    Image {
        id: image

        x: 1000
        y: 1000

        width: 1000
        height: 1000

        source: "http://localhost/~wincent/mona-lisa.png"
//        source: "http://192.168.56.1/~wincent/mona-lisa.png"
        fillMode: Image.PreserveAspectFit

        SequentialAnimation on x {
            loops: Animation.Infinite
            PropertyAnimation { to: 900; duration: 10000; easing.type: Easing.InOutQuad }
            PropertyAnimation { to: 100; duration: 10000; easing.type: Easing.InOutQuad }
        }

        SequentialAnimation on y {
            loops: Animation.Infinite
            PropertyAnimation { to: 100; duration: 10000; easing.type: Easing.InOutQuad }
            PropertyAnimation { to: 500; duration: 10000; easing.type: Easing.InOutQuad }
        }

    }
*/
    /*
    TextEdit {
        id: edit

        width: parent.width / 2
        height: 20

        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10

        focus: true

        onTextChanged:
        {
            console.log("Text changed to \"" + edit.text + "\"");
        }
    }
    */



    MouseArea {
        anchors.fill: parent

        property variant startX
        property variant startY

        property variant mousePressX
        property variant mousePressY

        onPressed:
        {
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

    Keys.onPressed:
    {
        var diffX = 0;
        var diffY = 0;

        if (event.key == Qt.Key_Up)
            diffY -= 10;
        else if (event.key == Qt.Key_Down)
            diffY += 10;
        else if (event.key == Qt.Key_Left)
            diffX -= 10;
        else if (event.key == Qt.Key_Right)
            diffX += 10;

        var multiplier = 1;
        if (event.modifiers & Qt.ShiftModifier)
            multiplier *= 4;

        if (event.modifiers & Qt.ControlModifier)
            multiplier *= 15;

        rect.x = rect.x + diffX * multiplier;
        rect.y = rect.y + diffY * multiplier;
    }

    Connections {
        target: commandBridge

        onCommandReceived:
        {
//            console.log("command received: " + command + " for session with ID: " + id);
            if (command === "switchRectColor")
            {
                rect.color = (rect.color == "#808080") ? "red" : "gray";
                commandBridge.response = "switched";
            }
            else
            {
                commandBridge.response = "invalid command";
            }
        }
    }
}
