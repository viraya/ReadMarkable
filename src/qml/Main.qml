import QtQuick
import "diagnostic"

Rectangle {
    id: root
    width:  renderer.isLandscape ? 1696 : 954
    height: renderer.isLandscape ? 954  : 1696
    color: "white"

    property string screenState:     "library"
    property string currentBookPath: ""

    property bool   bookRemovedDialogVisible: false

    Connections {
        target: libraryModel
        function onCurrentBookExternallyDeleted(filePath) {

            if (root.screenState === "reader") {
                renderer.closeDocument()
            }
            root.screenState             = "library"
            root.bookRemovedDialogVisible = true
        }
    }

    Item {
        anchors.fill: parent
        visible: root.bookRemovedDialogVisible
        z: 100

        Rectangle {
            anchors.fill: parent
            color:        Theme.overlayDim
            MouseArea {
                anchors.fill: parent

            }
        }

        RmPanel {
            width:           Math.round(parent.width * Theme.popupWidthFraction)
            height:          dialogColumn.implicitHeight + Theme.spaceLG * 2
            anchors.centerIn: parent

            Column {
                id: dialogColumn
                anchors {
                    left:    parent.left
                    right:   parent.right
                    top:     parent.top
                    margins: Theme.spaceLG
                }
                spacing: Theme.spaceMD

                Text {
                    width:               parent.width
                    text:                "Book removed"
                    font.pixelSize:      Theme.textLG
                    font.bold:           true
                    font.family:         Theme.uiFontFamily
                    color:               Theme.ink
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    width:               parent.width
                    text:                "This book was removed from the device."
                    font.pixelSize:      Theme.textMD
                    font.family:         Theme.uiFontFamily
                    color:               Theme.muted
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode:            Text.WordWrap
                }

                RmButton {
                    label:  "OK"
                    width:  parent.width
                    height: Theme.bookDetailOkButtonHeight
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: root.bookRemovedDialogVisible = false
                }
            }
        }
    }

    property var    savedFolderHistory: []
    property string savedFolderKey:     ""
    property string savedFolderName:    ""

    property string detailBookPath:    ""
    property string detailTitle:       ""
    property string detailAuthor:      ""
    property int    detailProgress:    0
    property string detailChapter:     ""
    property var    detailLastRead:    null
    property var    detailAddedDate:   null
    property string detailDescription: ""

    Loader {
        id: libraryLoader
        anchors.fill: parent
        active:  root.screenState === "library"
        visible: active
        sourceComponent: LibraryScreen {
            initialFolderHistory: root.savedFolderHistory
            initialFolderKey:     root.savedFolderKey
            initialFolderName:    root.savedFolderName
            onNavStateChanged: (history, key, name) => {
                root.savedFolderHistory = history
                root.savedFolderKey     = key
                root.savedFolderName    = name
            }
        }
    }

    Connections {
        target:  libraryLoader.item
        enabled: libraryLoader.active
        function onBookDetailRequested(bookPath, title, author, progress, chapter, lastRead, addedDate, description) {
            root.detailBookPath    = bookPath
            root.detailTitle       = title
            root.detailAuthor      = author
            root.detailProgress    = progress
            root.detailChapter     = chapter
            root.detailLastRead    = lastRead
            root.detailAddedDate   = addedDate
            root.detailDescription = description
            root.screenState       = "detail"
        }
    }

    Connections {
        target:  libraryLoader.item
        enabled: libraryLoader.active
        function onBookOpened(bookPath) {
            root.currentBookPath = bookPath
            renderer.loadDocument(bookPath)
            root.screenState = "reader"
        }
    }

    Connections {
        target:  libraryLoader.item
        enabled: libraryLoader.active
        function onOpenSettings() {
            root.screenState = "globalSettings"
        }
    }

    Loader {
        id: detailLoader
        anchors.fill: parent
        active:  root.screenState === "detail"
        visible: active
        sourceComponent: BookDetailScreen {
            bookPath:        root.detailBookPath
            bookTitle:       root.detailTitle
            bookAuthor:      root.detailAuthor
            bookProgress:    root.detailProgress
            bookChapter:     root.detailChapter
            bookLastRead:    root.detailLastRead
            bookAddedDate:   root.detailAddedDate
            bookDescription: root.detailDescription
        }
    }

    Connections {
        target:  detailLoader.item
        enabled: detailLoader.active
        function onOpenBook(bookPath) {
            root.currentBookPath = bookPath
            renderer.loadDocument(bookPath)
            root.screenState = "reader"
        }
    }

    Connections {
        target:  detailLoader.item
        enabled: detailLoader.active
        function onGoBack() {
            root.screenState = "library"
        }
    }

    Loader {
        id: readerLoader
        anchors.fill: parent
        active:  root.screenState === "reader"
        visible: active
        sourceComponent: ReaderView {
            currentPage:   renderer.currentPage
            pageCount:     renderer.pageCount
            isLoaded:      renderer.isLoaded
            renderVersion: renderer.renderVersion
        }
    }

    Connections {
        target:  readerLoader.item
        enabled: readerLoader.active
        function onGoBack() {
            renderer.closeDocument()

            root.screenState = "library"
            libraryModel.refreshPositions()
        }
    }

    Loader {
        id: globalSettingsLoader
        anchors.fill: parent
        active:  root.screenState === "globalSettings"
        visible: active
        source:  "GlobalSettingsScreen.qml"
    }

    Connections {
        target:  globalSettingsLoader.item
        enabled: globalSettingsLoader.active
        function onGoBack() {
            root.screenState = "library"
        }
    }

    property bool showDiag: false
    Loader {
        anchors.fill: parent
        active: root.showDiag
        sourceComponent: FullDiagScreen {}
    }

    ReturningSplash {
        id: returningSplash
    }

    Connections {
        target: debugCtrl
        function onAboutToQuit() {
            returningSplash.show()
        }
    }
}
