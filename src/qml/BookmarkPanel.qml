import QtQuick 2.15
import QtQuick.Controls 2.15
import ReadMarkable

Drawer {
    id: bookmarkPanel

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

    signal bookmarkSelected(string xpointer)

    Item {
        anchors.fill: parent

        RmHeader {
            id: bookmarkHeader
            anchors.top:   parent.top
            anchors.left:  parent.left
            anchors.right: parent.right
            title: "Bookmarks"

            RmButton {
                compact: true
                label: "Close"
                onClicked: bookmarkPanel.close()
            }
        }

        Column {
            anchors.centerIn: parent
            visible: (typeof bookmarkModel === "undefined") || bookmarkModel.count === 0
            spacing: Theme.spaceXS

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text:           "No bookmarks yet"
                font.pixelSize: Theme.textMD
                font.bold:      true
                font.family:    Theme.uiFontFamily
                color:          Theme.ink
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: bookmarkPanel.width - Theme.spaceXL
                text:           "Tap the bookmark button while reading to add one."
                font.pixelSize: Theme.textSM
                font.family:    Theme.uiFontFamily
                color:          Theme.muted
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }

        ListView {
            id: bookmarkListView
            anchors.top:    bookmarkHeader.bottom
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.bottom: parent.bottom
            clip: true

            visible: (typeof bookmarkModel !== "undefined") && bookmarkModel.count > 0
            model:   (typeof bookmarkModel !== "undefined") ? bookmarkModel : null

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: RmListRow {

                title:        "Page " + model.page
                subtitle:     model.timestamp
                trailingText: (model.percent / 100).toFixed(0) + "%"
                onClicked: {
                    bookmarkPanel.bookmarkSelected(model.xpointer)
                }
            }
        }
    }
}
