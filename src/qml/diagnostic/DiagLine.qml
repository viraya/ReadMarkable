import QtQuick

Row {
    property string label: ""
    property string value: ""
    property color valueColor: "black"
    property bool labelBold: false

    spacing: 20

    Text {
        text: label + ":"
        font.pixelSize: 28
        font.bold: labelBold
        color: "black"
        width: 300
    }
    Text {
        text: value
        font.pixelSize: 28
        font.bold: labelBold
        color: valueColor
    }
}
