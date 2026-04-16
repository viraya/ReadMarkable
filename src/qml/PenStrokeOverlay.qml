import QtQuick 2.15
import ReadMarkable

Item {
    id: penStrokeOverlay
    visible: (typeof penInputController !== "undefined") && penInputController.isDrawing

    Repeater {
        model: (typeof penInputController !== "undefined") ? penInputController.inkTrail : []
        delegate: Rectangle {
            x:      modelData.x
            y:      modelData.y
            width:  Math.max(2, modelData.width)
            height: Math.max(2, modelData.height)
            color:  Theme.strokeOverlay
        }
    }
}
