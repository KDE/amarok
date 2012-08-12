
import QtQuick 1.1
import org.kde.plasma.components 0.1

Item {

    property alias text: textview.text
    property alias title: title.text

    Text {
        id: title
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 30
        text: "Title"
        font.pointSize: 15
        horizontalAlignment: Text.AlignHCenter
        anchors.bottomMargin: 5
    }
    
    Flickable {
        id: it
        contentHeight: textview.height
        anchors.top: title.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        flickableDirection: Flickable.VerticalFlick

        clip: true

        Text {
            id: textview
            anchors.top: parent.top
            anchors.left : parent.left
            width: parent.width - scroll.width - 5
            wrapMode: Text.Wrap
            anchors.rightMargin: 50
            text: ""
        }
    }
    ScrollBar {
        id: scroll
        flickableItem: it
    }
}