import QtQuick
import ReadMarkable

Item {
    id: root
    width:  ListView.view ? ListView.view.width : 954
    height: model.isFolder ? Theme.folderPillHeight : Theme.libraryListRowHeight

    property bool isEmpty:       model.isEmpty || false
    property bool showSeparator: true

    function formatLastRead(dateTime) {
        if (!dateTime || !dateTime.getTime) return ""
        var now  = new Date()
        var diff = Math.floor((now - dateTime) / 86400000)
        if (diff === 0) return "Today"
        if (diff === 1) return "Yesterday"
        if (diff < 7)   return diff + " days ago"
        if (diff < 30)  return Math.floor(diff / 7) + "w ago"
        return Qt.formatDate(dateTime, "MMM d")
    }

    Item {
        visible: model.isFolder
        anchors.fill: parent

        Rectangle {
            id: listFolderPill
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
                    width:          listFolderPill.width
                                        - Theme.folderPillHPadding * 2
                                        - Theme.folderPillGlyphSize
                                        - Theme.spaceSM
                }
            }
        }
    }

    Item {
        visible: !model.isFolder
        anchors.fill: parent

        Row {
            id: contentRow
            anchors {
                left:         parent.left
                right:        parent.right
                top:          parent.top
                bottom:       progressBar.top
                topMargin:    Theme.spaceXS
                bottomMargin: Theme.spaceTight
            }
            spacing: Theme.spaceSM

            Item {
                id:     thumbContainer
                width:  Theme.libraryThumbWidth
                height: Theme.libraryThumbHeight
                anchors.verticalCenter: parent.verticalCenter

                Image {
                    id:           thumbImage
                    anchors.fill: parent
                    source:       "image://cover/" + encodeURIComponent(model.filePath)
                    fillMode:     Image.PreserveAspectCrop
                    cache:        true
                    visible:      status !== Image.Error

                    Rectangle {
                        anchors.fill: parent
                        color:        Theme.subtle
                        border.width: Theme.borderWidth - 1
                        border.color: Theme.ink
                        visible:      thumbImage.status !== Image.Ready
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color:        Theme.subtle
                    border.width: Theme.borderWidth - 1
                    border.color: Theme.ink
                    visible:      thumbImage.status === Image.Error

                    Text {
                        anchors.centerIn: parent
                        text:             "?"
                        font.pixelSize:   Theme.textSM
                        color:            Theme.muted
                    }
                }
            }

            Column {
                id:      textCol
                width:   parent.width - thumbContainer.width - progressLabel.implicitWidth
                             - (progressLabel.visible ? parent.spacing : 0)
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spaceHair

                Text {
                    id:               titleText
                    width:            parent.width
                    text:             model.title || ""
                    font.pixelSize:   Theme.textListTitle
                    font.bold:        true
                    font.family:      Theme.uiFontFamily
                    color:            Theme.ink
                    elide:            Text.ElideRight
                    maximumLineCount: 1
                }

                Text {
                    id:               authorText
                    width:            parent.width
                    text:             model.author || ""
                    font.pixelSize:   Theme.textXS
                    color:            Theme.muted
                    elide:            Text.ElideRight
                    maximumLineCount: 1
                    visible:          text !== ""
                }

                Text {
                    id:    infoText
                    width: parent.width
                    text: {
                        var parts = []
                        if (model.currentChapter && model.currentChapter !== "") {
                            parts.push(model.currentChapter)
                        }
                        var lr = root.formatLastRead(model.lastRead)
                        if (lr !== "") {
                            parts.push("Last read " + lr)
                        }
                        return parts.join(" \u2013 ")
                    }
                    font.pixelSize:   Theme.textXS
                    color:            Theme.muted
                    elide:            Text.ElideRight
                    maximumLineCount: 1
                }
            }

            Text {
                id:             progressLabel
                anchors.verticalCenter: parent.verticalCenter
                text:           model.progressPercent > 0 ? model.progressPercent + "%" : ""
                font.pixelSize: Theme.textXS
                color:          Theme.dimmed
                visible:        text !== ""
            }
        }

        Item {
            id:      progressBar
            anchors {
                left:        parent.left
                right:       parent.right
                bottom:      separatorLine.top
            }
            height:  Theme.libraryListProgressBarHeight
            visible: model.progressPercent > 0

            Rectangle {
                anchors.fill: parent
                color:        Theme.subtle
            }

            Rectangle {
                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                width: Math.floor(parent.width * (model.progressPercent / 100))
                color: Theme.ink
            }
        }

        Rectangle {
            id:     separatorLine
            anchors {
                left:        parent.left
                right:       parent.right
                bottom:      parent.bottom
            }
            height:  1
            color:   Theme.separator
            visible: root.showSeparator
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
