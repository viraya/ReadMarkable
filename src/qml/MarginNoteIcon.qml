import QtQuick 2.15
import ReadMarkable

Item {
    id: marginNoteIcon
    width: Theme.marginNoteIconSize
    height: Theme.marginNoteIconSize

    property int    noteId: 0
    property var    strokeData: null
    property string paragraphXPointer: ""
    property int    paragraphY: 0

    signal iconTapped(int noteId, var strokeData, string paragraphXPointer, int paragraphY)

    Rectangle {
        anchors.fill: parent
        color:        iconArea.containsPress ? Theme.ink : Theme.paper
        border.color: Theme.ink
        border.width: 1

        Text {
            anchors.centerIn: parent
            text:        "N"
            font.pixelSize: Theme.textXS
            font.family: Theme.uiFontFamily
            font.bold:   true
            color:       iconArea.containsPress ? Theme.paper : Theme.ink
        }
    }

    MouseArea {
        id: iconArea
        anchors.fill: parent
        onClicked: {
            marginNoteIcon.iconTapped(
                marginNoteIcon.noteId,
                marginNoteIcon.strokeData,
                marginNoteIcon.paragraphXPointer,
                marginNoteIcon.paragraphY)
        }
    }
}
