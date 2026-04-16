import QtQuick 2.15
import QtQuick.Controls 2.15
import ReadMarkable

Drawer {
    id: annotationsPanel

    edge:       Qt.RightEdge
    width:      parent.width * Theme.drawerWidthFraction
    height:     parent.height
    dragMargin: 0
    modal:      true

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

    background: Rectangle {
        color:        Theme.paper
        border.color: Theme.separator
        border.width: Theme.borderWidth - 1
    }

    signal navigateToAnnotation(string xpointer)

    Item {
        anchors.fill: parent

        RmHeader {
            id: annotationsHeader
            anchors.top:   parent.top
            anchors.left:  parent.left
            anchors.right: parent.right
            title: "Highlights"

            RmButton {
                compact: true
                label: "Close"
                onClicked: annotationsPanel.close()
            }
        }

        Column {
            anchors.centerIn: parent
            visible: (typeof annotationModel === "undefined") || annotationModel.count === 0
            spacing: Theme.spaceXS

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text:           "No highlights yet"
                font.pixelSize: Theme.textMD
                font.bold:      true
                font.family:    Theme.uiFontFamily
                color:          Theme.ink
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: annotationsPanel.width - Theme.spaceXL
                text:           "Select text and tap Highlight, or draw a note in the margin with the pen."
                font.pixelSize: Theme.textSM
                font.family:    Theme.uiFontFamily
                color:          Theme.muted
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }

        ListView {
            id: annotationsListView
            anchors.top:    annotationsHeader.bottom
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.bottom: parent.bottom
            clip: true

            visible: (typeof annotationModel !== "undefined") && annotationModel.count > 0
            model:   (typeof annotationModel !== "undefined") ? annotationModel : null

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: RmListRow {

                title: (model.isMarginNote === true)
                       ? "Handwritten note"
                       : (model.selectedText || "")
                subtitle: model.chapterTitle || ""
                onClicked: {
                    if (typeof annotationModel === "undefined") return
                    var xptr = annotationModel.startXPointerAt(model.index)
                    if (xptr !== "")
                        annotationsPanel.navigateToAnnotation(xptr)
                }
            }
        }
    }
}
