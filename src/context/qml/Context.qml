
import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.plasma.core 0.1
import "globals.js" as Globals

// Context view
Item {

    id: root
    property bool playing;

    playing: false

    onPlayingChanged: {
        infoSection.playing = playing
    }

    function startupFunction() {
        console.log("START UP")
        playing = true
        playing = false
    }

    Component.onCompleted: startupFunction();

    Svg {
        id: mainSvg
        imagePath: "Amarok/theme"
        usingRenderingCache: false
    }

    InfoWidget {
        id: infoSection
        anchors.right: parent.right; anchors.left: parent.left;
        anchors.top: parent.top
        height: 100
        anchors.rightMargin: 10
        anchors.leftMargin: 5
        anchors.topMargin: 15
        DataSource {
            id: currentSource
            engine: "amarok-current"
            connectedSources: ["current"]
            onDataChanged: {
                d = currentSource.data['current']
                root.playing = d["displayReady"]

                if (root.playing) {
                    infoSection.track = d['track']
                    infoSection.album_artist = d["artist"] + " - " + d["album"]
                    infoSection.albumart = d["albumart"]
                }
                console.log(Globals.iconSize)
            }
        }
    }

    Item {
        id: moreInfo

        anchors.top: infoSection.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        opacity: root.playing? 1:0

        Behavior on opacity {
            SmoothedAnimation { velocity: 3.5 }
        }

        Item {
            id: buttons
            height: Globals.iconSize
            anchors.right: parent.right
            anchors.left: parent.left;
            anchors.top: parent.top

            clip: true;

            SvgItem {
                id: lyrics_button
                elementId: "text"
                svg: mainSvg
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 2
                height: Globals.iconSize
                width: Globals.iconSize
                MouseArea {
                    anchors.fill: parent
                }
                MouseArea {
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
                height: Globals.iconSize
                width: Globals.iconSize
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        texts.push(wikitext)
                    }
                }
            }

    //         SvgItem { FIXME: uncomment when ready
    //             id: more_button
    //             elementId: "more"
    //             svg: mainSvg
    //             height: Globals.iconSize

    //             width: Globals.iconSize
    //             anchors.verticalCenter: parent.verticalCenter
    //             anchors.right: parent.right
    //         }
        }

        PageStack {
            id: texts
            anchors.topMargin: 4
            anchors.bottomMargin: 10
            anchors.top: buttons.top // this was buttons.bottom in the original design
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            initialPage: lyrics_text

            TextualViewer {
                z: 10
                id: wikitext
                title: "Wikipedia"
                visible: false
                Item {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    width: Globals.iconSize * 3 + Globals.iconSize
                    height: Globals.iconSize
                    clip: true
                    z: 5
                    Text {
                        anchors.fill: parent
                        text: "Waga"
                    }
                }
                
            }
            DataSource {
                id: wikiSource
                engine: "amarok-wikipedia"
                connectedSources: ["wikipedia"]
                onDataChanged: {
                    d = wikiSource.data['wikipedia']
                    wikitext.title = "Wikipedia" // - " + d['title']
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
    //                     lyrics_text.title = d['artist'] + " - " + d['title']
                        lyrics_text.title = "Lyrics"
                        lyrics_text.text = d['lyrics']
                    } else {
                        lyrics_text.title = "No lyrics available"
                        lyrics_text.text = ""
                    }
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
