import QtQuick 2.15
import ReadMarkable

Item {
    id: root
    anchors.fill: parent
    z: 100
    visible: false

    property string footnoteText: ""
    property string footnoteHref: ""

    readonly property bool isOpen: root.visible

    function show(text, href) {
        footnoteText = text
        footnoteHref = href
        root.visible = true
    }

    function dismiss() {
        root.visible = false
    }

    Rectangle {
        anchors.fill: parent
        color:        Theme.overlayDim
        MouseArea {
            anchors.fill: parent
            onClicked:    root.dismiss()
        }
    }

    RmPanel {
        id: card
        width:  parent.width  * Theme.popupWidthFraction
        height: parent.height * Theme.popupHeightFraction
        anchors.centerIn: parent

        header: RmHeader {
            title: "Footnote"

            RmButton {
                label: "Close"
                onClicked: root.dismiss()
            }
        }

        Column {
            id: contentColumn
            anchors.fill: parent
            spacing: Theme.spaceSM

            Flickable {
                id: bodyFlick
                width:  parent.width
                height: parent.height - (goToButton.visible ? goToButton.height + Theme.spaceSM : 0)
                contentWidth:  width
                contentHeight: footnoteBody.implicitHeight
                clip: true

                Text {
                    id: footnoteBody
                    width:          parent.width
                    text:           root.footnoteText
                    textFormat:     Text.RichText
                    font.pixelSize: Theme.textMD
                    font.family:    Theme.uiFontFamily
                    wrapMode:       Text.WordWrap
                    color:          Theme.ink
                }
            }

            RmButton {
                id: goToButton
                label:   "Go to reference"
                visible: root.footnoteHref !== ""
                onClicked: {
                    if (typeof navController !== "undefined")
                        navController.navigateLink(root.footnoteHref)
                    root.dismiss()
                }
            }
        }
    }
}
