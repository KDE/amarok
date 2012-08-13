
import QtQuick 1.1

Item {
    property alias track: title.text
    property alias album_artist: albumartist.text
    property alias albumart: albumart.color // FIXME

    Text {
        id: title
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        text: "The worst day since yesterday"
        font.pointSize: 12
    }
    Text {
        id: albumartist
        anchors.top: title.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 5
        text: "Floggin molly - Swagger"
    }
    Rectangle {
        id: albumart
        width: 100
        height: 100
        anchors.right: parent.right
        anchors.top: parent.top
        color: "gray"
        z: -50
    }
}
