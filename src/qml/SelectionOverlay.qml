import QtQuick 2.15
import ReadMarkable

Item {
    id: selectionOverlay

    visible: (typeof selectionController !== "undefined") && selectionController.hasSelection

    Repeater {
        model: (typeof selectionController !== "undefined") ? selectionController.selectionRects : []

        delegate: Rectangle {

            x:      modelData.x      / readerView.coordScaleX
            y:      modelData.y      / readerView.coordScaleY
            width:  modelData.width  / readerView.coordScaleX
            height: modelData.height / readerView.coordScaleY

            color: Theme.selectionOverlay
        }
    }
}
