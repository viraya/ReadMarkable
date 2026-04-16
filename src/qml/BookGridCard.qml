import QtQuick
import ReadMarkable

Item {
    id: root
    width:  GridView.view ? GridView.view.cellWidth  : 220
    height: GridView.view ? GridView.view.cellHeight : 360

    property bool isEmpty: model.isEmpty || false

    Item {
        visible: model.isFolder
        anchors.fill: parent

        Rectangle {
            id: folderPill
            anchors {
                left:        parent.left
                right:       parent.right
                top:         parent.top
                leftMargin:  Theme.spaceXXS
                rightMargin: Theme.spaceXXS
            }
            height:       Theme.folderPillHeight
            color:        Theme.subtle
            radius:       Theme.folderPillRadius
            border.width: 0

            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left:           parent.left
                anchors.leftMargin:     Theme.folderPillHPadding
                spacing:                Theme.spaceSM

                Image {
                    source:            root.isEmpty ? "qrc:/icons/folder-outlined.svg"
                                                    : "qrc:/icons/folder-filled.svg"
                    sourceSize.width:  Theme.folderPillGlyphSize
                    sourceSize.height: Theme.folderPillGlyphSize
                    width:             Theme.folderPillGlyphSize
                    height:            Theme.folderPillGlyphSize
                    smooth:            false
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text:           model.title || ""
                    font.pixelSize: Theme.textMD
                    font.family:    Theme.uiFontFamily
                    color:          Theme.ink
                    elide:          Text.ElideRight
                    width:          folderPill.width
                                        - Theme.folderPillHPadding * 2
                                        - Theme.folderPillGlyphSize
                                        - Theme.spaceSM
                }
            }
        }
    }

    Item {
        visible: !model.isFolder
        anchors {
            fill:         parent
            leftMargin:   Theme.spaceXXS
            rightMargin:  Theme.spaceXXS
            topMargin:    Theme.spaceTight
            bottomMargin: Theme.spaceTight
        }

        Item {
            id: coverContainer
            anchors {
                left:  parent.left
                right: parent.right
                top:   parent.top
            }
            height: parent.height * 0.7
            clip:   true

            Image {
                id: coverImage
                anchors.fill: parent
                source:   "image://cover/" + encodeURIComponent(model.filePath)
                fillMode: Image.PreserveAspectCrop
                cache:    true
                visible:  status === Image.Ready
            }

            Rectangle {
                anchors.fill: parent
                color:        Theme.subtle
                border.width: Theme.borderWidth
                border.color: Theme.ink
                visible:      coverImage.status !== Image.Ready

                Column {
                    anchors.centerIn: parent
                    width:   parent.width - Theme.spaceMD
                    spacing: Theme.spaceTight

                    Text {
                        width:               parent.width
                        text:                model.title || "Untitled"
                        font.pixelSize:      Theme.textXS
                        font.bold:           true
                        color:               Theme.ink
                        wrapMode:            Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        maximumLineCount:    4
                        elide:               Text.ElideRight
                    }
                    Text {
                        width:               parent.width
                        text:                model.author || ""
                        font.pixelSize:      Theme.textXS
                        color:               Theme.dimmed
                        wrapMode:            Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        maximumLineCount:    2
                        elide:               Text.ElideRight
                        visible:             (model.author || "") !== ""
                    }
                }
            }

            Rectangle {
                anchors { left: parent.left; bottom: parent.bottom; right: parent.right }
                height:  Theme.libraryListProgressBarHeight
                color:   Theme.subtle
                visible: model.progressPercent > 0

                Rectangle {
                    anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                    width: Math.floor(parent.width * (model.progressPercent / 100))
                    color: Theme.ink
                }
            }
        }

        Column {
            id: textColumn
            anchors {
                left:      parent.left
                right:     parent.right
                top:       coverContainer.bottom
                topMargin: Theme.spaceMicro
            }
            spacing: Theme.spaceHair
            clip:    true

            Text {
                width:            parent.width
                text:             model.title || ""
                font.pixelSize:   Theme.textLibraryGridTitle
                font.family:      Theme.uiFontFamily
                color:            Theme.ink
                elide:            Text.ElideRight
                wrapMode:         Text.WordWrap
                maximumLineCount: 2
                lineHeight:       1.1
            }

            Text {
                width:            parent.width
                text:             model.author || ""
                font.pixelSize:   Theme.textSM
                color:            Theme.muted
                elide:            Text.ElideRight
                maximumLineCount: 1
                visible:          (model.author || "") !== ""
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (model.isFolder) {

                var key = (model.xochitlUuid !== "" && model.xochitlUuid !== undefined)
                              ? model.xochitlUuid : model.folderPath
                libraryScreen.openFolder(key, model.title || "")
            } else {
                libraryScreen.openBook(model.filePath, model.title, model.author,
                                       model.progressPercent, model.currentChapter,
                                       model.lastRead, model.addedDate, model.description)
            }
        }
    }
}
