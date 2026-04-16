import QtQuick 2.15
import ReadMarkable

Item {
    id: selectionHandles

    visible: (typeof selectionController !== "undefined") && selectionController.hasSelection

    Item {
        id: startHandle

        readonly property int handleW: Theme.selectionHandleWidth
        readonly property int handleH: Theme.selectionHandleHeight

        x: (typeof selectionController !== "undefined")
           ? selectionController.startHandleX / readerView.coordScaleX - handleW / 2
           : 0
        y: (typeof selectionController !== "undefined")
           ? selectionController.startHandleY / readerView.coordScaleY
           : 0

        width:  handleW
        height: handleH

        Rectangle {
            anchors.fill: parent
            color:  Theme.ink
            radius: Theme.borderRadius
        }

        Rectangle {
            anchors.centerIn: parent
            width:  Theme.handleBarWidth
            height: parent.height - Theme.selectionHandleBarInset
            color:  Theme.paper
        }

        MouseArea {
            id: startDragArea
            anchors.fill: parent

            anchors.margins: -Theme.selectionHandleHitSlop

            property real pressX: 0
            property real pressY: 0

            onPressed: function(mouse) {
                pressX = mouse.x
                pressY = mouse.y
                mouse.accepted = true
            }

            onPositionChanged: function(mouse) {
                if (!pressed) return
                if (typeof selectionController === "undefined") return

                var itemX = startHandle.x + mouse.x
                var itemY = startHandle.y + mouse.y

                var clampedX = Math.max(0, Math.min(selectionHandles.width  - 1, itemX))
                var clampedY = Math.max(0, Math.min(selectionHandles.height - 1, itemY))

                selectionController.extendSelectionTo(
                    clampedX * readerView.coordScaleX,
                    clampedY * readerView.coordScaleY, true)
            }
        }
    }

    Item {
        id: endHandle

        readonly property int handleW: Theme.selectionHandleWidth
        readonly property int handleH: Theme.selectionHandleHeight

        x: (typeof selectionController !== "undefined")
           ? selectionController.endHandleX / readerView.coordScaleX - handleW / 2
           : 0
        y: (typeof selectionController !== "undefined")
           ? selectionController.endHandleY / readerView.coordScaleY
           : 0

        width:  handleW
        height: handleH

        Rectangle {
            anchors.fill: parent
            color:  Theme.ink
            radius: Theme.borderRadius
        }
        Rectangle {
            anchors.centerIn: parent
            width:  Theme.handleBarWidth
            height: parent.height - Theme.selectionHandleBarInset
            color:  Theme.paper
        }

        MouseArea {
            id: endDragArea
            anchors.fill: parent
            anchors.margins: -Theme.selectionHandleHitSlop

            onPressed: function(mouse) {
                mouse.accepted = true
            }

            onPositionChanged: function(mouse) {
                if (!pressed) return
                if (typeof selectionController === "undefined") return

                var itemX = endHandle.x + mouse.x
                var itemY = endHandle.y + mouse.y

                var clampedX = Math.max(0, Math.min(selectionHandles.width  - 1, itemX))
                var clampedY = Math.max(0, Math.min(selectionHandles.height - 1, itemY))

                selectionController.extendSelectionTo(
                    clampedX * readerView.coordScaleX,
                    clampedY * readerView.coordScaleY, false)
            }
        }
    }
}
