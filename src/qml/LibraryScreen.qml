import QtQuick
import QtQuick.Controls
import ReadMarkable

Item {
    id: libraryScreen
    anchors.fill: parent

    signal bookOpened(string bookPath)

    signal bookDetailRequested(string bookPath, string title, string author,
                               int progress, string chapter, var lastRead,
                               var addedDate, string description)

    signal openSettings()

    property bool   gridMode:            true

    property string currentFolderKey:    ""
    property string currentFolderName:   ""

    property var    folderHistory:       []

    property var    initialFolderHistory: []
    property string initialFolderKey:     ""
    property string initialFolderName:    ""
    signal navStateChanged(var history, string key, string name)

    function _emitNavState() {
        libraryScreen.navStateChanged(libraryScreen.folderHistory,
                                      libraryScreen.currentFolderKey,
                                      libraryScreen.currentFolderName)
    }

    property bool scanInProgress: false
    property bool scanDone:       false

    Connections {
        target: libraryModel
        function onScanFinished() {
            libraryScreen.scanInProgress = false
            libraryScreen.scanDone       = true

            if (typeof debugCtrl !== "undefined") {
                debugCtrl.libraryStats()
            }

            var pathVal = libraryModel.lastReadBookPath
            var wouldShow = (libraryScreen.scanDone
                             && libraryScreen.folderHistory.length === 0
                             && pathVal !== "")
            console.log("HERO eval: scanDone=" + libraryScreen.scanDone
                + " folderDepth=" + libraryScreen.folderHistory.length
                + " pathLen=" + pathVal.length
                + " -> " + (wouldShow ? "SHOW" : "HIDE"))

            Qt.callLater(libraryScreen.resetScrollPosition)
        }
    }

    function openBook(bookPath, title, author, progress, chapter, lastRead, addedDate, description) {
        libraryScreen.bookDetailRequested(bookPath, title || "", author || "", progress || 0,
                                         chapter || "", lastRead || null, addedDate || null,
                                         description || "")
    }

    function openFolder(folderKey, folderName) {
        var hist = libraryScreen.folderHistory.slice()
        hist.push({ key: libraryScreen.currentFolderKey, name: libraryScreen.currentFolderName })
        libraryScreen.folderHistory    = hist
        libraryScreen.currentFolderKey  = folderKey
        libraryScreen.currentFolderName = folderName || folderKey

        libraryScreen.scanInProgress = false
        libraryScreen.scanDone       = true
        libraryModel.setCurrentFolder(folderKey)
        resetScrollPosition()
        libraryScreen._emitNavState()
    }

    function goBack() {
        var hist = libraryScreen.folderHistory.slice()
        if (hist.length === 0) return
        var prev = hist.pop()
        libraryScreen.folderHistory    = hist
        libraryScreen.currentFolderKey  = prev.key
        libraryScreen.currentFolderName = prev.name

        libraryScreen.scanInProgress = false
        libraryScreen.scanDone       = true
        libraryModel.setCurrentFolder(prev.key)
        resetScrollPosition()
        libraryScreen._emitNavState()
    }

    function goToRoot() {
        libraryScreen.folderHistory    = []
        libraryScreen.currentFolderKey  = ""
        libraryScreen.currentFolderName = ""
        libraryScreen.scanInProgress    = false
        libraryScreen.scanDone          = true
        libraryModel.setCurrentFolder("")
        resetScrollPosition()
        libraryScreen._emitNavState()
    }

    function resetScrollPosition() {
        if (bookGrid) {
            bookGrid.positionViewAtBeginning()
            if (bookGrid.headerItem) {
                bookGrid.contentY = -bookGrid.headerItem.height
            }
        }
        if (bookList) {
            bookList.positionViewAtBeginning()
            if (bookList.headerItem) {
                bookList.contentY = -bookList.headerItem.height
            }
        }
    }

    onGridModeChanged: Qt.callLater(resetScrollPosition)

    onVisibleChanged: {
        if (visible) Qt.callLater(resetScrollPosition)
    }
    Component.onCompleted: {

        if (libraryModel.count() > 0) {
            libraryScreen.scanDone = true
        }

        libraryScreen.folderHistory    = libraryScreen.initialFolderHistory
        libraryScreen.currentFolderKey  = libraryScreen.initialFolderKey
        libraryScreen.currentFolderName = libraryScreen.initialFolderName
        libraryModel.setCurrentFolder(libraryScreen.currentFolderKey)

        Qt.callLater(resetScrollPosition)
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.paper
    }

    Item {
        id: headerBar
        anchors {
            left:  parent.left
            right: parent.right
            top:   parent.top
            topMargin: Theme.spaceXS
        }
        height: Theme.toolbarHeight

        RmButton {
            id: backButton
            anchors {
                left:           parent.left
                verticalCenter: parent.verticalCenter
                leftMargin:     Theme.spaceSM
            }
            label:   "< Back"
            visible: libraryScreen.folderHistory.length > 0
            onClicked: libraryScreen.goBack()
        }

        Text {
            id: headerTitle
            anchors {
                left:           libraryScreen.folderHistory.length > 0
                                    ? backButton.right : parent.left
                right:          headerControls.left
                verticalCenter: parent.verticalCenter
                leftMargin:     Theme.spaceSM
                rightMargin:    Theme.spaceSM
            }
            text: libraryScreen.currentFolderKey === ""
                    ? "Library"
                    : libraryScreen.currentFolderName
            font.pixelSize: Theme.textXL
            font.bold:      true
            font.family:    Theme.uiFontFamily
            color:          Theme.ink
            elide:          Text.ElideRight
        }

        Row {
            id: headerControls
            anchors {
                right:          parent.right
                verticalCenter: parent.verticalCenter
                rightMargin:    Theme.spaceLG
            }
            spacing: Theme.spaceXS

            Rectangle {
                width:  sortDropdownText.implicitWidth + Theme.spaceLG * 2
                height: Theme.touchTarget
                color:  sortDropdownArea.containsPress ? Theme.ink : Theme.paper
                border.color: Theme.separator
                border.width: Theme.borderWidth
                radius: Theme.borderRadius

                Text {
                    id: sortDropdownText
                    anchors.centerIn: parent
                    text: {
                        var labels = ["Recent", "Added", "Title", "Author"]
                        var mode = librarySortProxy ? librarySortProxy.sortMode : 0
                        return labels[mode] + " v"
                    }
                    font.pixelSize:  Theme.textSM
                    font.family:     Theme.uiFontFamily
                    color:           sortDropdownArea.containsPress ? Theme.paper : Theme.ink
                }
                MouseArea {
                    id: sortDropdownArea
                    anchors.fill: parent
                    onClicked: sortMenu.visible = !sortMenu.visible
                }
            }

            Rectangle {
                width:  viewToggleText.implicitWidth + Theme.spaceLG
                height: Theme.touchTarget
                color:  Theme.paper
                border.color: Theme.separator
                border.width: Theme.borderWidth
                radius: Theme.borderRadius

                Text {
                    id: viewToggleText
                    anchors.centerIn: parent
                    text:            libraryScreen.gridMode ? "Grid" : "List"
                    font.pixelSize:  Theme.textSM
                    font.family:     Theme.uiFontFamily
                    color:           Theme.ink
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked:    libraryScreen.gridMode = !libraryScreen.gridMode
                }
            }

            Rectangle {
                width:  Theme.touchTarget
                height: Theme.touchTarget
                color:  settingsArea.containsPress ? Theme.ink : Theme.paper
                border.color: Theme.separator
                border.width: Theme.borderWidth
                radius: Theme.borderRadius

                Text {
                    anchors.centerIn: parent
                    text:            "Aa"
                    font.pixelSize:  Theme.textMD
                    font.bold:       true
                    font.family:     Theme.uiFontFamily
                    color:           settingsArea.containsPress ? Theme.paper : Theme.ink
                }
                MouseArea {
                    id: settingsArea
                    anchors.fill: parent
                    onClicked:    libraryScreen.openSettings()
                }
            }

            Rectangle {
                width:  Theme.touchTarget
                height: Theme.touchTarget
                color:  quitArea.containsPress ? Theme.ink : Theme.paper
                border.color: Theme.separator
                border.width: Theme.borderWidth
                radius: Theme.borderRadius

                Image {
                    anchors.centerIn: parent
                    source:           "qrc:/icons/cross.svg"
                    sourceSize.width:  Theme.textLG
                    sourceSize.height: Theme.textLG
                    width:             Theme.textLG
                    height:            Theme.textLG
                    smooth:            false
                }
                MouseArea {
                    id: quitArea
                    anchors.fill: parent
                    onClicked:    debugCtrl.quitApp()
                }
            }
        }

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: Theme.spaceHair
            color:  Theme.separator
        }
    }

    Rectangle {
        id: sortMenu
        visible: false
        z: 10
        anchors {
            right:       parent.right
            top:         headerBar.bottom
            rightMargin: Theme.spaceLG
        }
        width:  Theme.sortMenuWidth
        height: sortMenuColumn.implicitHeight + Theme.spaceSM * 2
        color:  Theme.paper
        border.color: Theme.separator
        border.width: Theme.borderWidth - 1
        radius: Theme.borderRadius

        Column {
            id: sortMenuColumn
            anchors {
                left:    parent.left
                right:   parent.right
                top:     parent.top
                margins: Theme.spaceSM
            }

            Repeater {
                model: [
                    { label: "Recent",  mode: 0 },
                    { label: "Added",   mode: 1 },
                    { label: "Title",   mode: 2 },
                    { label: "Author",  mode: 3 }
                ]
                delegate: Rectangle {
                    width:  parent.width
                    height: Theme.touchTarget
                    color:  (librarySortProxy && librarySortProxy.sortMode === modelData.mode)
                            ? Theme.subtle : Theme.paper

                    Text {
                        anchors {
                            left:           parent.left
                            leftMargin:     Theme.spaceMD
                            verticalCenter: parent.verticalCenter
                        }
                        text:           modelData.label
                        font.pixelSize: Theme.textMD
                        font.family:    Theme.uiFontFamily
                        font.bold:      librarySortProxy && librarySortProxy.sortMode === modelData.mode
                        color:          Theme.ink
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            librarySortProxy.setSortMode(modelData.mode)
                            sortMenu.visible = false
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        visible: sortMenu.visible
        z: 9
        onClicked: sortMenu.visible = false
    }

    Item {
        id:      breadcrumbBar
        anchors {
            left:  parent.left
            right: parent.right
            top:   headerBar.bottom
        }
        height:  libraryScreen.currentFolderKey !== "" ? Theme.spaceLG : 0
        visible: libraryScreen.currentFolderKey !== ""
        clip:    true

        Row {
            anchors {
                left:           parent.left
                verticalCenter: parent.verticalCenter
                leftMargin:     Theme.spaceMD
            }
            spacing: Theme.spaceMicro

            Text {
                text:            "Library"
                font.pixelSize:  Theme.textXS
                color:           Theme.faint
                anchors.verticalCenter: parent.verticalCenter
                MouseArea {
                    anchors.fill: parent
                    onClicked:    libraryScreen.goToRoot()
                }
            }

            Repeater {
                model: {
                    if (libraryScreen.currentFolderKey === "") return []
                    var crumbs = []
                    for (var i = 0; i < libraryScreen.folderHistory.length; i++) {
                        var h = libraryScreen.folderHistory[i]
                        if (h.key !== "") {
                            crumbs.push(h)
                        }
                    }
                    crumbs.push({ key: libraryScreen.currentFolderKey,
                                  name: libraryScreen.currentFolderName })
                    return crumbs
                }
                delegate: Row {
                    spacing: Theme.spaceMicro
                    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

                    Text {
                        text:            " > "
                        font.pixelSize:  Theme.textXS
                        color:           Theme.muted
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        readonly property int lastIdx: {
                            var c = 0
                            for (var i = 0; i < libraryScreen.folderHistory.length; i++) {
                                if (libraryScreen.folderHistory[i].key !== "") c++
                            }
                            return c
                        }
                        text:            modelData.name || modelData.key
                        font.pixelSize:  Theme.textXS
                        font.bold:       index === lastIdx
                        color:           index === lastIdx ? Theme.ink : Theme.faint
                        anchors.verticalCenter: parent.verticalCenter

                        MouseArea {
                            anchors.fill: parent

                            enabled: index < parent.lastIdx
                            onClicked: {

                                var targetKey  = modelData.key
                                var targetName = modelData.name
                                var newHist = []
                                var histFiltered = []
                                for (var i = 0; i < libraryScreen.folderHistory.length; i++) {
                                    if (libraryScreen.folderHistory[i].key !== "") {
                                        histFiltered.push(libraryScreen.folderHistory[i])
                                    }
                                }
                                for (var j = 0; j < index; j++) {
                                    newHist.push(histFiltered[j])
                                }
                                libraryScreen.folderHistory    = newHist
                                libraryScreen.currentFolderKey  = targetKey
                                libraryScreen.currentFolderName = targetName
                                libraryScreen.scanInProgress    = false
                                libraryScreen.scanDone          = true
                                libraryModel.setCurrentFolder(targetKey)
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: Theme.spaceHair
            color:  Theme.subtle
        }
    }

    readonly property bool heroVisible: libraryScreen.scanDone
                                       && libraryScreen.folderHistory.length === 0
                                       && libraryModel.lastReadBookPath !== ""

    Item {
        id:      continueCard
        anchors {
            left:        parent.left
            right:       parent.right
            top:         breadcrumbBar.bottom

            leftMargin:  Theme.spaceMD
            rightMargin: Theme.spaceMD
            topMargin:   libraryScreen.heroVisible ? Theme.spaceMD : 0
        }

        height:  libraryScreen.heroVisible ? Theme.continueHeroHeight : 0
        visible: height > 0
        clip:    true

        Rectangle {
            anchors.fill: parent
            color:        Theme.paper
            border.color: Theme.panelBorder
            border.width: Theme.borderWidth
        }

        Row {
            anchors {
                left:           parent.left
                right:          parent.right
                verticalCenter: parent.verticalCenter
                leftMargin:     Theme.spaceMD
                rightMargin:    Theme.spaceMD
            }
            spacing: Theme.spaceMD

            Image {
                id:       continueThumb
                width:    Theme.continueHeroThumbWidth
                height:   Theme.continueHeroThumbHeight
                source:   libraryModel.lastReadBookPath !== ""
                            ? "image://cover/" + encodeURIComponent(libraryModel.lastReadBookPath)
                            : ""
                fillMode: Image.PreserveAspectCrop
                cache:    true
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    anchors.fill: parent
                    color:        Theme.subtle
                    border.width: Theme.borderWidth - 1
                    border.color: Theme.ink
                    visible:      continueThumb.status !== Image.Ready
                }
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spaceXXS

                width: parent.width - continueThumb.width - resumeButton.width
                           - parent.spacing * 2

                Text {
                    text:            "Continue reading"
                    font.pixelSize:  Theme.continueHeroChapterSize
                    font.family:     Theme.uiFontFamily
                    color:           Theme.muted
                    width:           parent.width
                    elide:           Text.ElideRight
                }
                Text {
                    text:            libraryModel.lastReadBookTitle
                    font.pixelSize:  Theme.continueHeroTitleSize
                    font.bold:       true
                    font.family:     Theme.uiFontFamily
                    color:           Theme.ink
                    width:           parent.width
                    elide:           Text.ElideRight
                }
                Text {
                    text:            libraryModel.lastReadBookCurrentChapter
                    font.pixelSize:  Theme.continueHeroChapterSize
                    font.family:     Theme.uiFontFamily
                    color:           Theme.muted
                    width:           parent.width
                    elide:           Text.ElideRight
                    visible:         libraryModel.lastReadBookCurrentChapter !== ""
                }

                Rectangle {
                    width:  parent.width
                    height: Theme.libraryListProgressBarHeight
                    color:  Theme.subtle

                    Rectangle {
                        width:  Math.floor(parent.width * (libraryModel.lastReadBookProgress / 100))
                        height: parent.height
                        color:  Theme.ink
                    }
                }
                Text {
                    text:            libraryModel.lastReadBookProgress + "%"
                    font.pixelSize:  Theme.continueHeroChapterSize
                    font.family:     Theme.uiFontFamily
                    color:           Theme.dimmed
                }
            }

            RmButton {
                id: resumeButton
                z:  2
                anchors.verticalCenter: parent.verticalCenter
                label:    "Resume"
                onClicked: libraryScreen.bookOpened(libraryModel.lastReadBookPath)
            }
        }

        TapHandler {
            onTapped: {
                if (libraryModel.lastReadBookPath !== "") {
                    libraryScreen.bookDetailRequested(
                        libraryModel.lastReadBookPath,
                        libraryModel.lastReadBookTitle,
                        "",
                        libraryModel.lastReadBookProgress,
                        libraryModel.lastReadBookCurrentChapter,
                        null,
                        null,
                        ""
                    )
                }
            }
        }

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: Theme.spaceHair
            color:  Theme.subtle
        }
    }

    Component {
        id: folderHeaderComponent
        Item {
            id: folderHeaderRoot

            width:  (folderHeaderRoot.ListView.view
                        ? folderHeaderRoot.ListView.view.width
                        : (folderHeaderRoot.GridView.view
                               ? folderHeaderRoot.GridView.view.width
                               : 0))

            height: folderHeaderGrid.implicitHeight > 0
                    ? folderHeaderGrid.implicitHeight + Theme.spaceSM * 2
                    : 0
            visible: folderHeaderGrid.implicitHeight > 0

            Grid {
                id: folderHeaderGrid
                anchors {
                    left:        parent.left
                    right:       parent.right
                    top:         parent.top
                    topMargin:   Theme.spaceSM

                    leftMargin:  0
                    rightMargin: 0
                }
                columns:       Theme.libraryFolderHeaderColumns
                rowSpacing:    Theme.spaceXS
                columnSpacing: Theme.spaceXS

                readonly property int cellW: Math.floor(
                    (width - columnSpacing * (columns - 1)) / columns)

                Repeater {
                    model: folderSortProxy
                    delegate: Rectangle {
                        id:     folderCell
                        width:  folderHeaderGrid.cellW
                        height: Theme.folderPillHeight
                        color:  Theme.subtle
                        radius: Theme.folderPillRadius
                        border.width: 0

                        property bool isEmpty: (model.isEmpty === true)

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left:           parent.left
                            anchors.leftMargin:     Theme.folderPillHPadding
                            spacing:                Theme.spaceSM

                            Image {
                                source:            folderCell.isEmpty
                                                     ? "qrc:/icons/folder-outlined.svg"
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
                                font.pixelSize: Theme.textSM
                                font.family:    Theme.uiFontFamily
                                color:          Theme.ink
                                elide:          Text.ElideRight
                                width:          folderHeaderGrid.cellW
                                                    - Theme.folderPillHPadding * 2
                                                    - Theme.folderPillGlyphSize
                                                    - Theme.spaceSM
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                var key = (model.xochitlUuid !== ""
                                           && model.xochitlUuid !== undefined)
                                    ? model.xochitlUuid : model.folderPath
                                libraryScreen.openFolder(key, model.title || "")
                            }
                        }
                    }
                }
            }
        }
    }

    Item {
        id:     bookViewContainer
        anchors {
            left:         parent.left
            right:        parent.right
            top:          continueCard.bottom

            topMargin:    libraryScreen.heroVisible ? Theme.spaceMD : 0
            bottom:       parent.bottom
            bottomMargin: Theme.spaceMD
        }

        Text {
            anchors.centerIn: parent
            text:            "Scanning library..."
            font.pixelSize:  Theme.textLG
            color:           Theme.muted
            visible:         libraryScreen.scanInProgress
        }

        Text {
            anchors.centerIn: parent
            text: libraryScreen.currentFolderKey === ""
                    ? "No books found.\n\nAdd EPUB files to:\n/home/root/.local/share/readmarkable/books"
                    : "This folder is empty."
            font.pixelSize:  Theme.textLG
            color:           Theme.muted
            horizontalAlignment: Text.AlignHCenter
            visible:  libraryScreen.scanDone && librarySortProxy.count === 0
            wrapMode: Text.WordWrap
            width:    Math.min(parent.width - Theme.spaceXL * 2, parent.width * 0.6)
        }

        GridView {
            id:           bookGrid
            anchors {
                fill:        parent
                leftMargin:  Theme.spaceMD
                rightMargin: Theme.spaceMD
                topMargin:   Theme.spaceSM
            }
            visible:      libraryScreen.gridMode
            model:        bookSortProxy
            delegate:     BookGridCard {}

            header: folderHeaderComponent

            readonly property int columns: renderer.isLandscape ? Theme.folderGridColumns + 1 : Theme.folderGridColumns
            cellWidth:  Math.floor(width / columns)
            cellHeight: Math.floor(cellWidth * Theme.libraryGridCellAspect)

            interactive:       true
            cacheBuffer:       cellHeight * 2
            clip:              true
            boundsBehavior:    Flickable.StopAtBounds
            flickDeceleration: 5000

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
                width: Theme.spaceXXS
            }
        }

        ListView {
            id:           bookList
            anchors {
                fill:        parent
                leftMargin:  Theme.spaceMD
                rightMargin: Theme.spaceMD
            }
            visible:      !libraryScreen.gridMode
            model:        bookSortProxy
            delegate:     BookListCard {}

            header: folderHeaderComponent

            interactive:       true
            cacheBuffer:       Theme.libraryListRowHeight * 2
            clip:              true
            boundsBehavior:    Flickable.StopAtBounds
            flickDeceleration: 5000

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
                width: Theme.spaceXXS
            }
        }
    }
}
