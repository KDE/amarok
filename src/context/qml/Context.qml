
import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.plasma.core 0.1

// Context view
Item {
    
    // Sources and data

//     PlasmaCore.DataModel {
//         id: lyricsModel
//         dataSource: lyricsSource
//         keyRoleFilter: "[\\d]*"
//     }
    
    Flickable {
        id: info
        anchors.fill: parent
//         anchors.top: search.bottom
//         anchors.bottom: parent.bottom
//         anchors.right: parent.right
//         anchors.left: parent.left
//         anchors.margins: 3

        flickableDirection: Flickable.HorizontalFlick
        clip: true

        Item {
            id: buttons
            height: 30
            anchors.right: parent.right; anchors.left: parent.left;
            anchors.top: parent.top
            anchors.topMargin: 15

            clip: true;
            Image {
                id: lyrics_button
                source: "qrc:images/lyrics.png"
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 2
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

            Image {
                id: wikipedia_button
                source: "qrc:images/wikipedia.png"
//                 anchors.verticalCenter: parent.verticalCenter
                anchors.left: lyrics_button.right
                anchors.margins: 2
                anchors.leftMargin: 14
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        texts.push(wikitext)
                    }
                }
            }

            Image {
                id: more_button
                source: "qrc:images/more.png"
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
            RichTextualViewer {
                id: wikitext
                title: "Wikipedia - Artist"
                visible: false
                html: ""
            }
            DataSource {
                id: wikiSource
                engine: "amarok-wikipedia"
                connectedSources: ["wikipedia"]
                onDataChanged: {
                    d = wikiSource.data['wikipedia']
                    anObject = d
                    for (var prop in anObject) {
//                         console.log("Object item:", prop, "=", anObject[prop])
                    }
//                     if (d['displayReady'] == true) {
                        wikitext.title = "Wikipedia - " + d['title']
//                         wikitext.html = d['page']
                        wikitext.url = d['url']
//                     } else {
//                         lyrics_text.title = "Lyrics"
//                         lyrics_text.text = ""
//                     }
                }
            }
            TextualViewer {
                id: lyrics_text
                title: "Lyrics"
                visible: false
                text: ""
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
        }


    }
    Image {
            z: -100
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            source: "qrc:images/logo.png"
        }

}
