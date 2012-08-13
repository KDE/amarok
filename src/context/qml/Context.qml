
import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.plasma.core 0.1

// Context view
Item {
    Svg {
        id: mainSvg
//         imagePath: "widgets/arrows"
        imagePath: "Amarok/theme"
//         multipleImages: true
        usingRenderingCache: false
    }
    
    Item {
        id: buttons
        height: 30
        anchors.right: parent.right; anchors.left: parent.left;
        anchors.top: parent.top
        anchors.topMargin: 15

        clip: true;

        SvgItem {
            id: lyrics_button
            elementId: "text"
            svg: mainSvg
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 2
            height: 30
            width: 30
            MouseArea {
                anchors.fill: parent
            }
            MouseArea {
//                     color: "red"
                anchors.fill: parent
                onClicked: {
                    texts.push(lyrics_text)
                }
            }
        }

        SvgItem {
            id: wikipedia_button
            elementId: "wikipedia"
            svg: mainSvg
            anchors.left: lyrics_button.right
            anchors.margins: 2
            anchors.leftMargin: 14
            height: 30
            width: 30
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    texts.push(wikitext)
                }
            }
        }

        SvgItem {
            id: more_button
            elementId: "more"
            svg: mainSvg
            height: 30
            width: 30
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
        }
    }

    PageStack {
        id: texts
        anchors.topMargin: 15
        anchors.top: buttons.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.leftMargin: 15
        initialPage: lyrics_text
        TextualViewer {
            z: 10
            id: wikitext
            title: "Wikipedia - Artist"
            visible: false
        }
        DataSource {
            id: wikiSource
            engine: "amarok-wikipedia"
            connectedSources: ["wikipedia"]
            onDataChanged: {
                d = wikiSource.data['wikipedia']
                wikitext.title = "Wikipedia - " + d['title']
                wikitext.text = d['page']
            }
        }

        TextualViewer {
            z: 10
            id: lyrics_text
            title: "Lyrics"
            visible: false
        }
        DataSource {
            id: lyricsSource
            engine: "amarok-lyrics"
            connectedSources: ["lyrics"]
            onDataChanged: {
                d = lyricsSource.data['lyrics']
                if (d['displayReady'] == true) {
                    lyrics_text.title = d['artist'] + " - " + d['title']
                    lyrics_text.text = d['lyrics']
                } else {
                    lyrics_text.title = "No lyrics available"
                    lyrics_text.text = ""
                }
            }
        }

        DataSource {
            id: currentSource
            engine: "amarok-current"
            connectedSources: ["current"]
            onDataChanged: {
                console.log("current has got DATA")
                d = currentSource.data['current']
                for (var stuff in d) {
                    console.log(stuff + " uguale a " + d[stuff])
                }
            }
        }
    }

    SvgItem {
            z: -100
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            elementId: "logo"
            svg: mainSvg
            width: 244
            height: 244
        }

}
