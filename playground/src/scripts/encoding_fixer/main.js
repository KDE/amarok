/**************************************************************************
*   Encoding Fixer Script for Amarok 2.0                                  *
*   Last Modified:  21/11/2008                                            *
*                                                                         *
*   Copyright                                                             *
*   (C) 2008 Peter ZHOU  <peterzhoulei@gmail.com>                         *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
**************************************************************************/

Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.gui" );
Importer.loadQtBinding( "qt.uitools" );

function checkerURL( encoding, webencoding, url, querystr )
{
    this.encoding = encoding;
    this.webencoding = webencoding;
    this.url = url;
    this.querystr = querystr;
}

function fixencoding( encoding )
{
//Yes = 3, No = 4
    if ( Amarok.alert( "<p>the track might be encoded in " + encoding + ".</p>"
                       + "<p>Title: " + Amarok.Lyrics.QStringtoUtf8( Amarok.Engine.currentTrack().title, encoding ) + "</p>"
                       + "<p>Artist: " + Amarok.Lyrics.QStringtoUtf8( Amarok.Engine.currentTrack().artist, encoding ) + "</p>"
+                      + "<p>Album: " + Amarok.Lyrics.QStringtoUtf8( Amarok.Engine.currentTrack().album, encoding ) + "</p>"
                       + "<p>Comment: " + Amarok.Lyrics.QStringtoUtf8( Amarok.Engine.currentTrack().comment, encoding ) + "</p>"
                       + "<p>Do you want to convert it to UTF-8?</p>", "questionYesNo" ) == 3 )
    {
        Amarok.debug( "converting the track tag to " + encoding + "..." );
        Amarok.Engine.currentTrack().title = Amarok.Lyrics.QStringtoUtf8( Amarok.Engine.currentTrack().title, encoding );
        Amarok.Engine.currentTrack().artist = Amarok.Lyrics.QStringtoUtf8( Amarok.Engine.currentTrack().artist, encoding );
        Amarok.Engine.currentTrack().album = Amarok.Lyrics.QStringtoUtf8( Amarok.Engine.currentTrack().album, encoding );
        Amarok.Engine.currentTrack().comment = Amarok.Lyrics.QStringtoUtf8( Amarok.Engine.currentTrack().comment, encoding );
    }
}

function checkGB18030( gb18030reply )
{
    try
    {
        this.pageresult = gb18030reply;
//        Amarok.debug( reply );
        Amarok.debug( "searching..." );
        this.resultPos = this.pageresult.search( /<tr><td rowspan="2" style="line-height:21px"><font class=mr>*/ );

        if( this.resultPos > 0 )
        {
            Amarok.debug( "found info at pos " + this.resultPos );
            Amarok.debug( "the encoding of the track might be: GB18030" );
            fixencoding( "GB18030" );
        }
        else
        {
            Amarok.debug( "cannot find info using encoding: GB18030" );
        }
    }
    catch( err )
    {
        Amarok.debug ( err );
    }
}

function checkBig5( big5reply )
{
    try
    {
        this.pageresult = big5reply;
//        Amarok.debug( reply );
        Amarok.debug( "searching..." );
        this.resultPos = this.pageresult.search( /<tr><td rowspan="2" style="line-height:21px"><font class=mr>*/ );

        if( this.resultPos > 0 )
        {
            Amarok.debug( "found info at pos " + this.resultPos );
            Amarok.debug( "the encoding of the track might be: Big5" );
            fixencoding( "Big5" );
        }
        else
        {
            Amarok.debug( "cannot find info using encoding: Big5" );
        }
    }
    catch( err )
    {
        Amarok.debug ( err );
    }
}

function checkKOI8R( koi8rreply )
{
    try
    {
        this.pageresult = koi8rreply;
//        Amarok.debug( reply );
        Amarok.debug( "searching..." );
        this.resultPos = this.pageresult.search( /style="margin-right: 30px;">*/ );

        if( this.resultPos == 0 )
        {
            Amarok.debug( "found info at pos " + this.resultPos );
            Amarok.debug( "the encoding of the track might be: KOI8-R" );
            fixencoding( "KOI8-R" );
        }
        else
        {
            Amarok.debug( "cannot find info using encoding: KOI8-R" );
        }
    }
    catch( err )
    {
        Amarok.debug ( err );
    }
}

function urlGenerator( title, urlInfo )
{
    try
    {
        Amarok.debug( "checking for track title: " + title );

        Amarok.debug( "checking encoding candidate: " + urlInfo.encoding );
        this.info = Amarok.Lyrics.QStringtoUtf8( title, urlInfo.encoding );
        Amarok.debug( "trying: " + this.info );

        this.encodedTitleKey = Amarok.Lyrics.fromUtf8( urlInfo.querystr, urlInfo.webencoding );
        this.encodedTitle = Amarok.Lyrics.fromUtf8( this.info, urlInfo.webencoding );
        this.url = new QUrl( urlInfo.url );
        this.url.addEncodedQueryItem( this.encodedTitleKey, this.encodedTitle );
        return this.url;
    }
    catch( err )
    {
        Amarok.debug ( err );
    }

}

var urlInfo = new Array (
    new checkerURL( "GB18030", "GB2312", "http://mp3.sogou.com/gecisearch.so", "query" ),
    new checkerURL( "Big5", "GB2312", "http://mp3.sogou.com/gecisearch.so", "query" ),
    new checkerURL( "KOI8-R", "KOI8-R", "http://www.zvuki.ru/search/?what=song", "query" )
//    new checkerURL( "UTF-8", "", "", "" )
);

function doubleChecker()
{
    //checking gb18030
    this.title = Amarok.Engine.currentTrack().title;
    this.gb18030url = urlGenerator( this.title, this.urlInfo[0] );
    this.gb18030d = new Downloader( this.gb18030url, checkGB18030, urlInfo[0].webencoding );
    //checking big5
    this.big5url = urlGenerator( this.title, this.urlInfo[1] );
    this.big5d = new Downloader( this.big5url, checkBig5, urlInfo[1].webencoding );
    //checking KOI8-R
    this.koi8rurl = urlGenerator( this.title, this.urlInfo[2] );
    this.koi8rd = new Downloader( this.koi8rurl, checkKOI8R, urlInfo[2].webencoding );

//Amarok.Script.writeConfig( "test", "123" );
//Amarok.debug( "configuation test: " + Amarok.Script.readConfig( "test", "no return" ) );
}

function onConfiguration()
{
    try
    {
        var UIloader = new QUiLoader( this );
        var uifile = new QFile ( Amarok.Info.scriptPath() + "/main.ui" );
        uifile.open( QIODevice.ReadOnly );
        var myWidget = UIloader.load( uifile, this);
        uifile.close();
        myWidget.show();
    }
    catch( err )
    {
        Amarok.debug ( err );
    }
}

Amarok.Window.addToolsSeparator();
Amarok.Window.addToolsMenu( "checkencoding", "Check Encodings" );
Amarok.Window.addSettingsSeparator();
Amarok.Window.addSettingsMenu( "configencoding", "Encoding Fixer Settings..." );
Amarok.Window.ToolsMenu.checkencoding.triggered.connect( doubleChecker );
Amarok.Window.SettingsMenu.configencoding.triggered.connect( onConfiguration );