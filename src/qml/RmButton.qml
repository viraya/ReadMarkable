import QtQuick 2.15
import ReadMarkable

Rectangle {
    id: root

    property string label:      ""
    property string iconSource: ""
    property bool   isActive:   false
    property int    paddingH:   compact ? Theme.spaceSM : Theme.spaceMD
    property int    minWidth:   compact ? Theme.toolbarButtonHeight : Theme.touchTarget

    property bool   compact:    false

    property bool   flat:       false

    signal clicked()

    height: compact ? Theme.toolbarButtonHeight : Theme.touchTarget
    width:  Math.max(minWidth, contentRow.implicitWidth + paddingH * 2)

    border.color: flat ? "transparent" : Theme.panelBorder
    border.width: flat ? 0 : Theme.borderWidth
    radius:       Theme.borderRadius
    opacity:      enabled ? 1.0 : 0.4

    readonly property bool _inverted: mouseArea.containsPress || isActive
    color: flat
               ? (_inverted ? Theme.subtle : "transparent")
               : (_inverted ? Theme.ink : Theme.paper)

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: Theme.spaceXS

        Image {
            id: iconImage
            source:  root.iconSource
            visible: root.iconSource !== ""
            width:   Theme.iconButtonSm
            height:  Theme.iconButtonSm
            fillMode: Image.PreserveAspectFit
            smooth: false

        }

        Text {
            id: labelText
            text: root.label
            visible: root.label !== ""

            color: (root.flat || !root._inverted) ? Theme.ink : Theme.paper
            font.pixelSize: root.compact ? Theme.textSM : Theme.textToolbar
            font.family: Theme.uiFontFamily
            verticalAlignment: Text.AlignVCenter
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.enabled
        onClicked: root.clicked()
    }
}
