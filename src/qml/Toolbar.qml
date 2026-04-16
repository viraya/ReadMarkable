import QtQuick 2.15
import QtQuick.Controls 2.15
import ReadMarkable

Item {
    id: toolbar
    anchors.fill: parent

    signal openToc()
    signal openBookmarks()
    signal openAnnotations()
    signal openSettings()
    signal openSearch()
    signal dismiss()
    signal goToLibrary()

    Rectangle {
        id: topBar
        anchors.top:   parent.top
        anchors.left:  parent.left
        anchors.right: parent.right
        height: Theme.readerTopBarHeight
        color:  Theme.paper

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.bottom: parent.bottom
            height: Theme.borderWidth
            color:  Theme.separator
        }

        Column {
            anchors.fill: parent
            spacing: 0

            Item {
                id: titleRow
                width:  parent.width
                height: Theme.toolbarTitleRowHeight

                RmButton {
                    id: backBtn
                    compact: true; flat: true
                    anchors.left:           parent.left
                    anchors.leftMargin:     Theme.spaceMD
                    anchors.verticalCenter: parent.verticalCenter
                    label: "Back"
                    onClicked: toolbar.goToLibrary()
                }

                RmButton {
                    id: bookmarkBtn
                    compact: true
                    anchors.right:          parent.right
                    anchors.rightMargin:    Theme.spaceMD
                    anchors.verticalCenter: parent.verticalCenter
                    label: "Bookmark"
                    isActive: (typeof navController !== "undefined")
                              && navController.currentPageBookmarked
                    onClicked: {
                        if (typeof navController !== "undefined")
                            navController.toggleBookmark()
                    }
                }

                Column {
                    anchors.left:           backBtn.right
                    anchors.right:          bookmarkBtn.left
                    anchors.leftMargin:     Theme.spaceMD
                    anchors.rightMargin:    Theme.spaceMD
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 0

                    Text {
                        id: bookTitleText
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: (typeof navController !== "undefined")
                              ? navController.currentBookTitle : ""
                        font.pixelSize: Theme.textSM
                        font.family:    Theme.uiFontFamily
                        color:          Theme.muted
                        elide:          Text.ElideRight
                        visible:        text !== ""
                    }

                    Text {
                        id: chapterNameText
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: (typeof navController !== "undefined")
                              ? navController.currentChapterTitle : ""
                        font.pixelSize: Theme.textChapter
                        font.bold:      true
                        font.family:    Theme.uiFontFamily
                        color:          Theme.ink
                        elide:          Text.ElideRight
                    }
                }
            }

            Item {
                id: actionRow
                width:  parent.width
                height: Theme.toolbarActionRowHeight

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter:   parent.verticalCenter
                    spacing: Theme.spaceLG

                    RmButton {
                        compact: true; flat: true
                        label: "TOC"
                        onClicked: toolbar.openToc()
                    }

                    RmButton {
                        compact: true; flat: true
                        label: "Bookmarks"
                        onClicked: toolbar.openBookmarks()
                    }

                    RmButton {
                        compact: true; flat: true
                        label: "Highlights"
                        onClicked: toolbar.openAnnotations()
                    }

                    RmButton {
                        compact: true; flat: true
                        label: "Aa"
                        onClicked: toolbar.openSettings()
                    }

                    RmButton {
                        compact: true; flat: true
                        label: "Search"
                        onClicked: toolbar.openSearch()
                    }
                }
            }
        }
    }

    Rectangle {
        id: bottomBar
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        height: Theme.readerBottomBarHeight
        color:  Theme.paper

        Rectangle {
            anchors.left:  parent.left
            anchors.right: parent.right
            anchors.top:   parent.top
            height: Theme.borderWidth
            color:  Theme.separator
        }

        Rectangle {
            id: progressTrack
            anchors.top:   parent.top
            anchors.left:  parent.left
            anchors.right: parent.right
            height: Theme.progressBarHeight
            color:  Theme.subtle

            Rectangle {
                height: parent.height
                width:  parent.width * Math.max(0, Math.min(100,
                            (typeof navController !== "undefined")
                            ? navController.progressPercent : 0)) / 100
                color:  Theme.ink
            }
        }

        Item {
            anchors.top:    progressTrack.bottom
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.bottom: parent.bottom

            Text {
                id: pageCounterText
                anchors.left:           parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin:     Theme.spaceMD
                text: (typeof readerView !== "undefined" && readerView.isLoaded)
                      ? ("Page " + readerView.currentPage + " of " + readerView.pageCount)
                      : ""
                font.pixelSize: Theme.textSM
                font.family:    Theme.uiFontFamily
                color:          Theme.ink
            }

            Item {
                id: chapterProgressRow
                anchors.right:          parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin:    Theme.spaceMD
                height: chapterProgressTitle.implicitHeight
                width:  Math.min(
                            parent.width - pageCounterText.width - Theme.spaceLG,
                            chapterProgressTitle.implicitWidth + chapterProgressSuffix.width)

                Text {
                    id: chapterProgressSuffix
                    anchors.right:          parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: (typeof navController !== "undefined")
                          ? (" · " + navController.chapterProgressPercent + "%")
                          : ""
                    font.pixelSize: Theme.textSM
                    font.family:    Theme.uiFontFamily
                    color:          Theme.ink
                }

                Text {
                    id: chapterProgressTitle
                    anchors.right:          chapterProgressSuffix.left
                    anchors.left:           parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment:    Text.AlignRight
                    text: (typeof navController !== "undefined")
                          ? navController.currentChapterTitle : ""
                    font.pixelSize: Theme.textSM
                    font.family:    Theme.uiFontFamily
                    color:          Theme.ink
                    elide:          Text.ElideLeft
                }
            }
        }
    }
}
