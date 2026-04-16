import QtQuick 2.15
import ReadMarkable

Item {
    id: root

    visible: false

    signal highlightRequested(int style)
    signal dictionaryRequested()
    signal copyRequested()
    signal noteRequested()

    property int currentStyle: 4

    readonly property int _primaryRowHeight: Theme.touchTarget
    readonly property int _paletteRowHeight: Theme.panelHeaderHeight
    readonly property int _colorDotSize:     Theme.iconButtonMd
    readonly property int _primaryMinWidth:  Theme.touchTarget + Theme.spaceMD * 2

    width:  _primaryMinWidth * 4
    height: _primaryRowHeight + Theme.borderWidth + _paletteRowHeight

    Rectangle {
        x: Theme.spaceHair
        y: Theme.spaceHair
        width:  parent.width
        height: parent.height
        color: Theme.highlightShadow
    }

    Rectangle {
        id: toolbarCard
        anchors.fill: parent
        color: Theme.paper
        border.color: Theme.panelBorder
        border.width: Theme.borderWidth

        Column {
            anchors.fill: parent
            spacing: 0

            Row {
                id: primaryRow
                width:  parent.width
                height: root._primaryRowHeight
                spacing: 0

                RmButton {
                    label:    "Highlight"
                    minWidth: root._primaryMinWidth
                    height:   root._primaryRowHeight
                    onClicked: root.highlightRequested(root.currentStyle)
                }

                RmButton {
                    label:    "Note"
                    minWidth: root._primaryMinWidth
                    height:   root._primaryRowHeight
                    onClicked: root.noteRequested()
                }

                RmButton {
                    label:    "Dictionary"
                    minWidth: root._primaryMinWidth
                    height:   root._primaryRowHeight
                    onClicked: root.dictionaryRequested()
                }

                RmButton {
                    label:    "Copy"
                    minWidth: root._primaryMinWidth
                    height:   root._primaryRowHeight
                    onClicked: root.copyRequested()
                }
            }

            Rectangle {
                width:  parent.width
                height: Theme.borderWidth
                color:  Theme.separator
            }

            Item {
                width:  parent.width
                height: root._paletteRowHeight

                Row {
                    anchors.centerIn: parent
                    spacing: Theme.spaceXS

                    Repeater {

                        model: [
                            { style: 4, fill: Theme.highlightYellow },
                            { style: 5, fill: Theme.highlightGreen  },
                            { style: 6, fill: Theme.highlightBlue   },
                            { style: 7, fill: Theme.highlightRed    },
                            { style: 8, fill: Theme.highlightOrange },
                            { style: 0, fill: Theme.highlightGray   }
                        ]

                        delegate: Rectangle {
                            width:  root._colorDotSize
                            height: root._colorDotSize
                            radius: root._colorDotSize / 2
                            color:  modelData.fill
                            border.color: Theme.ink
                            border.width: colorDotArea.containsPress
                                          ? Theme.borderWidth
                                          : (modelData.style === root.currentStyle
                                             ? Theme.borderWidth
                                             : Theme.borderWidth - 1)

                            MouseArea {
                                id: colorDotArea
                                anchors.fill: parent

                                anchors.margins: -Theme.spaceXXS
                                onClicked: {
                                    root.currentStyle = modelData.style
                                    root.highlightRequested(modelData.style)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

}
