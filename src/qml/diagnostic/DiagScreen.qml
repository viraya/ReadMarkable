import QtQuick

Item {
    id: diagScreen
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: "white"
    }

    Text {
        id: title
        anchors.top: parent.top
        anchors.topMargin: 40
        anchors.horizontalCenter: parent.horizontalCenter
        text: "ReadMarkable Diagnostics"
        font.pixelSize: 40
        font.bold: true
        color: "black"
    }

    Column {
        id: resultsColumn
        anchors.top: title.bottom
        anchors.topMargin: 30
        anchors.left: parent.left
        anchors.leftMargin: 40
        spacing: 16

        DiagLine { label: "Device"; value: diagRunner.deviceModel }
        DiagLine { label: "FB ID"; value: diagRunner.fbId }
        DiagLine { label: "Resolution"; value: diagRunner.fbWidth + " x " + diagRunner.fbHeight }
        DiagLine { label: "Stride (bytes)"; value: "" + diagRunner.fbStride }
        DiagLine { label: "Stride (pixels)"; value: "" + diagRunner.fbStridePixels }
        DiagLine { label: "BPP"; value: "" + diagRunner.fbBitsPerPixel }
        DiagLine {
            label: "Stride == Width"
            value: diagRunner.strideMatchesWidth ? "PASS (no padding)" : "FAIL (stride mismatch!)"
            valueColor: diagRunner.strideMatchesWidth ? "black" : "red"
        }

        Item { width: 1; height: 12 }
        DiagLine { label: "DPI"; value: diagRunner.dpi.toFixed(1) }
        DiagLine {
            label: "DPI > 150"
            value: diagRunner.dpiAbove150 ? "PASS" : "FAIL"
            valueColor: diagRunner.dpiAbove150 ? "black" : "red"
        }

        Item { width: 1; height: 12 }
        DiagLine {
            label: "ALL CHECKS"
            value: diagRunner.allPassed ? "PASSED" : "FAILED"
            valueColor: diagRunner.allPassed ? "black" : "red"
            labelBold: true
        }

        Item { width: 1; height: 12 }
        DiagLine { label: "Xochitl"; value: xochitlGuard.xochitlStopped ? "stopped (" + xochitlGuard.strategy + ")" : "running" }
        DiagLine { label: "Awake"; value: lifecycleManager.awake ? "yes" : "sleeping" }
        DiagLine { label: "Wake count"; value: "" + lifecycleManager.wakeCount }
    }

    Canvas {
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            var m = 10;
            ctx.strokeStyle = "black";
            ctx.lineWidth = 3;

            ctx.beginPath();
            ctx.moveTo(m, m);
            ctx.lineTo(width - m, m);
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(m, height - m);
            ctx.lineTo(width - m, height - m);
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(m, m);
            ctx.lineTo(m, height - m);
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(width - m, m);
            ctx.lineTo(width - m, height - m);
            ctx.stroke();

            var cx = width / 2;
            var cy = height / 2;
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(cx - 40, cy);
            ctx.lineTo(cx + 40, cy);
            ctx.moveTo(cx, cy - 40);
            ctx.lineTo(cx, cy + 40);
            ctx.stroke();
        }
    }

    Text {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Results: /tmp/readmarkable-diag.json"
        font.pixelSize: 20
        color: "gray"
    }

    Component.onCompleted: {
        diagRunner.runAll();
    }
}
