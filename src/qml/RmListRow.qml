import QtQuick 2.15
import ReadMarkable

Rectangle {
    id: root

    property string title:         ""
    property string subtitle:      ""
    property string trailingText:  ""
    property bool   selected:      false
    property bool   showSeparator: true

    signal clicked()

    height: Theme.touchTarget
    width:  parent ? parent.width : implicitWidth

    color: selected
               ? Theme.ink
               : (mouseArea.containsPress ? Theme.subtle : Theme.paper)

    readonly property color _fgPrimary:   selected ? Theme.paper : Theme.ink
    readonly property color _fgSecondary: selected ? Theme.paper : Theme.muted

    Column {
        id: titleBlock
        anchors.left:           parent.left
        anchors.leftMargin:     Theme.spaceLG
        anchors.right:          trailingLabel.visible ? trailingLabel.left : parent.right
        anchors.rightMargin:    Theme.spaceLG
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.spaceHair

        Text {
            text:    root.title
            visible: root.title !== ""
            color:   root._fgPrimary
            font.pixelSize: Theme.textListTitle
            font.family:    Theme.uiFontFamily
            elide: Text.ElideRight
            width: parent.width
        }

        Text {
            text:    root.subtitle
            visible: root.subtitle !== ""
            color:   root._fgSecondary
            font.pixelSize: Theme.textXS
            font.family:    Theme.uiFontFamily
            elide: Text.ElideRight
            width: parent.width
        }
    }

    Text {
        id: trailingLabel
        anchors.right:          parent.right
        anchors.rightMargin:    Theme.spaceLG
        anchors.verticalCenter: parent.verticalCenter
        visible: root.trailingText !== ""
        text:    root.trailingText
        color:   root._fgSecondary
        font.pixelSize: Theme.textXS
        font.family:    Theme.uiFontFamily
    }

    Rectangle {
        visible: root.showSeparator
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        height: Theme.borderWidth
        color:  Theme.separator
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked:    root.clicked()
    }
}
