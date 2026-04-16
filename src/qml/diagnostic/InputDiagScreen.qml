import QtQuick

Item {
    id: inputDiag
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: "white"
    }

    Text {
        id: title
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Input Diagnostics"
        font.pixelSize: 36
        font.bold: true
        color: "black"
    }

    Column {
        id: deviceInfo
        anchors.top: title.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 30
        spacing: 10

        DiagLine { label: "Pen device"; value: inputDiscovery.penFound ? inputDiscovery.penDevicePath : "NOT FOUND"; valueColor: inputDiscovery.penFound ? "black" : "red" }
        DiagLine { label: "Touch device"; value: inputDiscovery.touchFound ? inputDiscovery.touchDevicePath : "NOT FOUND"; valueColor: inputDiscovery.touchFound ? "black" : "red" }

        Item { width: 1; height: 8 }

        DiagLine { label: "Pen in range"; value: penReader.penInRange ? "YES" : "no" }
        DiagLine { label: "Pen down"; value: penReader.penDown ? "YES (pressure: " + penReader.pressure + ")" : "no" }
        DiagLine { label: "Pen X"; value: "" + penReader.penX.toFixed(0) + " / " + penReader.maxX }
        DiagLine { label: "Pen Y"; value: "" + penReader.penY.toFixed(0) + " / " + penReader.maxY }
        DiagLine { label: "Pen pressure"; value: "" + penReader.pressure + " / " + penReader.maxPressure }
        DiagLine { label: "Pen tilt"; value: penReader.tiltX + ", " + penReader.tiltY }

        Item { width: 1; height: 8 }

        DiagLine { label: "Touch X, Y"; value: touchHandler.lastX.toFixed(0) + ", " + touchHandler.lastY.toFixed(0) }
        DiagLine { label: "Tap count"; value: "" + touchHandler.tapCount }
        DiagLine { label: "Last gesture"; value: touchHandler.lastGesture || "none" }
    }

    Rectangle {
        id: pressureBar
        anchors.right: parent.right
        anchors.rightMargin: 30
        anchors.top: deviceInfo.top
        width: 30
        height: 400
        color: "#eee"
        border.color: "black"
        border.width: 1

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: penReader.maxPressure > 0 ? (parent.height * penReader.pressure / penReader.maxPressure) : 0
            color: "black"
        }

        Text {
            anchors.bottom: parent.top
            anchors.bottomMargin: 4
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Pressure"
            font.pixelSize: 16
            color: "black"
        }
    }

    Rectangle {
        id: touchArea
        anchors.top: deviceInfo.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.right: pressureBar.left
        anchors.rightMargin: 30
        height: parent.height - y - 80
        color: "#f8f8f8"
        border.color: "black"
        border.width: 2

        Text {
            anchors.centerIn: parent
            text: "Touch test area\nTap or swipe here"
            font.pixelSize: 24
            color: "#ccc"
            horizontalAlignment: Text.AlignHCenter
            visible: touchHandler.tapCount === 0
        }

        Rectangle {
            id: tapMarker
            width: 20
            height: 20
            radius: 10
            color: "black"
            visible: touchHandler.tapCount > 0
            x: touchHandler.lastX - touchArea.x - 10
            y: touchHandler.lastY - touchArea.y - 10
        }

        MouseArea {
            anchors.fill: parent
            onPressed: function(mouse) {
                touchHandler.onTouchPressed(mouse.x + touchArea.x, mouse.y + touchArea.y);
            }
            onReleased: function(mouse) {
                touchHandler.onTouchReleased(mouse.x + touchArea.x, mouse.y + touchArea.y);
            }
            onPositionChanged: function(mouse) {
                touchHandler.onTouchMoved(mouse.x + touchArea.x, mouse.y + touchArea.y);
            }
        }
    }

    Text { x: 30; y: touchArea.y + 4; text: "(0,0)"; font.pixelSize: 16; color: "gray" }
    Text { anchors.right: pressureBar.left; anchors.rightMargin: 34; y: touchArea.y + 4; text: "(" + touchArea.width + ",0)"; font.pixelSize: 16; color: "gray"; horizontalAlignment: Text.AlignRight }

    Text {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Swipe left to go back to DiagScreen"
        font.pixelSize: 18
        color: "gray"
    }

    Component.onCompleted: {
        inputDiscovery.discover();

    }

    Connections {
        target: inputDiscovery
        function onDevicesDiscovered() {
            if (inputDiscovery.penFound) {
                penReader.start(inputDiscovery.penDevicePath);
            }
        }
    }
}
