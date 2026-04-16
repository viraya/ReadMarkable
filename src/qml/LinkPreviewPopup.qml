import QtQuick 2.15
import ReadMarkable

Item {
    id: root
    anchors.fill: parent
    z: 100
    visible: false

    property string previewText: ""
    property string linkHref:    ""

    readonly property bool isOpen: root.visible

    function show(text, href) {
        previewText = text
        linkHref    = href
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
            title: "Link"

            RmButton {
                label: "Close"
                onClicked: root.dismiss()
            }
        }

        Column {
            id: popupColumn
            anchors.fill: parent
            spacing: Theme.spaceSM

            Text {
                id: hrefText
                width:          parent.width
                text:           root.linkHref
                font.pixelSize: Theme.textSM
                font.family:    Theme.uiFontFamily
                color:          Theme.muted
                wrapMode:       Text.WrapAnywhere
                visible:        root.linkHref !== ""
            }

            Flickable {
                id: bodyFlick
                width:  parent.width
                height: parent.height
                        - (hrefText.visible ? hrefText.implicitHeight + popupColumn.spacing : 0)
                        - followButton.height
                        - popupColumn.spacing
                contentWidth:  width
                contentHeight: previewBody.implicitHeight
                clip: true

                Text {
                    id: previewBody
                    width:          parent.width
                    text:           root.previewText
                    font.pixelSize: Theme.textMD
                    font.family:    Theme.uiFontFamily
                    wrapMode:       Text.WordWrap
                    color:          Theme.ink
                }
            }

            RmButton {
                id: followButton
                label: "Follow"
                onClicked: {
                    if (typeof navController !== "undefined")
                        navController.navigateLink(root.linkHref)
                    root.dismiss()
                }
            }
        }
    }
}
