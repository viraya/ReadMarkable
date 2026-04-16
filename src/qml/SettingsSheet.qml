import QtQuick 2.15
import QtQuick.Controls 2.15
import ReadMarkable

Drawer {
    id: settingsSheet

    edge:   Qt.BottomEdge
    width:  parent ? parent.width : 0
    height: parent ? Math.floor(parent.height * 0.55) : 0
    modal:  true
    dragMargin: 0
    focus:  true

    enter: Transition {
        NumberAnimation {
            property:    "position"
            from:        0.0
            to:          1.0
            duration:    Theme.animDuration
            easing.type: Easing.Linear
        }
    }
    exit: Transition {
        NumberAnimation {
            property:    "position"
            from:        1.0
            to:          0.0
            duration:    Theme.animDuration
            easing.type: Easing.Linear
        }
    }

    Overlay.modal: Rectangle { color: "transparent" }

    background: Rectangle {
        color: Theme.paper

        Rectangle {
            anchors.top:   parent.top
            anchors.left:  parent.left
            anchors.right: parent.right
            height: Theme.borderWidth
            color:  Theme.ink
        }
    }

    contentItem: RmPanel {
        id: settingsPanel

        header: RmHeader {
            title: "Settings"

            RmButton {
                compact: true
                label: "Close"
                onClicked: settingsSheet.close()
            }
        }

        Flickable {
            anchors.fill: parent
            contentHeight: settingsColumn.implicitHeight
            clip: true

            Column {
                id: settingsColumn
                width:   parent.width
                spacing: Theme.spaceMD

                Row {
                    spacing: Theme.spaceXXS
                    Text {
                        text: "Font"
                        font.pixelSize: Theme.textMD
                        font.bold: true
                        font.family: Theme.uiFontFamily
                        color: Theme.ink
                    }
                    Rectangle {
                        width:  Theme.spaceXXS
                        height: Theme.spaceXXS
                        radius: Theme.spaceMicro
                        color:  Theme.ink
                        visible: settingsController ? settingsController.fontFaceOverridden : false
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                FontPicker {
                    width: parent.width
                }

                Row {
                    spacing: Theme.spaceXXS
                    Text {
                        text: "Size"
                        font.pixelSize: Theme.textMD
                        font.bold: true
                        font.family: Theme.uiFontFamily
                        color: Theme.ink
                    }
                    Rectangle {
                        width:  Theme.spaceXXS
                        height: Theme.spaceXXS
                        radius: Theme.spaceMicro
                        color:  Theme.ink
                        visible: settingsController ? settingsController.fontSizeOverridden : false
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    spacing: Theme.spaceMD

                    RmButton {
                        label: "−"
                        onClicked: {
                            if (settingsController)
                                settingsController.fontSize = settingsController.fontSize - 1
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: settingsController ? (settingsController.fontSize + " pt") : "14 pt"
                        font.pixelSize: Theme.textLG
                        font.family: Theme.uiFontFamily
                        color: Theme.ink
                        width: Theme.numericFieldWidth
                        horizontalAlignment: Text.AlignHCenter
                    }

                    RmButton {
                        label: "+"
                        onClicked: {
                            if (settingsController)
                                settingsController.fontSize = settingsController.fontSize + 1
                        }
                    }
                }

                Row {
                    spacing: Theme.spaceXXS
                    Text {
                        text: "Margins"
                        font.pixelSize: Theme.textMD
                        font.bold: true
                        font.family: Theme.uiFontFamily
                        color: Theme.ink
                    }
                    Rectangle {
                        width:  Theme.spaceXXS
                        height: Theme.spaceXXS
                        radius: Theme.spaceMicro
                        color:  Theme.ink
                        visible: settingsController ? settingsController.marginPresetOverridden : false
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    spacing: Theme.spaceXS
                    Repeater {
                        model: ["Narrow", "Normal", "Wide"]
                        delegate: RmButton {
                            label: modelData
                            isActive: (settingsController && settingsController.marginPreset === index)
                            onClicked: {
                                if (settingsController)
                                    settingsController.marginPreset = index
                            }
                        }
                    }
                }

                Row {
                    spacing: Theme.spaceXXS
                    Text {
                        text: "Line Spacing"
                        font.pixelSize: Theme.textMD
                        font.bold: true
                        font.family: Theme.uiFontFamily
                        color: Theme.ink
                    }
                    Rectangle {
                        width:  Theme.spaceXXS
                        height: Theme.spaceXXS
                        radius: Theme.spaceMicro
                        color:  Theme.ink
                        visible: settingsController ? settingsController.lineSpacingOverridden : false
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Row {
                    spacing: Theme.spaceXS
                    width: parent.width

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "80%"
                        font.pixelSize: Theme.textXS
                        font.family: Theme.uiFontFamily
                        color: Theme.muted
                    }

                    Slider {
                        id: lineSpacingSlider
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - Theme.numericFieldWidth
                        from:      80
                        to:        200
                        stepSize:  10
                        value:     settingsController ? settingsController.lineSpacing : 100

                        onMoved: {
                            if (settingsController)
                                settingsController.lineSpacing = value
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "200%"
                        font.pixelSize: Theme.textXS
                        font.family: Theme.uiFontFamily
                        color: Theme.muted
                    }
                }

                Text {
                    text: settingsController ? ("Line spacing: " + settingsController.lineSpacing + "%") : ""
                    font.pixelSize: Theme.textXS
                    font.family: Theme.uiFontFamily
                    color: Theme.muted
                }

                RmButton {
                    width: parent.width
                    label: "Reset to Defaults"
                    visible: settingsController ? settingsController.hasAnyOverride : false
                    onClicked: {
                        if (settingsController)
                            settingsController.resetToGlobalDefaults()
                    }
                }

            }
        }
    }
}
