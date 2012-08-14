
import QtQuick 1.1
import org.kde.qtextracomponents 0.1

Item {
    property alias track: title.text
    property alias album_artist: albumartist.text
    property alias albumart: albumart.pixmap
    property bool playing;

    id: root
    playing:false

    Text {
        id: notplayingtext

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width*0.8
        anchors.horizontalCenter: parent.horizontalCenter

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap

        text: root.playing? "" : "Play a track to display additional information"
        font.pointSize: 15

        Behavior on opacity {
            SmoothedAnimation { velocity: 2 }
        }
    }

    Item {
        id: information
        anchors.fill: parent
        opacity: root.playing? 1:0

        Behavior on opacity {
            SmoothedAnimation { velocity: 2 }
        }

        Text {
            id: title
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: albumart.left
            wrapMode: Text.WordWrap
            font.pointSize: 15
            text: ""
        }
        Text {
            id: albumartist
            anchors.top: title.bottom
            anchors.left: parent.left
            anchors.right: albumart.left
            clip: true
            text: ""
            wrapMode: Text.WordWrap
            anchors.topMargin: 5
            font.pointSize: 12
        }
        QPixmapItem {
            id: albumart
            width: nativeWidth
            height: nativeHeight
            anchors.right: parent.right
            anchors.top: parent.top
            z: -50
        }
    }
}
