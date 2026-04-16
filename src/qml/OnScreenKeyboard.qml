import QtQuick 2.15
import ReadMarkable

Item {
    id: keyboard

    height: isOpen ? keyboardContent.height : 0
    visible: isOpen
    z: 300

    property var target: null
    property bool isOpen: false

    function show() {
        isOpen = true
    }

    function dismiss() {
        isOpen = false
    }

    property bool shifted: false
    property bool symbolMode: false

    readonly property int keyH: 72
    readonly property int keySpacing: 4
    readonly property real stdKeyW: (keyboard.width - 11 * keySpacing) / 10

    Rectangle {
        anchors.fill: keyboardContent
        color: Theme.subtle
    }

    Column {
        id: keyboardContent
        anchors.left:  parent.left
        anchors.right: parent.right
        spacing: keySpacing
        padding: keySpacing

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: keyboard.keySpacing

            Repeater {
                model: keyboard.symbolMode
                       ? ["1","2","3","4","5","6","7","8","9","0"]
                       : ["q","w","e","r","t","y","u","i","o","p"]
                delegate: keyDelegate
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: keyboard.keySpacing

            Item { width: keyboard.stdKeyW * 0.5; height: 1 }

            Repeater {
                model: keyboard.symbolMode
                       ? ["-","/",":",";","(",")","$","&","@"]
                       : ["a","s","d","f","g","h","j","k","l"]
                delegate: keyDelegate
            }

            Item { width: keyboard.stdKeyW * 0.5; height: 1 }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: keyboard.keySpacing

            Item {
                width:  keyboard.stdKeyW * 1.4
                height: keyboard.keyH

                Rectangle {
                    anchors.fill: parent
                    color: shiftArea.containsPress ? Theme.ink : Theme.paper
                    border.width: 1
                    border.color: Theme.separator
                }

                Text {
                    anchors.centerIn: parent
                    text: keyboard.symbolMode ? "#=" : (keyboard.shifted ? "SHIFT" : "Shift")
                    font.pixelSize: Theme.textSM
                    font.family: Theme.uiFontFamily
                    font.bold: keyboard.shifted
                    color: shiftArea.containsPress ? Theme.paper : Theme.ink
                }

                MouseArea {
                    id: shiftArea
                    anchors.fill: parent
                    onClicked: keyboard.shifted = !keyboard.shifted
                }
            }

            Repeater {
                model: keyboard.symbolMode
                       ? [".",",","?","!","'","\"","#"]
                       : ["z","x","c","v","b","n","m"]
                delegate: keyDelegate
            }

            Item {
                width:  keyboard.stdKeyW * 1.4
                height: keyboard.keyH

                Rectangle {
                    anchors.fill: parent
                    color: bkspArea.containsPress ? Theme.ink : Theme.paper
                    border.width: 1
                    border.color: Theme.separator
                }

                Text {
                    anchors.centerIn: parent
                    text: "<--"
                    font.pixelSize: Theme.textSM
                    font.family: Theme.uiFontFamily
                    color: bkspArea.containsPress ? Theme.paper : Theme.ink
                }

                MouseArea {
                    id: bkspArea
                    anchors.fill: parent
                    onClicked: {
                        if (!keyboard.target) return
                        var t = keyboard.target.text
                        if (t.length > 0)
                            keyboard.target.text = t.substring(0, t.length - 1)
                    }
                }
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: keyboard.keySpacing

            Item {
                width:  keyboard.stdKeyW * 2
                height: keyboard.keyH

                Rectangle {
                    anchors.fill: parent
                    color: symArea.containsPress ? Theme.ink : Theme.paper
                    border.width: 1
                    border.color: Theme.separator
                }

                Text {
                    anchors.centerIn: parent
                    text: keyboard.symbolMode ? "ABC" : "123"
                    font.pixelSize: Theme.textSM
                    font.family: Theme.uiFontFamily
                    font.bold: true
                    color: symArea.containsPress ? Theme.paper : Theme.ink
                }

                MouseArea {
                    id: symArea
                    anchors.fill: parent
                    onClicked: {
                        keyboard.symbolMode = !keyboard.symbolMode
                        keyboard.shifted = false
                    }
                }
            }

            Item {
                width:  keyboard.stdKeyW * 5
                height: keyboard.keyH

                Rectangle {
                    anchors.fill: parent
                    color: spaceArea.containsPress ? Theme.ink : Theme.paper
                    border.width: 1
                    border.color: Theme.separator
                }

                Text {
                    anchors.centerIn: parent
                    text: "Space"
                    font.pixelSize: Theme.textSM
                    font.family: Theme.uiFontFamily
                    color: spaceArea.containsPress ? Theme.paper : Theme.ink
                }

                MouseArea {
                    id: spaceArea
                    anchors.fill: parent
                    onClicked: {
                        if (keyboard.target)
                            keyboard.target.text += " "
                    }
                }
            }

            Item {
                width:  keyboard.stdKeyW * 2
                height: keyboard.keyH

                Rectangle {
                    anchors.fill: parent
                    color: doneArea.containsPress ? Theme.paper : Theme.ink
                    border.width: 1
                    border.color: Theme.ink
                }

                Text {
                    anchors.centerIn: parent
                    text: "Done"
                    font.pixelSize: Theme.textSM
                    font.family: Theme.uiFontFamily
                    font.bold: true
                    color: doneArea.containsPress ? Theme.ink : Theme.paper
                }

                MouseArea {
                    id: doneArea
                    anchors.fill: parent
                    onClicked: keyboard.dismiss()
                }
            }
        }
    }

    Component {
        id: keyDelegate

        Item {
            width:  keyboard.stdKeyW
            height: keyboard.keyH

            Rectangle {
                anchors.fill: parent
                color: keyArea.containsPress ? Theme.ink : Theme.paper
                border.width: 1
                border.color: Theme.separator
            }

            Text {
                anchors.centerIn: parent
                text: {
                    var ch = modelData
                    if (!keyboard.symbolMode && keyboard.shifted)
                        ch = ch.toUpperCase()
                    return ch
                }
                font.pixelSize: Theme.textMD
                font.family: Theme.uiFontFamily
                color: keyArea.containsPress ? Theme.paper : Theme.ink
            }

            MouseArea {
                id: keyArea
                anchors.fill: parent
                onClicked: {
                    if (!keyboard.target) return
                    var ch = modelData
                    if (!keyboard.symbolMode && keyboard.shifted)
                        ch = ch.toUpperCase()
                    keyboard.target.text += ch

                    if (keyboard.shifted && !keyboard.symbolMode)
                        keyboard.shifted = false
                }
            }
        }
    }
}
