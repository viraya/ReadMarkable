import QtQuick 2.15
import ReadMarkable

Item {
    id: highlightOverlay

    Repeater {
        model: (typeof selectionController !== "undefined")
               ? selectionController.pageHighlightRects
               : []

        delegate: Item {
            id: annotationGroup

            readonly property var   hlRects:      modelData.rects
            readonly property int   hlStyle:      modelData.style
            readonly property int   hlAnnotId:    modelData.annotationId

            readonly property int bbLeft:   hlRects.length > 0 ? computeLeft()   : 0
            readonly property int bbTop:    hlRects.length > 0 ? computeTop()    : 0
            readonly property int bbRight:  hlRects.length > 0 ? computeRight()  : 0
            readonly property int bbBottom: hlRects.length > 0 ? computeBottom() : 0

            function computeLeft() {
                var l = hlRects[0].x
                for (var i = 1; i < hlRects.length; ++i)
                    if (hlRects[i].x < l) l = hlRects[i].x
                return l / readerView.coordScaleX
            }
            function computeTop() {
                var t = hlRects[0].y
                for (var i = 1; i < hlRects.length; ++i)
                    if (hlRects[i].y < t) t = hlRects[i].y
                return t / readerView.coordScaleY
            }
            function computeRight() {
                var r = hlRects[0].x + hlRects[0].width
                for (var i = 1; i < hlRects.length; ++i) {
                    var rr = hlRects[i].x + hlRects[i].width
                    if (rr > r) r = rr
                }
                return r / readerView.coordScaleX
            }
            function computeBottom() {
                var b = hlRects[0].y + hlRects[0].height
                for (var i = 1; i < hlRects.length; ++i) {
                    var bb = hlRects[i].y + hlRects[i].height
                    if (bb > b) b = bb
                }
                return b / readerView.coordScaleY
            }

            Repeater {
                model: annotationGroup.hlRects

                delegate: Rectangle {
                    x:      modelData.x      / readerView.coordScaleX
                    y:      modelData.y      / readerView.coordScaleY
                    width:  modelData.width  / readerView.coordScaleX
                    height: modelData.height / readerView.coordScaleY

                    color: {
                        switch (annotationGroup.hlStyle) {
                        case 4:  return Theme.highlightYellowOverlay
                        case 1:
                        case 5:  return Theme.highlightGreenOverlay
                        case 2:
                        case 6:  return Theme.highlightBlueOverlay
                        case 3:
                        case 7:  return Theme.highlightRedOverlay
                        case 8:  return Theme.highlightOrangeOverlay
                        default: return Theme.highlightGrayOverlay
                        }
                    }
                }
            }

            MouseArea {
                x:      annotationGroup.bbLeft
                y:      annotationGroup.bbTop
                width:  annotationGroup.bbRight  - annotationGroup.bbLeft
                height: annotationGroup.bbBottom - annotationGroup.bbTop

                onClicked: {
                    if (typeof selectionController !== "undefined") {
                        selectionController.reactivateHighlight(annotationGroup.hlAnnotId)
                        readerView.selectionMode = selectionController.hasSelection
                    }
                }
            }

        }
    }
}
