import QtQuick 2.15
import ReadMarkable

Rectangle {
    id: root

    property string title:    ""
    property string subtitle: ""
    property bool   twoRow:   false
    default property alias actions: actionsRow.data

    height: twoRow ? Theme.toolbarHeight : Theme.panelHeaderHeight
    color:  Theme.paper

    Column {
        id: titleBlock
        anchors.left:           parent.left
        anchors.leftMargin:     Theme.spaceLG
        anchors.verticalCenter: root.twoRow ? undefined : parent.verticalCenter
        anchors.top:            root.twoRow ? parent.top : undefined
        anchors.topMargin:      root.twoRow ? Theme.spaceXS : 0
        spacing: Theme.spaceHair

        Text {
            text:    root.title
            visible: root.title !== ""
            color:   Theme.ink
            font.pixelSize: Theme.textLG
            font.bold:      true
            font.family:    Theme.uiFontFamily
            elide: Text.ElideRight
        }

        Text {
            text:    root.subtitle
            visible: root.subtitle !== ""
            color:   Theme.muted
            font.pixelSize: Theme.textSM
            font.family:    Theme.uiFontFamily
            elide: Text.ElideRight
        }
    }

    Row {
        id: actionsRow
        anchors.right:          parent.right
        anchors.rightMargin:    Theme.spaceSM
        anchors.verticalCenter: root.twoRow ? undefined : parent.verticalCenter
        anchors.top:            root.twoRow ? parent.top : undefined
        anchors.topMargin:      root.twoRow ? Theme.spaceXS : 0
        spacing: Theme.spaceXS
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        height: Theme.borderWidth
        color:  Theme.separator
    }
}
