import QtQuick 2.15
import QtQuick.Controls 2.15
import ReadMarkable

Drawer {
    id: tocPanel

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

    signal chapterSelected(int index)

    onOpened: {
        if (typeof navController !== "undefined" && typeof tocModel !== "undefined") {
            const idx = navController.currentChapterIndex
            if (idx >= 0 && idx < tocModel.count)
                tocListView.positionViewAtIndex(idx, ListView.Center)
        }
    }

    Item {
        anchors.fill: parent

        RmHeader {
            id: tocHeader
            anchors.top:   parent.top
            anchors.left:  parent.left
            anchors.right: parent.right
            title: "Table of Contents"

            RmButton {
                compact: true
                label: "Close"
                onClicked: tocPanel.close()
            }
        }

        Text {
            anchors.centerIn: parent
            visible: (typeof tocModel === "undefined") || tocModel.count === 0
            width:   parent.width - Theme.spaceXL
            text:    "No table of contents available"
            font.pixelSize: Theme.textMD
            font.family:    Theme.uiFontFamily
            color:          Theme.muted
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        ListView {
            id: tocListView
            anchors.top:    tocHeader.bottom
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.bottom: parent.bottom
            clip: true

            visible: (typeof tocModel !== "undefined") && tocModel.count > 0
            model:   (typeof tocModel !== "undefined") ? tocModel : null

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: RmListRow {
                title:        model.title
                subtitle:     model.page > 0 ? ("Page " + model.page) : ""
                selected:     (typeof navController !== "undefined")
                              && (model.index === navController.currentChapterIndex)
                onClicked: {
                    tocPanel.chapterSelected(model.index)
                }
            }
        }
    }
}
