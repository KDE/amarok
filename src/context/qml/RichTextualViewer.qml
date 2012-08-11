
import QtQuick 1.1
import QtWebKit 1.1
import org.kde.plasma.components 0.1

Item {

    property alias url: webview.url
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
//         contentWidth: webview.width
        contentHeight: webview.height
        anchors.top: title.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        flickableDirection: Flickable.VerticalFlick

        clip: true

        WebView {
            id: webview
            backgroundColor: "transparent"
            url: ""
            anchors.top: parent.top
            anchors.left : parent.left
            width: parent.width - scroll.width - 5
            anchors.rightMargin: 50
        }
    }
    ScrollBar {
        id: scroll
        flickableItem: it
    }
}
