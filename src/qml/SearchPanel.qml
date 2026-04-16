import QtQuick 2.15
import ReadMarkable

Item {
    id: searchPanel
    anchors.fill: parent
    visible: isOpen
    z: 100

    property bool isOpen: false

    signal searchNavigated(string query)

    function show() {
        isOpen = true
        searchKeyboard.show()
        searchField.forceActiveFocus()
    }

    function dismiss() {
        searchKeyboard.dismiss()
        isOpen = false
        if (typeof searchService !== "undefined")
            searchService.clearResults()
        searchField.text = ""
    }

    function formatSnippet(text) {
        if (!text) return ""
        return text.replace(/\*\*([^*]+)\*\*/g, '<b>$1</b>')
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.paper
    }

    OnScreenKeyboard {
        id: searchKeyboard
        target: searchField

        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
    }

    Rectangle {
        id: searchRow
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: searchKeyboard.isOpen ? searchKeyboard.top : parent.bottom
        height: Theme.toolbarHeight
        color: Theme.paper

        Rectangle {
            anchors.left:  parent.left
            anchors.right: parent.right
            anchors.top:   parent.top
            height: Theme.borderWidth
            color:  Theme.separator
        }

        Row {
            anchors {
                left:           parent.left
                right:          parent.right
                verticalCenter: parent.verticalCenter
                leftMargin:     Theme.spaceMD
                rightMargin:    Theme.spaceSM
            }
            spacing: Theme.spaceSM

            Rectangle {
                id: fieldBg
                width:  parent.width - resultCountLabel.width - closeButton.width
                        - parent.spacing * 2
                height: Theme.toolbarHeight - Theme.spaceMD
                color:  Theme.paper
                border.color: Theme.separator
                border.width: Theme.borderWidth

                TextInput {
                    id: searchField
                    anchors {
                        left:           parent.left
                        right:          parent.right
                        verticalCenter: parent.verticalCenter
                        leftMargin:     Theme.spaceSM
                        rightMargin:    Theme.spaceSM
                    }
                    font.pixelSize: Theme.textMD
                    font.family:    Theme.uiFontFamily
                    color:          Theme.ink

                    Text {
                        anchors.fill: parent
                        text:         "Search..."
                        font.pixelSize: Theme.textMD
                        font.family:    Theme.uiFontFamily
                        color:          Theme.muted
                        visible:        searchField.text.length === 0
                        verticalAlignment: Text.AlignVCenter
                    }

                    onTextChanged: {
                        searchDebounce.restart()
                    }
                }
            }

            Text {
                id: resultCountLabel
                anchors.verticalCenter: parent.verticalCenter
                text: (typeof searchService !== "undefined" && searchService.resultCount > 0)
                      ? searchService.resultCount + ""
                      : ""
                font.pixelSize: Theme.textSM
                font.family:    Theme.uiFontFamily
                color:          Theme.faint
                visible:        (typeof searchService !== "undefined")
                                && searchService.resultCount > 0
            }

            Item {
                id: closeButton
                width:  closeText.implicitWidth + Theme.spaceLG
                height: Theme.toolbarHeight - Theme.spaceMD
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    anchors.fill: parent
                    color: closeButtonArea.containsPress ? Theme.ink : Theme.paper
                }

                Text {
                    id: closeText
                    anchors.centerIn: parent
                    text:           "X"
                    font.pixelSize: Theme.textMD
                    font.family:    Theme.uiFontFamily
                    font.bold:      true
                    color:          closeButtonArea.containsPress ? Theme.paper : Theme.ink
                }

                MouseArea {
                    id: closeButtonArea
                    anchors.fill: parent
                    onClicked: searchPanel.dismiss()
                }
            }
        }
    }

    ListView {
        id: resultsList
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.top:    parent.top
        anchors.bottom: searchRow.top

        model: (typeof searchService !== "undefined")
               ? searchService.results : []

        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Text {
            anchors.centerIn: parent
            visible: resultsList.count === 0 && searchField.text.length >= 2
            text: "No results"
            font.pixelSize: Theme.textMD
            font.family:    Theme.uiFontFamily
            color:          Theme.muted
        }

        Text {
            anchors.centerIn: parent
            visible: searchField.text.length < 2
            text: "Type to search"
            font.pixelSize: Theme.textMD
            font.family:    Theme.uiFontFamily
            color:          Theme.muted
        }

        delegate: Item {
            width: resultsList.width
            height: delegateContent.implicitHeight + Theme.spaceMD

            Rectangle {
                anchors.fill: parent
                color: delegateArea.containsPress ? Theme.subtle : Theme.paper
            }

            Column {
                id: delegateContent
                anchors {
                    left:    parent.left
                    right:   parent.right
                    top:     parent.top
                    margins: Theme.spaceMD
                }
                spacing: Theme.spaceXS

                Text {
                    anchors.left:  parent.left
                    anchors.right: parent.right
                    text:           searchPanel.formatSnippet(modelData.snippet)
                    textFormat:     Text.StyledText
                    font.pixelSize: Theme.textSM
                    font.family:    Theme.uiFontFamily
                    color:          Theme.ink
                    wrapMode:       Text.Wrap
                }

                Text {
                    anchors.left:  parent.left
                    anchors.right: parent.right
                    text: {
                        var parts = []
                        if (modelData.chapterTitle && modelData.chapterTitle.length > 0)
                            parts.push(modelData.chapterTitle)
                        parts.push("p. " + modelData.pageNum)
                        return parts.join(" -- ")
                    }
                    font.pixelSize: Theme.textXS
                    font.family:    Theme.uiFontFamily
                    color:          Theme.faint
                    elide:          Text.ElideRight
                }
            }

            Rectangle {
                anchors.left:   parent.left
                anchors.right:  parent.right
                anchors.bottom: parent.bottom
                height:         Theme.borderWidth
                color:          Theme.subtle
            }

            MouseArea {
                id: delegateArea
                anchors.fill: parent
                onClicked: {
                    var query = searchField.text.trim()
                    if (typeof renderer !== "undefined")
                        renderer.goToPage(modelData.pageNum)
                    searchPanel.searchNavigated(query)
                    searchPanel.dismiss()
                }
            }
        }
    }

    Timer {
        id: searchDebounce
        interval: 500
        repeat:   false
        onTriggered: {
            if (typeof searchService === "undefined") return
            var q = searchField.text.trim()
            if (q.length >= 2)
                searchService.search(q)
            else
                searchService.clearResults()
        }
    }
}
