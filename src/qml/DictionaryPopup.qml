import QtQuick 2.15
import ReadMarkable

Item {
    id: root
    anchors.fill: parent
    z: 100
    visible: false

    property string lookupWord: ""
    property var    entries:    []

    readonly property bool isOpen: root.visible

    function show(word) {
        lookupWord = word
        entries    = []
        expanded   = false
        root.visible = true
    }

    function showWithResults(word, resultEntries) {
        lookupWord = word
        entries    = resultEntries
        expanded   = false
        root.visible = true
    }

    function dismiss() {
        root.visible = false
    }

    property bool expanded: false

    Rectangle {
        anchors.fill: parent
        color:        Theme.overlayDim
        MouseArea {
            anchors.fill: parent
            onClicked:    root.dismiss()
        }
    }

    RmPanel {
        id: card
        width:  parent.width  * Theme.popupWidthFraction
        height: parent.height * Theme.popupHeightFraction
        anchors.centerIn: parent

        header: RmHeader {
            title: "Definition: " + root.lookupWord

            RmButton {
                label: "Close"
                onClicked: root.dismiss()
            }
        }

        Item {
            id: loadingArea
            anchors.fill: parent
            visible: (typeof dictionaryService !== "undefined")
                     && dictionaryService.loading
                     && root.entries.length === 0

            Text {
                anchors.centerIn: parent
                text:           "Looking up..."
                font.pixelSize: Theme.textMD
                font.family:    Theme.uiFontFamily
                font.italic:    true
                color:          Theme.dimmed
            }
        }

        Flickable {
            id: contentFlick
            anchors.fill: parent
            clip: true

            contentWidth:  width
            contentHeight: contentCol.implicitHeight

            visible: !loadingArea.visible

            Column {
                id: contentCol
                width:   parent.width
                spacing: 0

                Text {
                    width:          parent.width
                    height:         implicitHeight + Theme.spaceXXS
                    visible:        root.entries.length === 0
                                    && !(typeof dictionaryService !== "undefined"
                                         && dictionaryService.loading)
                    text:           "No definition found for '" + root.lookupWord + "'"
                    font.pixelSize: Theme.textMD
                    font.family:    Theme.uiFontFamily
                    font.italic:    true
                    color:          Theme.muted
                    wrapMode:       Text.WordWrap
                }

                Column {
                    width:   parent.width
                    spacing: Theme.spaceTight
                    visible: root.entries.length > 0

                    Text {
                        width:          parent.width
                        visible:        root.entries.length > 0
                                        && (root.entries[0].ipa || "") !== ""
                        text:           root.entries.length > 0
                                        ? (root.entries[0].ipa || "") : ""
                        font.pixelSize: Theme.textSM
                        font.family:    Theme.uiFontFamily
                        font.italic:    true
                        color:          Theme.dimmed
                        wrapMode:       Text.WordWrap
                    }

                    Text {
                        width:          parent.width
                        visible:        root.entries.length > 0
                                        && (root.entries[0].partOfSpeech || "") !== ""
                        text:           root.entries.length > 0
                                        ? (root.entries[0].partOfSpeech || "") : ""
                        font.pixelSize: Theme.textSM
                        font.bold:      true
                        font.family:    Theme.uiFontFamily
                        color:          Theme.faint
                        wrapMode:       Text.WordWrap
                    }

                    Text {
                        width:          parent.width
                        visible:        root.entries.length > 0
                        text:           root.entries.length > 0
                                        ? (root.entries[0].definition || "") : ""
                        font.pixelSize: Theme.textMD
                        font.family:    Theme.uiFontFamily
                        color:          Theme.ink
                        wrapMode:       Text.WordWrap
                    }

                    Text {
                        width:          parent.width
                        visible:        root.entries.length > 0
                                        && (root.entries[0].example || "") !== ""
                        text:           root.entries.length > 0
                                        ? (root.entries[0].example || "") : ""
                        font.pixelSize: Theme.textSM
                        font.family:    Theme.uiFontFamily
                        font.italic:    true
                        color:          Theme.dimmed
                        wrapMode:       Text.WordWrap
                        topPadding:     Theme.spaceMicro
                    }
                }

                Item {
                    width:   parent.width
                    height:  root.entries.length > 1 ? Theme.spaceXS : 0
                    visible: root.entries.length > 1
                }

                Rectangle {
                    width:   parent.width
                    height:  root.entries.length > 1 ? Theme.spaceHair : 0
                    visible: root.entries.length > 1
                    color:   Theme.separator
                }

                Item {
                    width:   parent.width
                    height:  root.entries.length > 1 ? Theme.spaceXXS : 0
                    visible: root.entries.length > 1
                }

                Rectangle {
                    id: moreBtn
                    width:   parent.width
                    height:  root.entries.length > 1 ? Theme.actionRowHeight : 0
                    visible: root.entries.length > 1
                    color:   moreArea.containsPress ? Theme.subtle : Theme.paper
                    border.color: Theme.separator
                    border.width: Theme.borderWidth - 1

                    Text {
                        anchors.left:           parent.left
                        anchors.leftMargin:     Theme.spaceXXS
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.expanded
                              ? "Hide extra definitions"
                              : "More definitions (" + (root.entries.length - 1) + ")"
                        font.pixelSize: Theme.textSM
                        font.family:    Theme.uiFontFamily
                        color:          Theme.faint
                    }

                    MouseArea {
                        id: moreArea
                        anchors.fill: parent
                        onClicked:    root.expanded = !root.expanded
                    }
                }

                Item {
                    width:   parent.width
                    height:  root.entries.length > 1 ? Theme.spaceXXS : 0
                    visible: root.entries.length > 1
                }

                Repeater {
                    model: root.expanded ? root.entries.length - 1 : 0

                    delegate: Column {
                        width:   contentCol.width
                        spacing: Theme.spaceTight

                        required property int index

                        Rectangle {
                            width:  parent.width
                            height: Theme.spaceHair
                            color:  Theme.separator
                        }

                        Item { width: parent.width; height: Theme.spaceXXS }

                        Text {
                            width:          parent.width
                            visible:        (root.entries[index + 1].ipa || "") !== ""
                            text:           root.entries[index + 1].ipa || ""
                            font.pixelSize: Theme.textSM
                            font.family:    Theme.uiFontFamily
                            font.italic:    true
                            color:          Theme.dimmed
                            wrapMode:       Text.WordWrap
                        }

                        Text {
                            width:          parent.width
                            visible:        (root.entries[index + 1].partOfSpeech || "") !== ""
                            text:           root.entries[index + 1].partOfSpeech || ""
                            font.pixelSize: Theme.textSM
                            font.bold:      true
                            font.family:    Theme.uiFontFamily
                            color:          Theme.faint
                            wrapMode:       Text.WordWrap
                        }

                        Text {
                            width:          parent.width
                            text:           root.entries[index + 1].definition || ""
                            font.pixelSize: Theme.textMD
                            font.family:    Theme.uiFontFamily
                            color:          Theme.ink
                            wrapMode:       Text.WordWrap
                        }

                        Text {
                            width:          parent.width
                            visible:        (root.entries[index + 1].example || "") !== ""
                            text:           root.entries[index + 1].example || ""
                            font.pixelSize: Theme.textSM
                            font.family:    Theme.uiFontFamily
                            font.italic:    true
                            color:          Theme.dimmed
                            wrapMode:       Text.WordWrap
                            topPadding:     Theme.spaceMicro
                        }

                        Item { width: parent.width; height: Theme.spaceXXS }
                    }
                }
            }
        }
    }
}
