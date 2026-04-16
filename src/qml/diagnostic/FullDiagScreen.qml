import QtQuick

Item {
    id: root
    anchors.fill: parent

    Rectangle { anchors.fill: parent; color: "white" }

    Flickable {
        anchors.fill: parent
        contentHeight: col.height + 60
        clip: true

        Column {
            id: col
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.top: parent.top
            anchors.topMargin: 30
            spacing: 10

            Text {
                text: "READMARKABLE DIAGNOSTICS"
                font.pixelSize: 32
                font.bold: true
                color: "black"
            }
            Text {
                text: "Version " + Qt.application.version + ", " + diagRunner.deviceModel
                font.pixelSize: 22
                color: "gray"
            }

            Item { width: 1; height: 16 }

            Text { text: "SUCCESS CRITERIA"; font.pixelSize: 26; font.bold: true; color: "black" }
            Rectangle { width: parent.width; height: 1; color: "black" }

            SCLine { label: "SC1: Display renders correctly"; passed: diagRunner.sc1_displayCorrect; auto_: true }
            SCLine { label: "SC2: Edge line at right edge"; passed: diagRunner.sc2_edgeLineOk; auto_: false }
            SCLine { label: "SC3: DPI > 150"; passed: diagRunner.sc3_dpiCorrect; auto_: true; detail: "DPI: " + diagRunner.dpi.toFixed(0) }
            SCLine { label: "SC4: Sleep/wake clean refresh"; passed: diagRunner.sc4_sleepWakeOk; auto_: false }
            SCLine { label: "SC5: Xochitl coexistence"; passed: diagRunner.sc5_coexistenceOk; auto_: false }

            Row {
                spacing: 10
                Text { text: "ALL CRITERIA:"; font.pixelSize: 28; font.bold: true; color: "black"; width: 340 }
                Text {
                    text: diagRunner.allCriteriaPassed ? "PASSED" : "INCOMPLETE"
                    font.pixelSize: 28
                    font.bold: true
                    color: diagRunner.allCriteriaPassed ? "black" : "red"
                }
            }

            Item { width: 1; height: 16 }

            Text { text: "FRAMEBUFFER"; font.pixelSize: 26; font.bold: true }
            Rectangle { width: parent.width; height: 1; color: "black" }
            DiagLine { label: "Device"; value: diagRunner.deviceModel }
            DiagLine { label: "FB ID"; value: diagRunner.fbId }
            DiagLine { label: "Resolution"; value: diagRunner.fbWidth + " x " + diagRunner.fbHeight }
            DiagLine { label: "Stride (bytes)"; value: "" + diagRunner.fbStride }
            DiagLine { label: "BPP"; value: "" + diagRunner.fbBitsPerPixel }
            DiagLine { label: "DPI"; value: diagRunner.dpi.toFixed(1) }
            DiagLine { label: "Stride == Width"; value: diagRunner.strideMatchesWidth ? "PASS" : "FAIL"; valueColor: diagRunner.strideMatchesWidth ? "black" : "red" }

            Item { width: 1; height: 16 }

            Text { text: "INPUT DEVICES"; font.pixelSize: 26; font.bold: true }
            Rectangle { width: parent.width; height: 1; color: "black" }
            DiagLine { label: "Pen device"; value: inputDiscovery.penFound ? inputDiscovery.penDevicePath : "NOT FOUND"; valueColor: inputDiscovery.penFound ? "black" : "red" }
            DiagLine { label: "Touch device"; value: inputDiscovery.touchFound ? inputDiscovery.touchDevicePath : "NOT FOUND"; valueColor: inputDiscovery.touchFound ? "black" : "red" }
            DiagLine { label: "Pen in range"; value: penReader.penInRange ? "YES" : "no" }
            DiagLine { label: "Pen pressure"; value: "" + penReader.pressure + " / " + penReader.maxPressure }
            DiagLine { label: "Pen X/Y"; value: penReader.penX.toFixed(0) + " / " + penReader.penY.toFixed(0) }
            DiagLine { label: "Pen tilt"; value: penReader.tiltX + ", " + penReader.tiltY }
            DiagLine { label: "Touch X, Y"; value: touchHandler.lastX.toFixed(0) + ", " + touchHandler.lastY.toFixed(0) }
            DiagLine { label: "Tap count"; value: "" + touchHandler.tapCount }

            Item { width: 1; height: 16 }

            Text { text: "LIFECYCLE"; font.pixelSize: 26; font.bold: true }
            Rectangle { width: parent.width; height: 1; color: "black" }
            DiagLine { label: "Xochitl"; value: xochitlGuard.xochitlStopped ? "stopped (" + xochitlGuard.strategy + ")" : "running" }
            DiagLine { label: "Awake"; value: lifecycleManager.awake ? "yes" : "sleeping" }
            DiagLine { label: "Wake count"; value: "" + lifecycleManager.wakeCount }

            Item { width: 1; height: 16 }

            Text { text: "TOUCH TEST"; font.pixelSize: 26; font.bold: true }
            Rectangle { width: parent.width; height: 1; color: "black" }

            Rectangle {
                width: parent.width
                height: 300
                color: "#f8f8f8"
                border.color: "black"
                border.width: 2

                Text {
                    anchors.centerIn: parent
                    text: "Tap here to test touch"
                    font.pixelSize: 24
                    color: "#ccc"
                    visible: touchHandler.tapCount === 0
                }

                Rectangle {
                    width: 16; height: 16; radius: 8
                    color: "black"
                    visible: touchHandler.tapCount > 0
                    x: Math.max(0, Math.min(parent.width - 16, touchHandler.lastX - parent.mapToGlobal(0, 0).x - 8))
                    y: Math.max(0, Math.min(parent.height - 16, touchHandler.lastY - parent.mapToGlobal(0, 0).y - 8))
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: function(mouse) {
                        var global = parent.mapToGlobal(mouse.x, mouse.y);
                        touchHandler.onTouchPressed(global.x, global.y);
                    }
                    onReleased: function(mouse) {
                        var global = parent.mapToGlobal(mouse.x, mouse.y);
                        touchHandler.onTouchReleased(global.x, global.y);
                    }
                }
            }

            Item { width: 1; height: 16 }

            Text { text: "VERIFICATION"; font.pixelSize: 26; font.bold: true }
            Rectangle { width: parent.width; height: 1; color: "black" }

            MarkButton { text: "Mark SC2 Edge Line OK"; checked: diagRunner.sc2_edgeLineOk; onClicked: diagRunner.markEdgeLineVerified(true) }
            MarkButton { text: "Mark SC4 Sleep/Wake OK"; checked: diagRunner.sc4_sleepWakeOk; onClicked: diagRunner.markSleepWakeTested(true) }
            MarkButton { text: "Mark SC5 Coexistence OK"; checked: diagRunner.sc5_coexistenceOk; onClicked: diagRunner.markCoexistenceTested(true) }
            MarkButton { text: "Mark Touch Working"; checked: diagRunner.touchWorking; onClicked: diagRunner.markTouchWorking(true) }
            MarkButton { text: "Export JSON"; checked: false; onClicked: { diagRunner.exportJson("/tmp/readmarkable-diag.json") } }

            Item { width: 1; height: 40 }

        }
    }

    Canvas {
        anchors.fill: parent
        z: 100
        onPaint: {
            var ctx = getContext("2d");
            var m = 10;
            ctx.strokeStyle = "black";
            ctx.lineWidth = 3;

            ctx.beginPath();
            ctx.moveTo(m, m); ctx.lineTo(width - m, m); ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(m, height - m); ctx.lineTo(width - m, height - m); ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(m, m); ctx.lineTo(m, height - m); ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(width - m, m); ctx.lineTo(width - m, height - m); ctx.stroke();
        }
    }

    Component.onCompleted: {
        diagRunner.runAll();
        inputDiscovery.discover();
    }

    Connections {
        target: inputDiscovery
        function onDevicesDiscovered() {
            if (inputDiscovery.penFound) {
                penReader.start(inputDiscovery.penDevicePath);
                diagRunner.markPenDiscovered(true);
                diagRunner.setPenAxisData(penReader.maxX, penReader.maxY, penReader.maxPressure);
            }
            diagRunner.setInputDevices(inputDiscovery.allDevices);
        }
    }

    component SCLine: Row {
        property string label: ""
        property bool passed: false
        property bool auto_: false
        property string detail: ""
        spacing: 10
        Text { text: label; font.pixelSize: 24; color: "black"; width: 500 }
        Text {
            text: passed ? "PASS" : (auto_ ? "FAIL" : "PENDING")
            font.pixelSize: 24
            font.bold: true
            color: passed ? "black" : (auto_ ? "red" : "gray")
        }
        Text { text: detail; font.pixelSize: 20; color: "gray"; visible: detail !== "" }
    }

    component MarkButton: Rectangle {
        property string text: ""
        property bool checked: false
        signal clicked()
        width: 500
        height: 52
        radius: 4
        color: checked ? "#e0e0e0" : "white"
        border.color: "black"
        border.width: 2
        Text {
            anchors.centerIn: parent
            text: parent.checked ? parent.text + " ✓" : parent.text
            font.pixelSize: 22
            color: "black"
        }
        MouseArea {
            anchors.fill: parent
            onClicked: parent.clicked()
        }
    }
}
