import QtQuick 1.1
import KCL 1.0
import Viridity 1.0

FocusScope {
    id: scene

    width: 1024
    height: 768
    focus: true

    ViridityDataBridge {
        id: dataBridge
        session: currentSession
        targetId: "sceneData1"

        function onNewCommandReceived(input)
        {
            testTimer1.start();
            console.log("input1: " + input);
            return { action: "blah111", test: 1243234.3423 };
        }
    }

    Component.onDestruction: console.log("GOODBYE WORLD!")

    Rectangle {
        anchors.fill: parent
        color: "lightgray"
        anchors.margins: 2
    }

    Image {
        id: image
        visible: true

        source: "qrc:/testimages/lena.png"
        fillMode: Image.PreserveAspectFit

        SequentialAnimation on x {
            running: imageAnimationEnabled.checked
            loops: Animation.Infinite
            PropertyAnimation { to: 900; duration: 4000; easing.type: Easing.InOutQuad }
            PropertyAnimation { to: 50; duration: 4000; easing.type: Easing.InOutQuad }
        }

        SequentialAnimation on y {
            running: imageAnimationEnabled.checked
            loops: Animation.Infinite
            PropertyAnimation { to: 50; duration: 4300; easing.type: Easing.InOutQuad }
            PropertyAnimation { to: 500; duration: 4300; easing.type: Easing.InOutQuad }
        }

    }

    Flickable {
        clip: true

        z: 400000
        x: 200
        y: 200
        height: 400
        width: 400

        contentWidth: contentImage.width
        contentHeight: contentImage.height
        Image {
            id: contentImage
            source: "qrc:/testimages/lena.png"
        }
    }

    Rectangle {
        id: rect
        width: 111
        height: 200
        color: "gray"

        x: 80
        y: 200

        objectName: "rect"

        SequentialAnimation on x {
            running: rectAnimationEnabled.checked
            loops: Animation.Infinite
            PropertyAnimation { to: 900; duration: 2000; easing.type: Easing.InOutQuad }
            PropertyAnimation { to: 100; duration: 2000; easing.type: Easing.InOutQuad }
        }

        SequentialAnimation on y {
            running: rectAnimationEnabled.checked
            loops: Animation.Infinite
            PropertyAnimation { to: 100; duration: 2300; easing.type: Easing.InOutQuad }
            PropertyAnimation { to: 500; duration: 2300; easing.type: Easing.InOutQuad }
        }
    }


    Rectangle {
        id: rect2
        width: 111
        height: 200
        color: "gray"

        objectName: "rect2"

        x: 200
        y: 200
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent

        preventStealing: true

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

    Column {
        x: 10
        y: 110

        spacing: 5

        CheckBox {
            id: rectAnimationEnabled
            text: "Animate rect"
        }

        CheckBox {
            id: imageAnimationEnabled
            text: "Animate image"
        }

        CheckBox {
            id: dummieCheckbox
            text: "Dummie"
        }

        CheckBox {
            id: alertCheckbox
            text: "Alert"

            onCheckedChanged:
            {
                dataBridge.sendData(["Hello World! " + checked, checked], function (response, displayId)
                {
                    console.log("Response from display " + displayId + ": " + response);
                });
            }
        }
    }

    Rectangle {
        width: parent.width / 2
        height: 90

        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10

        border.color: "black"
        border.width: 1

        smooth: true

        FocusScope {
            anchors.fill: parent

            TextEdit {
                id: edit

                anchors.fill: parent
                anchors.margins: 2

                focus: true
                selectByMouse: true

                text: "Hello World!"

                onTextChanged:
                {
                    console.log("Text changed to \"" + edit.text + "\"");
                }
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: scene.width + " x " + scene.height
    }
}
