
import QtQuick 1.1
import org.kde.qtextracomponents 0.1

Item {
    property alias track: title.text
    property alias album_artist: albumartist.text
    property alias albumart: albumart.pixmap
    property alias playing: notplayingtext.playing;

    Text {
        property bool playing;
        id: notplayingtext

        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        playing: false
        text: "No track playing"
        font.pointSize: 15

        onPlayingChanged: {
            console.log("CALLED!!!! : playing = "+ playing)
            if (playing) {
                text = ""
            } else {
                text = "No track playing"
            }
        }
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
