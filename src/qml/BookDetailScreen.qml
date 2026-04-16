import QtQuick
import ReadMarkable

Item {
    id: bookDetailScreen
    anchors.fill: parent

    property string bookPath:        ""
    property string bookTitle:       ""
    property string bookAuthor:      ""
    property int    bookProgress:    0
    property string bookChapter:     ""
    property var    bookLastRead:    null
    property var    bookAddedDate:   null
    property string bookDescription: ""

    property bool   exportDialogVisible:  false
    property string exportFilePath:       ""
    property int    exportHighlightCount: 0
    property int    exportNoteCount:      0
    property int    exportBookmarkCount:  0

    signal openBook(string bookPath)
    signal goBack()

    function formatDate(dateTime) {
        if (!dateTime || !dateTime.getTime) return ""
        var now  = new Date()
        var diff = Math.floor((now - dateTime) / 86400000)
        if (diff === 0) return "Today"
        if (diff === 1) return "Yesterday"
        if (diff < 7)   return diff + " days ago"
        if (diff < 30)  return Math.floor(diff / 7) + " weeks ago"
        return Qt.formatDate(dateTime, "MMM d, yyyy")
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.paper
    }

    Item {
        id: topBar
        anchors {
            left:  parent.left
            right: parent.right
            top:   parent.top
        }
        height: Theme.toolbarHeight

        Item {
            id: backButton
            anchors {
                left:           parent.left
                verticalCenter: parent.verticalCenter
                leftMargin:     Theme.spaceSM
            }
            width:  backLabel.implicitWidth + Theme.spaceLG * 2
            height: Theme.touchTarget

            Text {
                id: backLabel
                anchors.centerIn: parent
                text:            "\u2039 Back"
                font.pixelSize:  Theme.textMD
                font.family:     Theme.uiFontFamily
                color:           Theme.ink
            }

            MouseArea {
                anchors.fill: parent
                onClicked:    bookDetailScreen.goBack()
            }
        }

        Rectangle {
            anchors {
                left:   parent.left
                right:  parent.right
                bottom: parent.bottom
            }
            height: 1
            color:  Theme.separator
        }
    }

    Connections {
        target: exportService
        function onExportComplete(mdPath, highlightCount, noteCount, bookmarkCount) {
            bookDetailScreen.exportFilePath       = mdPath
            bookDetailScreen.exportHighlightCount = highlightCount
            bookDetailScreen.exportNoteCount      = noteCount
            bookDetailScreen.exportBookmarkCount  = bookmarkCount
            bookDetailScreen.exportDialogVisible  = true
        }
        function onExportFailed(reason) {
            bookDetailScreen.exportFilePath       = "Export failed: " + reason
            bookDetailScreen.exportHighlightCount = 0
            bookDetailScreen.exportNoteCount      = 0
            bookDetailScreen.exportBookmarkCount  = 0
            bookDetailScreen.exportDialogVisible  = true
        }
    }

    Flickable {
        id: contentFlick
        anchors {
            left:   parent.left
            right:  parent.right
            top:    topBar.bottom
            bottom: parent.bottom
        }
        contentWidth:  width
        contentHeight: contentColumn.implicitHeight + 32
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Column {
            id: contentColumn
            anchors {
                left:       parent.left
                right:      parent.right
                top:        parent.top
                topMargin:  Theme.spaceLG
            }
            spacing: 0

            Item {
                width:  parent.width
                height: coverImage.height + 16

                Image {
                    id: coverImage
                    anchors.horizontalCenter: parent.horizontalCenter
                    width:    Theme.coverDetailWidth
                    height:   Theme.coverDetailHeight
                    source:   bookDetailScreen.bookPath !== ""
                                ? "image://cover/" + encodeURIComponent(bookDetailScreen.bookPath)
                                : ""
                    fillMode: Image.PreserveAspectFit
                    cache:    true

                    Rectangle {
                        anchors.fill: parent
                        color:   Theme.subtle
                        visible: coverImage.status !== Image.Ready

                        Text {
                            anchors.centerIn: parent
                            text:            bookDetailScreen.bookTitle
                            font.pixelSize:  Theme.textSM
                            color:           Theme.dimmed
                            wrapMode:        Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                            width:           parent.width - 16
                            maximumLineCount: 4
                        }
                    }
                }
            }

            Text {
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                text:            bookDetailScreen.bookTitle
                font.pixelSize:  Theme.textLG
                font.bold:       true
                font.family:     Theme.uiFontFamily
                color:           Theme.ink
                wrapMode:        Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                topPadding:      Theme.spaceMD
                bottomPadding:   Theme.spaceXS
            }

            Text {
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                text:            bookDetailScreen.bookAuthor || ""
                font.pixelSize:  Theme.textMD
                color:           Theme.dimmed
                wrapMode:        Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                visible:         bookDetailScreen.bookAuthor !== ""
                bottomPadding:   Theme.spaceXS
            }

            Rectangle {
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                height:      1
                color:       Theme.subtle
            }

            Item {
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                height: Theme.touchTarget + Theme.spaceMD * 2

                Rectangle {
                    anchors {
                        left:           parent.left
                        right:          parent.right
                        verticalCenter: parent.verticalCenter
                    }
                    height:       Theme.touchTarget
                    color:        Theme.ink

                    Text {
                        anchors.centerIn: parent
                        text: bookDetailScreen.bookProgress > 0
                                ? "Continue Reading (" + bookDetailScreen.bookProgress + "%)"
                                : "Open Book"
                        font.pixelSize:  Theme.textMD
                        font.bold:       true
                        font.family:     Theme.uiFontFamily
                        color:           Theme.paper
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked:    bookDetailScreen.openBook(bookDetailScreen.bookPath)
                    }
                }
            }

            Column {
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                spacing:  Theme.spaceXXS
                visible:  bookDetailScreen.bookProgress > 0

                Item { width: 1; height: Theme.spaceSM }

                Item {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    height: Theme.bookDetailProgressContainerHeight

                    Rectangle {
                        anchors {
                            left:           parent.left
                            right:          parent.right
                            verticalCenter: parent.verticalCenter
                        }
                        height: Theme.bookDetailProgressTrackHeight
                        color:  Theme.subtle

                        Rectangle {
                            width:  Math.floor(parent.width * (bookDetailScreen.bookProgress / 100))
                            height: parent.height
                            color:  Theme.ink
                        }
                    }
                }

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text:            bookDetailScreen.bookProgress + "% complete"
                    font.pixelSize:  Theme.textSM
                    color:           Theme.faint
                }

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text:            bookDetailScreen.bookChapter
                    font.pixelSize:  Theme.textSM
                    color:           Theme.faint
                    visible:         bookDetailScreen.bookChapter !== ""
                }

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text: {
                        var d = bookDetailScreen.formatDate(bookDetailScreen.bookLastRead)
                        return d !== "" ? "Last read: " + d : ""
                    }
                    font.pixelSize:  Theme.textSM
                    color:           Theme.dimmed
                    visible:         text !== ""
                }
            }

            Rectangle {
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                height:      1
                color:       Theme.subtle
            }

            Column {
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                spacing:      Theme.spaceXXS

                Item { width: 1; height: Theme.spaceMD }

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text:           "Description"
                    font.pixelSize: Theme.textMD
                    font.bold:      true
                    font.family:    Theme.uiFontFamily
                    color:          Theme.ink
                }

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text:        bookDetailScreen.bookDescription !== ""
                                    ? bookDetailScreen.bookDescription
                                    : "No description available."
                    font.pixelSize: Theme.textSM
                    color:       bookDetailScreen.bookDescription !== ""
                                    ? Theme.faint
                                    : Theme.muted
                    font.italic: bookDetailScreen.bookDescription === ""
                    wrapMode:    Text.WordWrap
                    lineHeight:  1.4
                }
            }

            Rectangle {
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                height: 1
                color:  Theme.subtle
            }

            Column {
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                spacing:       Theme.spaceTight

                Item { width: 1; height: Theme.spaceMD }

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text:           "File Info"
                    font.pixelSize: Theme.textMD
                    font.bold:      true
                    font.family:    Theme.uiFontFamily
                    color:          Theme.ink
                }

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text:           bookDetailScreen.bookPath
                    font.pixelSize: Theme.textXS
                    color:          Theme.muted
                    wrapMode:       Text.WrapAnywhere
                }

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text: {
                        var d = bookDetailScreen.formatDate(bookDetailScreen.bookAddedDate)
                        return d !== "" ? "Added: " + d : ""
                    }
                    font.pixelSize: Theme.textXS
                    color:          Theme.dimmed
                    visible:        text !== ""
                }
            }

            Item {
                anchors {
                    left:        parent.left
                    right:       parent.right
                    leftMargin:  Theme.spaceLG
                    rightMargin: Theme.spaceLG
                }
                height: Theme.touchTarget + Theme.spaceXL

                Rectangle {
                    anchors {
                        left:           parent.left
                        right:          parent.right
                        verticalCenter: parent.verticalCenter
                    }
                    height:       Theme.touchTarget
                    color:        exportArea.containsPress ? Theme.ink : Theme.paper
                    border.color: Theme.ink
                    border.width: Theme.borderWidth

                    Text {
                        anchors.centerIn: parent
                        text:            "Export Highlights"
                        font.pixelSize:  Theme.textMD
                        font.family:     Theme.uiFontFamily
                        color:           exportArea.containsPress ? Theme.paper : Theme.ink
                    }

                    MouseArea {
                        id: exportArea
                        anchors.fill: parent
                        onClicked: {
                            exportService.exportBook(bookDetailScreen.bookPath,
                                                     bookDetailScreen.bookTitle,
                                                     bookDetailScreen.bookAuthor)
                        }
                    }
                }
            }
        }
    }

    Item {
        id: exportOverlay
        anchors.fill: parent
        visible: bookDetailScreen.exportDialogVisible
        z: 200

        Rectangle {
            anchors.fill: parent
            color: Theme.highlightShadow
        }

        Rectangle {
            id: exportDialog
            anchors.centerIn: parent
            width:  Math.floor(parent.width * 0.8)
            height: exportDialogColumn.implicitHeight + Theme.spaceLG * 2
            color:  Theme.paper
            border.color: Theme.ink
            border.width: Theme.borderWidth

            Column {
                id: exportDialogColumn
                anchors {
                    left:    parent.left
                    right:   parent.right
                    top:     parent.top
                    margins: Theme.spaceLG
                }
                spacing: Theme.spaceSM

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text:            "Export Complete"
                    font.pixelSize:  Theme.textLG
                    font.bold:       true
                    font.family:     Theme.uiFontFamily
                    color:           Theme.ink
                    wrapMode:        Text.WordWrap
                }

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text:            bookDetailScreen.exportFilePath
                    font.pixelSize:  Theme.textSM
                    color:           Theme.faint
                    wrapMode:        Text.WrapAnywhere
                }

                Text {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    text: bookDetailScreen.exportHighlightCount + " highlights, " +
                          bookDetailScreen.exportNoteCount      + " notes, " +
                          bookDetailScreen.exportBookmarkCount  + " bookmarks"
                    font.pixelSize:  Theme.textMD
                    color:           Theme.ink
                    wrapMode:        Text.WordWrap
                }

                Rectangle {
                    anchors {
                        left:  parent.left
                        right: parent.right
                    }
                    height: Theme.bookDetailOkButtonHeight
                    color:  Theme.ink

                    Text {
                        anchors.centerIn: parent
                        text:            "OK"
                        font.pixelSize:  Theme.textMD
                        font.bold:       true
                        font.family:     Theme.uiFontFamily
                        color:           Theme.paper
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked:    bookDetailScreen.exportDialogVisible = false
                    }
                }
            }
        }
    }
}
