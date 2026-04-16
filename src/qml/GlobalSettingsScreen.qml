import QtQuick 2.15
import QtQuick.Controls 2.15
import ReadMarkable

Item {
    id: globalSettingsScreen
    anchors.fill: parent

    signal goBack()

    Component.onCompleted: {
        if (settingsController)
            settingsController.enterGlobalSettingsMode()
    }

    Component.onDestruction: {
        if (settingsController)
            settingsController.exitGlobalSettingsMode()
    }

    onGoBack: {
        if (settingsController)
            settingsController.exitGlobalSettingsMode()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.paper
    }

    Item {
        id: header
        anchors {
            left:  parent.left
            right: parent.right
            top:   parent.top
        }
        height: Theme.toolbarHeight

        Item {
            id: backButton
            anchors {
                left:           parent.left
                verticalCenter: parent.verticalCenter
                leftMargin:     Theme.spaceLG
            }
            width:  chevron.width + backLabel.implicitWidth + Theme.spaceLG * 2
            height: Theme.touchTarget

            Canvas {
                id: chevron
                width:  16
                height: Theme.spaceMD
                anchors.verticalCenter: parent.verticalCenter

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = backArea.containsPress ? Theme.paper : Theme.ink
                    ctx.lineWidth   = 2
                    ctx.lineCap     = "round"
                    ctx.lineJoin    = "round"
                    ctx.beginPath()
                    ctx.moveTo(width - 2, 3)
                    ctx.lineTo(3,          height / 2)
                    ctx.lineTo(width - 2,  height - 3)
                    ctx.stroke()
                }

                Connections {
                    target: backArea
                    function onContainsPressChanged() { chevron.requestPaint() }
                }
            }

            Text {
                id: backLabel
                anchors {
                    left:           chevron.right
                    leftMargin:     6
                    verticalCenter: parent.verticalCenter
                }
                text:            "Back"
                font.pixelSize:  Theme.textMD
                font.family:     Theme.uiFontFamily
                color:           backArea.containsPress ? Theme.paper : Theme.ink
            }

            Rectangle {
                anchors.fill: parent
                color:        backArea.containsPress ? Theme.ink : "transparent"
                radius:       Theme.borderRadius
                z:            -1
            }

            MouseArea {
                id:          backArea
                anchors.fill: parent
                onClicked:   globalSettingsScreen.goBack()
            }
        }

        Text {
            anchors.centerIn: parent
            text:            "Reading Defaults"
            font.pixelSize:  Theme.textLG
            font.bold:       true
            font.family:     Theme.uiFontFamily
            color:           Theme.ink
        }

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 1
            color:  Theme.separator
        }
    }

    Flickable {
        id: bodyFlickable
        anchors {
            left:   parent.left
            right:  parent.right
            top:    header.bottom
            bottom: parent.bottom
        }
        contentHeight: bodyColumn.implicitHeight + Theme.spaceLG
        clip:          true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: bodyColumn
            width:   parent.width
            spacing: Theme.spaceSM

            Item { width: 1; height: Theme.spaceSM }

            Column {
                id:      controlsColumn
                width:   parent.width
                spacing: Theme.spaceMD

                anchors.leftMargin:  Theme.spaceLG
                anchors.rightMargin: Theme.spaceLG

                Item {
                    width:  parent.width
                    height: fontSectionHeader.height

                    Row {
                        id: fontSectionHeader
                        anchors {
                            left:       parent.left
                            leftMargin: Theme.spaceLG
                        }
                        spacing: Theme.spaceXXS
                        Text {
                            text:           "Font"
                            font.pixelSize: Theme.textMD
                            font.bold:      true
                            font.family:    Theme.uiFontFamily
                            color:          Theme.ink
                        }
                    }
                }
                Item {
                    width:  parent.width
                    height: globalFontPicker.height + Theme.spaceXS
                    FontPicker {
                        id: globalFontPicker
                        anchors {
                            left:        parent.left
                            right:       parent.right
                            leftMargin:  Theme.spaceLG
                            rightMargin: Theme.spaceLG
                        }
                        width: parent.width - Theme.spaceLG * 2
                    }
                }

                Item {
                    width:  parent.width
                    height: sizeSectionHeader.height

                    Row {
                        id: sizeSectionHeader
                        anchors {
                            left:       parent.left
                            leftMargin: Theme.spaceLG
                        }
                        spacing: Theme.spaceXXS
                        Text {
                            text:           "Size"
                            font.pixelSize: Theme.textMD
                            font.bold:      true
                            font.family:    Theme.uiFontFamily
                            color:          Theme.ink
                        }
                    }
                }
                Item {
                    width:  parent.width
                    height: sizeStepper.height

                    Row {
                        id: sizeStepper
                        anchors {
                            left:       parent.left
                            leftMargin: Theme.spaceLG
                        }
                        spacing: Theme.spaceMD

                        Rectangle {
                            width: Theme.touchTarget; height: Theme.touchTarget
                            radius: Theme.borderRadius
                            color:  decrementArea.containsPress ? Theme.ink : Theme.paper
                            border.color: Theme.ink; border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text:            "−"
                                font.pixelSize:  Theme.textLG
                                font.family:     Theme.uiFontFamily
                                color:           decrementArea.containsPress ? Theme.paper : Theme.ink
                            }
                            MouseArea {
                                id: decrementArea
                                anchors.fill: parent
                                onClicked: {
                                    if (settingsController)
                                        settingsController.fontSize = settingsController.fontSize - 1
                                }
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text:           settingsController ? (settingsController.fontSize + " pt") : "14 pt"
                            font.pixelSize: Theme.textLG
                            font.family:    Theme.uiFontFamily
                            color:          Theme.ink
                            width:          100
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Rectangle {
                            width: Theme.touchTarget; height: Theme.touchTarget
                            radius: Theme.borderRadius
                            color:  incrementArea.containsPress ? Theme.ink : Theme.paper
                            border.color: Theme.ink; border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text:            "+"
                                font.pixelSize:  Theme.textLG
                                font.family:     Theme.uiFontFamily
                                color:           incrementArea.containsPress ? Theme.paper : Theme.ink
                            }
                            MouseArea {
                                id: incrementArea
                                anchors.fill: parent
                                onClicked: {
                                    if (settingsController)
                                        settingsController.fontSize = settingsController.fontSize + 1
                                }
                            }
                        }
                    }
                }

                Item {
                    width:  parent.width
                    height: marginSectionHeader.height

                    Row {
                        id: marginSectionHeader
                        anchors {
                            left:       parent.left
                            leftMargin: Theme.spaceLG
                        }
                        spacing: Theme.spaceXXS
                        Text {
                            text:           "Margins"
                            font.pixelSize: Theme.textMD
                            font.bold:      true
                            font.family:    Theme.uiFontFamily
                            color:          Theme.ink
                        }
                    }
                }
                Item {
                    width:  parent.width
                    height: marginPresets.height

                    Row {
                        id: marginPresets
                        anchors {
                            left:       parent.left
                            leftMargin: Theme.spaceLG
                        }
                        spacing: 12
                        Repeater {
                            model: ["Narrow", "Normal", "Wide"]
                            delegate: Rectangle {
                                width:  marginLabel.implicitWidth + 32
                                height: Theme.touchTarget
                                radius: Theme.borderRadius
                                color:  (settingsController && settingsController.marginPreset === index)
                                        ? Theme.ink : Theme.paper
                                border.color: Theme.ink; border.width: 1
                                Text {
                                    id: marginLabel
                                    anchors.centerIn: parent
                                    text:           modelData
                                    font.pixelSize: Theme.textSM
                                    font.family:    Theme.uiFontFamily
                                    color: (settingsController && settingsController.marginPreset === index)
                                           ? Theme.paper : Theme.ink
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (settingsController)
                                            settingsController.marginPreset = index
                                    }
                                }
                            }
                        }
                    }
                }

                Item {
                    width:  parent.width
                    height: spacingSectionHeader.height

                    Row {
                        id: spacingSectionHeader
                        anchors {
                            left:       parent.left
                            leftMargin: Theme.spaceLG
                        }
                        spacing: Theme.spaceXXS
                        Text {
                            text:           "Line Spacing"
                            font.pixelSize: Theme.textMD
                            font.bold:      true
                            font.family:    Theme.uiFontFamily
                            color:          Theme.ink
                        }
                    }
                }
                Item {
                    width:  parent.width
                    height: spacingRow.height

                    Row {
                        id: spacingRow
                        anchors {
                            left:        parent.left
                            right:       parent.right
                            leftMargin:  Theme.spaceLG
                            rightMargin: Theme.spaceLG
                        }
                        spacing: 12

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text:           "80%"
                            font.pixelSize: Theme.textXS
                            font.family:    Theme.uiFontFamily
                            color:          Theme.dimmed
                        }

                        Slider {
                            id: lineSpacingSlider
                            anchors.verticalCenter: parent.verticalCenter
                            width:    parent.width - 100
                            from:     80
                            to:       200
                            stepSize: 10
                            value:    settingsController ? settingsController.lineSpacing : 100

                            onMoved: {
                                if (settingsController)
                                    settingsController.lineSpacing = value
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text:           "200%"
                            font.pixelSize: Theme.textXS
                            font.family:    Theme.uiFontFamily
                            color:          Theme.dimmed
                        }
                    }
                }

                Item {
                    width:  parent.width
                    height: spacingValueLabel.height + Theme.spaceXS
                    Text {
                        id: spacingValueLabel
                        anchors {
                            left:       parent.left
                            leftMargin: Theme.spaceLG
                        }
                        text:           settingsController ? ("Line spacing: " + settingsController.lineSpacing + "%") : ""
                        font.pixelSize: Theme.textXS
                        font.family:    Theme.uiFontFamily
                        color:          Theme.dimmed
                    }
                }

                Item {
                    width:  parent.width
                    height: autoSaveNote.height + Theme.spaceLG * 2

                    Text {
                        id: autoSaveNote
                        anchors.centerIn: parent
                        text:            "Changes are saved automatically"
                        font.pixelSize:  Theme.textXS
                        font.family:     Theme.uiFontFamily
                        color:           Theme.muted
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

            }

            Rectangle {
                id: previewBox
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                height: Math.min(previewText.implicitHeight + Theme.spaceMD * 2, 400)
                clip: true
                border.color: Theme.ink
                border.width: Theme.borderWidth
                color:        Theme.paper

                Text {
                    id: previewText
                    anchors {
                        left:        parent.left
                        right:       parent.right
                        top:         parent.top
                        leftMargin:  Theme.spaceMD
                        rightMargin: Theme.spaceMD
                        topMargin:   Theme.spaceMD
                    }
                    text: "The quick brown fox jumps over the lazy dog. Reading should feel like turning pages of paper, not using software."
                    font.family:    settingsController ? settingsController.fontFace : "Noto Serif"
                    font.pixelSize: settingsController ? settingsController.fontSize : 14
                    lineHeight:     settingsController ? (settingsController.lineSpacing / 100.0) : 1.0
                    wrapMode:       Text.WordWrap
                    color:          Theme.ink
                }
            }

            Item {
                width:  parent.width
                height: Theme.touchTarget + Theme.spaceLG * 2

                Rectangle {
                    anchors { left: parent.left; right: parent.right; top: parent.top }
                    height: 1
                    color:  Theme.separator
                }

                RmListRow {
                    anchors {
                        left:    parent.left
                        right:   parent.right
                        verticalCenter: parent.verticalCenter
                    }
                    title:         "Quit ReadMarkable"
                    showSeparator: false
                    onClicked: {
                        if (typeof debugCtrl !== "undefined")
                            debugCtrl.quitApp()
                    }
                }
            }

            Item { width: 1; height: Theme.spaceLG }

        }
    }
}
