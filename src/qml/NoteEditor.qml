import QtQuick 2.15
import QtQuick.Controls 2.15
import ReadMarkable

Item {
    id: root
    anchors.fill: parent

    readonly property bool isOpen: editorCard.visible

    function show(existingNote) {
        noteField.text = existingNote || ""
        editorCard.visible = true
        noteField.forceActiveFocus()
        noteKeyboard.show()
    }

    function dismiss() {
        noteKeyboard.dismiss()
        editorCard.visible = false
        noteField.text = ""
        root.noteCancelled()
    }

    signal noteSaved(string noteText)
    signal noteCancelled()

    MouseArea {
        anchors.fill: parent
        anchors.bottomMargin: editorCard.visible ? editorCard.height : 0
        enabled: editorCard.visible
        onClicked: root.dismiss()
    }

    Rectangle {
        id: editorCard
        anchors.left:  parent.left
        anchors.right: parent.right
        height: editorColumn.implicitHeight + 24 + (noteKeyboard.isOpen ? noteKeyboard.height : 0)
        color:  Theme.paper
        border.color: Theme.separator
        border.width: 1
        visible: false

        y: visible ? parent.height - height : parent.height

        Behavior on y {
            NumberAnimation { duration: 170; easing.type: Easing.Linear }
        }

        Column {
            id: editorColumn
            anchors.left:    parent.left
            anchors.right:   parent.right
            anchors.top:     parent.top
            anchors.margins: 16
            spacing:         12

            Item {
                width:  parent.width
                height: Theme.iconButtonSm

                Text {
                    anchors.left:           parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text:           "Add note"
                    font.pixelSize: Theme.textMD
                    font.bold:      true
                    font.family:    Theme.uiFontFamily
                    color:          Theme.ink
                }

                Rectangle {
                    anchors.right:          parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width:  32; height: 32
                    color:  cancelHeaderArea.containsPress ? Theme.ink : Theme.paper
                    border.color: Theme.ink; border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "X"; font.pixelSize: Theme.textMD
                        font.family: Theme.uiFontFamily
                        color: cancelHeaderArea.containsPress ? Theme.paper : Theme.ink
                    }

                    MouseArea {
                        id: cancelHeaderArea
                        anchors.fill: parent
                        onClicked: root.dismiss()
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: Theme.separator }

            TextArea {
                id: noteField
                width:       parent.width
                height:      Math.max(100, implicitHeight)
                wrapMode:    TextArea.Wrap
                font.pixelSize: Theme.textSM
                font.family:    Theme.uiFontFamily
                color:          Theme.ink
                background: Rectangle {
                    color: Theme.paper; border.color: Theme.separator; border.width: 1
                }
                placeholderText: "Type your note here..."
            }

            Row {
                spacing: 12

                Rectangle {
                    width: 120; height: Theme.touchTarget
                    color: saveArea.containsPress ? Theme.ink : Theme.paper
                    border.color: Theme.ink; border.width: 1

                    Text {
                        anchors.centerIn: parent; text: "Save"
                        font.pixelSize: Theme.textSM; font.family: Theme.uiFontFamily
                        color: saveArea.containsPress ? Theme.paper : Theme.ink
                    }

                    MouseArea {
                        id: saveArea; anchors.fill: parent
                        onClicked: {
                            noteKeyboard.dismiss()
                            var txt = noteField.text
                            editorCard.visible = false
                            noteField.text = ""
                            root.noteSaved(txt)
                        }
                    }
                }

                Rectangle {
                    width: 120; height: Theme.touchTarget
                    color: cancelArea.containsPress ? Theme.ink : Theme.paper
                    border.color: Theme.separator; border.width: 1

                    Text {
                        anchors.centerIn: parent; text: "Cancel"
                        font.pixelSize: Theme.textSM; font.family: Theme.uiFontFamily
                        color: cancelArea.containsPress ? Theme.paper : Theme.ink
                    }

                    MouseArea {
                        id: cancelArea; anchors.fill: parent
                        onClicked: root.dismiss()
                    }
                }
            }
        }

        OnScreenKeyboard {
            id: noteKeyboard
            target: noteField
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.bottom: parent.bottom
        }
    }
}
