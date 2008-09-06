Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.network" );
Importer.loadQtBinding( "qt.xml" );

function parseLyrics( lyrics )
{
    print( "parsing..." );

    lyrics = lyrics.replace( /<tr><td rowspan="2" style="line-height:21px"><font class=mr>/, "" );

    var doc = new QDomDocument( );
    var root = doc.createElement( "lyrics" );
    TrackInfo = Amarok.Engine.currentTrack();

    root.setAttribute( "page_url", page_url );
    root.setAttribute( "title", TrackInfo.title );
    root.setAttribute( "artist", TrackInfo.artist );

    try {
        // html -> plaintext:
        lyrics = lyrics.replace( /<[Bb][Rr][^>]*>/g, "\n" );
        lyrics = lyrics.replace( /<.*>/g, "" ); // erase everything after the lyric
        lyricsStr = lyrics.replace( /\n\n[\n]+/g, "\n" );

        xml = xml.replace( "{artist}", TrackInfo.artist );
        xml = xml.replace( "{title}", TrackInfo.title );
        xml = xml.replace( "{page}", page_url );
        xml = xml.replace( "{lyrics}", lyricsStr );

	print( xml );

        var text = doc.createTextNode( "lyricsText" );
        text.setData( lyricsStr );

    } catch (err) {
        print( "error!: " + err );
    }
    Amarok.Lyrics.showLyricsHtml( xml );
} 

function lyricsFetchResult( reply )
{
    try {
        lyrics = Amarok.Lyrics.codecForName( reply.readAll(), "GB2312" );
    } catch( err )
    {
        print( "got error: " + err );
    }    
    print( "result: " + lyrics );
    lyrics = lyrics.replace( /<[iI][mM][gG][^>]*>/g, "");
    lyrics = lyrics.replace( /<[aA][^>]*>[^<]*<\/[aA]>/g, "" );
    lyrics = lyrics.replace( /<[sS][cC][rR][iI][pP][tT][^>]*>[^<]*(<!--[^>]*>)*[^<]*<\/[sS][cC][rR][iI][pP][tT]>/g, "" );
    print( "searching..." );
    lyricsPos = lyrics.search( /<tr><td rowspan="2" style="line-height:21px"><font class=mr>*/ );

    if( lyricsPos > 0 )
    {
        print( "found lyrics at pos " + lyricsPos );
        parseLyrics( lyrics.slice( lyricsPos ) );
    }
}

function fetchLyrics( artist, title )
{
    xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><lyric artist=\"{artist}\" title=\"{title}\">{lyrics}</lyric>"
    connection = new QNetworkAccessManager();
    var host = "mp3.sogou.com";
    var path;

    path = "/gecisearch.so?query=" + encodeURI( title ) + "+" + encodeURI( artist ) + "&pf=&class=5&sid=&nohead=0&duppid=1&pid=01002401";
    page_url = "http://" + host + path;

    connection.finished.connect( lyricsFetchResult );

    connection.get( new QNetworkRequest( new QUrl( page_url ) ) );
}

Amarok.Lyrics.fetchLyrics.connect( fetchLyrics );