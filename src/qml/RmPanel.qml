import QtQuick 2.15
import ReadMarkable

Rectangle {
    id: root

    default property alias content: contentItem.data
    property alias        header:  headerLoader.sourceComponent
    property int          contentPadding: Theme.spaceMD

    color:        Theme.paper
    border.color: Theme.panelBorder
    border.width: Theme.borderWidth
    radius:       Theme.borderRadius

    Loader {
        id: headerLoader
        anchors.top:   parent.top
        anchors.left:  parent.left
        anchors.right: parent.right
        visible:       sourceComponent !== null

    }

    Item {
        id: contentItem
        anchors.top:    headerLoader.visible ? headerLoader.bottom : parent.top
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.margins: root.contentPadding
    }
}
