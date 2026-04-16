import QtQuick 2.15
import ReadMarkable

Flickable {
    id: fontPickerFlickable

    width: parent.width
    height: Theme.fontPickerHeight

    contentWidth: fontRow.width
    contentHeight: fontRow.height

    clip: true
    flickableDirection: Flickable.HorizontalFlick

    Row {
        id: fontRow
        spacing: 12

        Repeater {
            model: settingsController ? settingsController.availableFonts : []

            delegate: Rectangle {
                width: labelText.implicitWidth + 32
                height: Theme.iconButton
                radius: Theme.buttonRadius

                color:        settingsController.fontFace === modelData ? Theme.ink : Theme.paper
                border.color: Theme.ink
                border.width: 1

                Text {
                    id: labelText
                    anchors.centerIn: parent

                    text:            modelData.split(" ")[0]
                    font.family:     modelData
                    font.pixelSize:  Theme.textSM
                    color: settingsController.fontFace === modelData ? Theme.paper : Theme.ink
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: settingsController.fontFace = modelData
                }
            }
        }
    }
}
