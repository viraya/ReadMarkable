import QtQuick 2.15
import ReadMarkable

Item {
    id: marginNoteCanvas
    visible: false
    clip: true

    property int    activeNoteId: 0
    property string activeParagraphXPtr: ""
    property var    existingStrokes: []
    property bool   isEditing: false

    property int canvasX: 0
    property int canvasY: 0
    property int canvasW: 48
    property int canvasH: 200

    signal noteSaved()
    signal noteDismissed()

    function show(noteId, strokeData, paragraphXPtr, paragraphY, canvasWidth, canvasHeight) {
        activeNoteId        = noteId
        activeParagraphXPtr = paragraphXPtr

        if (strokeData && typeof marginNoteService !== "undefined") {
            var parsed = marginNoteService.deserializeStrokesToRects(strokeData)
            existingStrokes = parsed || []
        } else {
            existingStrokes = []
        }

        canvasX = 0
        canvasY = 0
        canvasW = canvasWidth
        canvasH = canvasHeight
        isEditing = (noteId === 0)
        visible = true
    }

    function dismiss() {
        visible      = false
        activeNoteId = 0
        existingStrokes = []
        isEditing    = false
        noteDismissed()
    }

    Rectangle {
        anchors.fill: parent
        color:        Theme.paper
        border.color: Theme.ink
        border.width: 1
    }

    Repeater {
        model: marginNoteCanvas.existingStrokes
        delegate: Rectangle {
            x:      modelData.x
            y:      modelData.y
            width:  Math.max(2, modelData.width)
            height: Math.max(2, modelData.height)
            color:  Theme.ink
        }
    }

    Row {
        id: controlBar
        anchors.bottom:           parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin:     Theme.spaceMicro
        spacing: Theme.spaceMicro
        visible: marginNoteCanvas.visible

        Rectangle {
            width:        saveText.implicitWidth + 12
            height:       Theme.textSM + 8
            color:        saveArea.containsPress ? Theme.ink : Theme.paper
            border.color: Theme.ink
            border.width: 1

            Text {
                id:             saveText
                anchors.centerIn: parent
                text:           "Save"
                font.pixelSize: Theme.textXS
                font.family:    Theme.uiFontFamily
                color:          saveArea.containsPress ? Theme.paper : Theme.ink
            }

            MouseArea {
                id: saveArea
                anchors.fill: parent
                onClicked: {
                    marginNoteCanvas.noteSaved()
                    marginNoteCanvas.dismiss()
                }
            }
        }

        Rectangle {
            width:        clearText.implicitWidth + 12
            height:       Theme.textSM + 8
            color:        clearArea.containsPress ? Theme.ink : Theme.paper
            border.color: Theme.separator
            border.width: 1

            Text {
                id:             clearText
                anchors.centerIn: parent
                text:           "Clear"
                font.pixelSize: Theme.textXS
                font.family:    Theme.uiFontFamily
                color:          clearArea.containsPress ? Theme.paper : Theme.dimmed
            }

            MouseArea {
                id: clearArea
                anchors.fill: parent
                onClicked: {
                    marginNoteCanvas.existingStrokes = []
                    if (typeof marginNoteService !== "undefined"
                            && marginNoteCanvas.activeNoteId > 0) {
                        marginNoteService.deleteMarginNote(marginNoteCanvas.activeNoteId)
                    }
                    marginNoteCanvas.dismiss()
                }
            }
        }

        Rectangle {
            width:        closeText.implicitWidth + 12
            height:       Theme.textSM + 8
            color:        closeCanvasArea.containsPress ? Theme.ink : Theme.paper
            border.color: Theme.separator
            border.width: 1

            Text {
                id:             closeText
                anchors.centerIn: parent
                text:           "X"
                font.pixelSize: Theme.textXS
                font.family:    Theme.uiFontFamily
                color:          closeCanvasArea.containsPress ? Theme.paper : Theme.dimmed
            }

            MouseArea {
                id: closeCanvasArea
                anchors.fill: parent
                onClicked: marginNoteCanvas.dismiss()
            }
        }
    }
}
