import QtQuick 2.15
import ReadMarkable

Item {
    id: readerView
    anchors.fill: parent

    signal goBack()

    property int  currentPage:    0
    property int  pageCount:      0
    property bool isLoaded:       false
    property int  renderVersion:  0

    property bool selectionMode: false

    property real swipeStartX:    0
    property real swipeStartY:    0
    property bool swipeTracking:  false
    property bool swipeConsumed:  false

    property real longPressX: 0
    property real longPressY: 0
    readonly property real longPressMoveThreshold: 20

    readonly property real swipeThreshold:   width * 0.15
    readonly property real swipeMaxVertical: height * 0.30

    property int marginH: (typeof settingsController !== "undefined") ? settingsController.marginPreset === 0 ? 24
                          : settingsController.marginPreset === 2 ? 80 : 48 : 48
    property int marginV: (typeof settingsController !== "undefined") ? settingsController.marginPreset === 0 ? 32
                          : settingsController.marginPreset === 2 ? 100 : 64 : 64

    readonly property real contentW: width  - 2 * marginH
    readonly property real contentH: height - 2 * marginV

    readonly property real coordScaleX: width  / contentW
    readonly property real coordScaleY: height / contentH

    Rectangle {
        anchors.fill: parent
        color: Theme.paper
    }

    Image {
        id: pageImage
        x: readerView.marginH
        y: readerView.marginV
        width:  readerView.width  - 2 * readerView.marginH
        height: readerView.height - 2 * readerView.marginV
        source: readerView.isLoaded
                    ? "image://reader/page/" + readerView.currentPage + "?v=" + readerView.renderVersion
                    : ""
        cache: false
        fillMode: Image.Stretch
        asynchronous: false
    }

    Rectangle {
        anchors.centerIn: parent
        width:   Theme.loadingBoxWidth
        height:  Theme.loadingBoxHeight
        visible: !readerView.isLoaded
        color:   Theme.paper

        Text {
            anchors.centerIn: parent
            text:           "Loading..."
            font.pixelSize: Theme.textLG
            font.family:    Theme.uiFontFamily
            color:          Theme.ink
        }
    }

    property bool toolbarVisible: false

    Timer {
        id: longPressTimer
        interval: 600
        repeat: false
        onTriggered: {
            if (!readerView.isLoaded) return
            if (typeof selectionController === "undefined") return
            if (searchPanel.isOpen ||
                tocPanel.visible || bookmarkPanel.visible ||
                annotationsPanel.visible ||
                settingsSheet.visible || footnotePopup.isOpen ||
                linkPreviewPopup.isOpen || dictionaryPopup.isOpen)
                return

            var imgX = readerView.longPressX - readerView.marginH
            var imgY = readerView.longPressY - readerView.marginV

            if (imgX < 0 || imgY < 0 || imgX >= readerView.contentW || imgY >= readerView.contentH)
                return

            var contentX = imgX * readerView.coordScaleX
            var contentY = imgY * readerView.coordScaleY
            selectionController.startSelectionAt(contentX, contentY)
            readerView.selectionMode = selectionController.hasSelection
            readerView.swipeConsumed = true
        }
    }

    MouseArea {
        id: tapArea
        anchors.fill: parent

        onPressed: function(mouse) {
            readerView.swipeStartX   = mouse.x
            readerView.swipeStartY   = mouse.y
            readerView.swipeTracking = true
            readerView.swipeConsumed = false

            readerView.longPressX = mouse.x
            readerView.longPressY = mouse.y
            longPressTimer.restart()
        }

        onPositionChanged: function(mouse) {

            var dx = mouse.x - readerView.longPressX
            var dy = mouse.y - readerView.longPressY
            if (dx * dx + dy * dy > readerView.longPressMoveThreshold * readerView.longPressMoveThreshold)
                longPressTimer.stop()
        }

        onReleased: function(mouse) {
            longPressTimer.stop()
            if (!readerView.swipeTracking) return
            readerView.swipeTracking = false

            if (searchPanel.isOpen    ||
                tocPanel.visible      || bookmarkPanel.visible ||
                annotationsPanel.visible ||
                settingsSheet.visible || footnotePopup.isOpen  ||
                linkPreviewPopup.isOpen || dictionaryPopup.isOpen)
                return

            if (readerView.selectionMode) return

            if (!readerView.isLoaded) return
            if (typeof navController === "undefined") return

            var deltaX = mouse.x - readerView.swipeStartX
            var deltaY = mouse.y - readerView.swipeStartY

            if (Math.abs(deltaX) > readerView.swipeThreshold
                    && Math.abs(deltaY) < readerView.swipeMaxVertical) {
                readerView.swipeConsumed = true
                if (deltaX < 0)
                    navController.nextPage()
                else
                    navController.previousPage()
            }
        }

        onClicked: function(mouse) {

            if (readerView.swipeConsumed) {
                readerView.swipeConsumed = false
                return
            }

            if (searchPanel.isOpen) {
                searchPanel.dismiss()
                return
            }

            if (readerView.selectionMode) {
                if (typeof selectionController !== "undefined")
                    selectionController.clearSelection()
                readerView.selectionMode = false
                return
            }

            if (dictionaryPopup.isOpen) {
                dictionaryPopup.dismiss()
                return
            }
            if (footnotePopup.isOpen) {
                footnotePopup.dismiss()
                return
            }
            if (linkPreviewPopup.isOpen) {
                linkPreviewPopup.dismiss()
                return
            }

            if (readerView.toolbarVisible) {
                readerView.toolbarVisible = false
                return
            }

            if (readerView.isLoaded && typeof navController !== "undefined") {
                var linkImgX = mouse.x - readerView.marginH
                var linkImgY = mouse.y - readerView.marginV

                if (linkImgX >= 0 && linkImgY >= 0
                        && linkImgX < readerView.contentW && linkImgY < readerView.contentH) {
                    var href = navController.checkLink(
                        linkImgX * readerView.coordScaleX,
                        linkImgY * readerView.coordScaleY)
                    if (href !== "") {
                        var targetText = navController.getLinkTargetText(href)
                        if (href.startsWith("#") && targetText !== "") {

                            footnotePopup.show(targetText, href)
                        } else {

                            var preview = targetText !== "" ? targetText : "Navigate to link target"
                            linkPreviewPopup.show(preview, href)
                        }
                        return
                    }
                }
            }

            const zone = mouse.x / readerView.width

            if (zone < 0.40) {
                if (readerView.isLoaded && typeof navController !== "undefined")
                    navController.previousPage()
            } else if (zone > 0.60) {
                if (readerView.isLoaded && typeof navController !== "undefined")
                    navController.nextPage()
            } else {

                readerView.toolbarVisible = true
            }
        }
    }

    HighlightOverlay {
        id: highlightOverlay
        x: readerView.marginH
        y: readerView.marginV
        width:  readerView.width  - 2 * readerView.marginH
        height: readerView.height - 2 * readerView.marginV
    }

    PenStrokeOverlay {
        id: penStrokeOverlay
        x: readerView.marginH
        y: readerView.marginV
        width:  readerView.width  - 2 * readerView.marginH
        height: readerView.height - 2 * readerView.marginV
    }

    Repeater {
        id: marginNoteIconRepeater
        model: (typeof marginNoteService !== "undefined") ? marginNoteService.currentPageNotes : []

        MarginNoteIcon {

            x: readerView.width - readerView.marginH + 4

            y: readerView.marginV + (modelData.paragraphY / readerView.coordScaleY)
            noteId:             modelData.id
            strokeData:         modelData.strokeData
            paragraphXPointer:  modelData.paragraphXPointer
            paragraphY:         modelData.paragraphY

            onIconTapped: function(nid, sdata, xptr, py) {
                var canvasW = readerView.marginH - 8
                var canvasH = 200
                marginNoteCanvas.x = readerView.width - readerView.marginH + 2
                marginNoteCanvas.y = readerView.marginV + (py / readerView.coordScaleY)
                marginNoteCanvas.width  = canvasW
                marginNoteCanvas.height = canvasH
                marginNoteCanvas.show(nid, sdata, xptr, py, canvasW, canvasH)
            }
        }
    }

    SelectionOverlay {
        id: selectionOverlay
        x: readerView.marginH
        y: readerView.marginV
        width:  readerView.width  - 2 * readerView.marginH
        height: readerView.height - 2 * readerView.marginV
    }

    SelectionHandles {
        id: selectionHandles
        x: readerView.marginH
        y: readerView.marginV
        width:  readerView.width  - 2 * readerView.marginH
        height: readerView.height - 2 * readerView.marginV
    }

    SelectionToolbar {
        id: selectionToolbar

        visible: readerView.selectionMode
                 && (typeof selectionController !== "undefined")
                 && selectionController.hasSelection
                 && readerView.isLoaded

        x: Math.max(0,
                    Math.min(readerView.width - width,
                             readerView.marginH + (readerView.width - 2 * readerView.marginH - width) / 2))

        y: {
            if (typeof selectionController === "undefined" || !selectionController.hasSelection)
                return 0

            var scaledEndY   = selectionController.endHandleY   / readerView.coordScaleY
            var scaledStartY = selectionController.startHandleY / readerView.coordScaleY

            var selBottom = readerView.marginV + scaledEndY
            var belowY    = selBottom + 48
            var aboveY    = readerView.marginV + scaledStartY - height - 48

            if (belowY + height <= readerView.height)
                return belowY
            else
                return Math.max(0, aboveY)
        }

        property int pendingAnnotationId: 0

        onHighlightRequested: function(style) {
            if (typeof selectionController === "undefined") return
            if (typeof annotationService === "undefined") return

            var activeId = selectionController.activeAnnotationId()
            if (activeId > 0) {
                var existingNoteForStyle = ""
                var annsForStyle = annotationService.annotationsForCurrentBook()
                for (var k = 0; k < annsForStyle.length; ++k) {
                    if (annsForStyle[k].id === activeId) {
                        existingNoteForStyle = annsForStyle[k].note || ""
                        break
                    }
                }
                annotationService.updateAnnotation(activeId, style, existingNoteForStyle)
            } else {

                var chapterTitle = (typeof navController !== "undefined")
                                   ? navController.currentChapterTitle : ""
                annotationService.saveHighlight(
                    selectionController.startXPointer,
                    selectionController.endXPointer,
                    selectionController.selectedText,
                    style,
                    chapterTitle)
            }

            selectionController.loadHighlightsForPage()

        }

        onDictionaryRequested: {
            if (typeof dictionaryService === "undefined") return
            if (typeof selectionController === "undefined") return
            var word = selectionController.selectedText.trim()
            if (word === "") return
            dictionaryPopup.show(word)
            dictionaryService.lookup(word)

        }

        onCopyRequested: {

            if (typeof selectionController !== "undefined") {
                var textToCopy = selectionController.selectedText

                if (typeof clipboard !== "undefined")
                    clipboard.setText(textToCopy)
                else
                    console.log("Copy:", textToCopy)

                selectionController.clearSelection()
                readerView.selectionMode = false
            }
        }

        onNoteRequested: {
            if (typeof selectionController === "undefined") return

            var existingNote = ""
            if (typeof annotationService !== "undefined") {
                var activeId2 = selectionController.activeAnnotationId()
                if (activeId2 > 0) {
                    var anns = annotationService.annotationsForCurrentBook()
                    for (var i = 0; i < anns.length; ++i) {
                        if (anns[i].id === activeId2) {
                            existingNote = anns[i].note || ""
                            break
                        }
                    }
                }
                selectionToolbar.pendingAnnotationId = activeId2
            }
            noteEditor.show(existingNote)
        }
    }

    NoteEditor {
        id: noteEditor

        onNoteSaved: function(noteText) {
            if (typeof annotationService === "undefined") return
            if (typeof selectionController === "undefined") return

            var targetId = selectionToolbar.pendingAnnotationId
            if (targetId > 0) {

                var currentStyle = 0
                var annsForNote = annotationService.annotationsForCurrentBook()
                for (var j = 0; j < annsForNote.length; ++j) {
                    if (annsForNote[j].id === targetId) {
                        currentStyle = annsForNote[j].style || 0
                        break
                    }
                }
                annotationService.updateAnnotation(targetId, currentStyle, noteText)
            } else {

                var chapterTitle2 = (typeof navController !== "undefined")
                                    ? navController.currentChapterTitle : ""
                var newId = annotationService.saveHighlight(
                    selectionController.startXPointer,
                    selectionController.endXPointer,
                    selectionController.selectedText,
                    0,
                    chapterTitle2)
                if (newId > 0)
                    annotationService.updateAnnotation(newId, 0, noteText)
            }

            selectionController.loadHighlightsForPage()
            selectionController.clearSelection()
            readerView.selectionMode = false
        }

        onNoteCancelled: {

        }
    }

    SearchPanel {
        id: searchPanel

        onSearchNavigated: function(query) {

            searchIndicator.pendingQuery = query
            searchHighlightTimer.restart()
        }
    }

    Timer {
        id: searchHighlightTimer
        interval: 300
        repeat: false
        onTriggered: {
            if (searchIndicator.pendingQuery !== "")
                searchIndicator.showForQuery(searchIndicator.pendingQuery)
        }
    }

    Rectangle {
        id: searchIndicator
        anchors.top:   parent.top
        anchors.left:  parent.left
        anchors.right: parent.right
        height: visible ? Theme.toolbarHeight * 0.7 : 0
        visible: false
        z: 90
        color: Theme.paper

        property string queryText: ""
        property string pendingQuery: ""

        function showForQuery(query) {
            queryText = query
            pendingQuery = ""
            visible = true

            if (typeof searchService !== "undefined" && typeof renderer !== "undefined") {
                var rects = searchService.findTextRects(query, renderer)
                console.log("findTextRects returned", rects.length, "rects for", query)
                searchHighlightRepeater.model = rects
            }
        }

        function dismiss() {
            visible = false
            searchHighlightRepeater.model = []
        }

        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Theme.borderWidth; color: Theme.separator
        }

        Row {
            anchors.fill: parent
            anchors.leftMargin: Theme.spaceMD
            anchors.rightMargin: Theme.spaceSM
            spacing: Theme.spaceSM

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Find: " + searchIndicator.queryText
                font.pixelSize: Theme.textSM
                font.family: Theme.uiFontFamily
                font.bold: true
                color: Theme.ink
                elide: Text.ElideRight
                width: parent.width - dismissIndicatorBtn.width - parent.spacing
            }

            Item {
                id: dismissIndicatorBtn
                width: Theme.touchTarget * 0.7
                height: parent.height

                Rectangle {
                    anchors.fill: parent
                    color: dismissIndicatorArea.containsPress ? Theme.ink : Theme.paper
                }

                Text {
                    anchors.centerIn: parent
                    text: "X"; font.pixelSize: Theme.textSM
                    font.family: Theme.uiFontFamily; font.bold: true
                    color: dismissIndicatorArea.containsPress ? Theme.paper : Theme.ink
                }

                MouseArea {
                    id: dismissIndicatorArea
                    anchors.fill: parent
                    onClicked: searchIndicator.dismiss()
                }
            }
        }
    }

    Repeater {
        id: searchHighlightRepeater
        model: []

        Rectangle {
            x: readerView.marginH + (modelData.x / readerView.coordScaleX)

            y: readerView.marginV + ((modelData.y + modelData.height) / readerView.coordScaleY) - 3
            width:  modelData.width  / readerView.coordScaleX
            height: Theme.searchHighlightHeight
            color: Theme.ink
            z: 50
        }
    }

    Connections {
        target: (typeof renderer !== "undefined") ? renderer : null
        function onCurrentPageChanged() {
            if (searchIndicator.pendingQuery === "")
                searchIndicator.dismiss()
        }
    }

    Toolbar {
        id: toolbar
        visible: readerView.toolbarVisible && readerView.isLoaded

        onOpenToc: {
            tocPanel.open()
        }
        onOpenBookmarks: {
            bookmarkPanel.open()
        }
        onOpenAnnotations: {
            annotationsPanel.open()
        }
        onOpenSettings: {
            settingsSheet.open()
        }
        onOpenSearch: {
            searchPanel.show()
            readerView.toolbarVisible = false
        }
        onDismiss: {
            readerView.toolbarVisible = false
        }
        onGoToLibrary: {
            readerView.goBack()
        }
    }

    Rectangle {
        id: progressLine
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        height: Theme.progressLineHeight
        z: 10
        visible: !readerView.toolbarVisible && readerView.isLoaded
        width: parent.width * Math.max(0, Math.min(100,
                    (typeof navController !== "undefined")
                    ? navController.progressPercent : 0)) / 100
        color: Theme.ink
    }

    Text {
        id: progressLinePageNum
        anchors.bottom:       parent.bottom
        anchors.right:        parent.right
        anchors.bottomMargin: Theme.spaceXS
        anchors.rightMargin:  Theme.spaceSM
        z: 10
        visible: progressLine.visible
        text: readerView.isLoaded
              ? (readerView.currentPage + "/" + readerView.pageCount)
              : ""
        font.pixelSize: Theme.textXS
        font.family:    Theme.uiFontFamily
        color:          Theme.muted
    }

    TocPanel {
        id: tocPanel

        onChapterSelected: function(index) {
            if (typeof navController !== "undefined")
                navController.goToTocEntry(index)
            tocPanel.close()
        }
    }

    BookmarkPanel {
        id: bookmarkPanel

        onBookmarkSelected: function(xpointer) {
            if (typeof navController !== "undefined")
                navController.goToBookmarkXPointer(xpointer)
            bookmarkPanel.close()
        }
    }

    AnnotationsPanel {
        id: annotationsPanel

        onNavigateToAnnotation: function(xpointer) {
            if (typeof navController !== "undefined")
                navController.goToBookmarkXPointer(xpointer)
            annotationsPanel.close()
        }
    }

    Item { id: orientationButton }

    SettingsSheet {
        id: settingsSheet
    }

    FootnotePopup {
        id: footnotePopup
    }

    LinkPreviewPopup {
        id: linkPreviewPopup
    }

    DictionaryPopup {
        id: dictionaryPopup
    }

    Connections {
        target: (typeof dictionaryService !== "undefined") ? dictionaryService : null
        function onLookupComplete(word, entries) {
            if (dictionaryPopup.isOpen && dictionaryPopup.lookupWord === word) {
                dictionaryPopup.entries = entries
            }
        }
    }

    MarginNoteCanvas {
        id: marginNoteCanvas

        onNoteSaved: {
            if (typeof marginNoteService !== "undefined")
                marginNoteService.loadNotesForPage()
        }

        onNoteDismissed: {
            if (typeof marginNoteService !== "undefined")
                marginNoteService.loadNotesForPage()

            if (typeof penInputController !== "undefined")
                penInputController.setMarginNoteMode(false)
        }
    }

    onMarginHChanged: updatePenGeometry()
    onMarginVChanged: updatePenGeometry()
    onWidthChanged:   updatePenGeometry()
    onHeightChanged:  updatePenGeometry()
    Component.onCompleted: updatePenGeometry()

    function updatePenGeometry() {
        if (typeof penInputController !== "undefined") {
            penInputController.setViewGeometry(
                readerView.marginH, readerView.marginV,
                readerView.width, readerView.height,
                readerView.coordScaleX, readerView.coordScaleY)
        }
    }
}
