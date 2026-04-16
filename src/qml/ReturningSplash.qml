import QtQuick 2.15
import ReadMarkable

Item {
    id: root
    anchors.fill: parent
    visible: false
    z: 9999

    Rectangle {
        anchors.fill: parent
        color:        Theme.paper
    }

    MouseArea {
        anchors.fill: parent
    }

    Text {
        anchors.centerIn: parent
        color:            Theme.ink
        font.pixelSize:   Theme.textMD
        font.family:      Theme.uiFontFamily
        horizontalAlignment: Text.AlignHCenter
        text:             "Returning to reMarkable…"
    }

    function show() {
        visible = true
    }
}
