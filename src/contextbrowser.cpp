// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#include "contextbrowser.h"
#include "collectiondb.h"
#include "metabundle.h"
#include "sqlite/sqlite.h"

#include <kapplication.h> //kapp->config(), QApplication::setOverrideCursor()
#include <kconfig.h>      //config object
#include <klocale.h>
#include <kcursor.h>      //waitCursor()
#include <kdebug.h>
#include <klineedit.h>
#include <kurl.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kurlcombobox.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <khtml_part.h>
#include <dirent.h>
#include <sys/stat.h>
#include <kurl.h>
#include "app.h"


ContextBrowser::ContextBrowser( const char *name )
        : QVBox( 0, name )
{
    setSpacing( 4 );
    setMargin( 5 );

    QHBox *hb1 = new QHBox( this );
    hb1->setSpacing( 4 );
    
    sqlInit();
    
    browser = new KHTMLPart( hb1 );
    browser->begin();
    browser->write( "<html></html>" );
    browser->end();

    connect( browser->browserExtension(),
             SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ), this,
             SLOT( openURLRequest(const KURL &, const KParts::URLArgs & ) ) );   
    
    setFocusProxy( hb1 ); //so focus is given to a sensible widget when the tab is opened
}


ContextBrowser::~ContextBrowser()
{}


void ContextBrowser::openURLRequest(const KURL &url, const KParts::URLArgs & )
{
    if ( url.protocol() == "album" )
    {
        QStringList info = QStringList::split( "/", url.path() );
        QStringList values;
        QStringList names;

        m_db->execSql( QString( "SELECT DISTINCT url FROM tags WHERE artist = %1 AND album = %2 ORDER BY track DESC;" )
                       .arg( info[0] )
                       .arg( info[1] ), &values, &names );

        for ( uint i = 0; i < values.count(); i++ )
        {
            if ( values[i].isEmpty() ) continue;

            KURL tmp;
            tmp.setPath( values[i] );
            pApp->insertMedia( tmp );
        }
    }

    if ( url.protocol() == "file" )
        pApp->insertMedia( url );
}


void ContextBrowser::showContextForItem( const MetaBundle &bundle )
{
    browser->begin();
    
    QString styleSheet( "a { color:black; font-size:8px; text-decoration:none; }"
                        "div { color:black; font-size:8px; text-decoration:none; }"
                        "td { color:black; font-size:8px; text-decoration:none; }"

                        ".song { color:black; font-size:8px; text-decoration:none; }"
                        ".song:hover { color:black; text-decoration:underline; background-color:#cccccc; }"
                        ".album { color:black; font-weight: bold; font-size:8px; text-decoration:none; }"
                        ".title { font-size: 11px; font-weight: bold; }"
                        ".head { font-size: 10px; font-weight: bold; }"

                        ".rbalbum        { border: solid #ffffff 1px; }"
                        ".rbalbum:hover  { cursor: default; border: solid #000000 1px; }"

                        ".rbcontent        { border: solid #cccccc 1px; }"
                        ".rbcontent:hover  { border: solid #000000 1px; }" );

    browser->setUserStyleSheet( styleSheet );

    browser->write( QString( "<html><div class='title'>Info for %1</div>" )
                    .arg( bundle.artist() ) );

    QStringList values;
    QStringList names;

    browser->write( "<div class='head'><br>Other titles:</div>" );
    browser->write( "<div class='rbcontent'>" );
    browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'><tr><td></td><td width='%100'>" );

    m_db->execSql( QString( "SELECT tags.title, tags.url "
                            "FROM tags, artist, album "
                            "WHERE tags.album = album.id AND album.name LIKE '%1' AND "
                                  "tags.artist = artist.id AND artist.name LIKE '%2' "
                            "ORDER BY random();" )
                   .arg( m_db->escapeString( bundle.album() ) )
                   .arg( m_db->escapeString( bundle.artist() ) ), &values, &names );

    for ( uint i = 0; i < ( values.count() / 2 ) && i < 10; i++ )
    {
        if ( values[i].isEmpty() ) continue;
        
        browser->write( QString ( "<a class='song' href=\"file:%1\">%2</a><br>" )
                        .arg( values[i*2 + 1] )
                        .arg( values[i*2] ) );
    }

    values.clear();
    names.clear();

    browser->write( "</td><td></td></tr></table>" );
    browser->write( "</div>" );
    browser->write( "<div class='head'><br>Other albums:</div>" );
    browser->write( "<table width='100%' border='0' cellspacing='2' cellpadding='1'>" );
    
    m_db->execSql( QString( "SELECT DISTINCT album.name, album.id, artist.id "
                            "FROM album, tags, artist "
                            "WHERE album.id = tags.album AND tags.artist = artist.id AND artist.name "
                            "LIKE '%1' ORDER BY album.name;" )
                   .arg( m_db->escapeString( bundle.artist() ) ), &values, &names );

    for ( uint i = 0; i < ( values.count() / 3 ); i++ )
    {
        if ( values[i].isEmpty() ) continue;

        browser->write( QString ( "<tr><td onClick='window.location.href=\"album:%1/%2\"' height='42' valign='top' class='rbalbum'>"
                                  "<img align='left' hspace='2' width='40' height='40' src='%3'><span class='album'>%4</span><br>%5 Tracks</td>"
                                  "<td></td></tr>" )
                        .arg( values[i*3 + 2] )
                        .arg( values[i*3 + 1] )
                        .arg( m_db->getImageForAlbum( values[i*3 + 2], values[i*3 + 1], locate( "data", "amarok/images/sound.png" ) ) )
                        .arg( values[i*3] )
                        .arg( m_db->albumSongCount( values[i*3 + 2], values[i*3 + 1] ) ) );
    }

    browser->write( "</table><br>" );
    
/*    const KURL &url = bundle.url();
    QString tipBuf;
    QStringList validExtensions;
    validExtensions << "jpg" << "png" << "gif" << "jpeg";

    tipBuf = "<table width=108 align=center>";
    
    DIR *d = opendir( url.directory( FALSE, FALSE ).local8Bit() );
    if ( d )
    {
        const QString td = "<td width=108><img width=100 src='%1%2'></td>";
        dirent *ent;

        while ( ( ent = readdir( d ) ) )
        {
            QString file( ent->d_name );

            if ( validExtensions.contains( file.mid( file.findRev('.')+1 ) ) )
            {
                // we found an image, let's add it to the tooltip
                tipBuf += "<tr>"; //extra row for spacing
                tipBuf += td.arg( url.directory( FALSE, TRUE ), file );
                tipBuf += "</tr>";
            }
        }

        closedir( d );
    }

    tipBuf += "</table>";*/
    
    QString tipBuf;
    browser->write( QString( "%1</html>" ).arg( tipBuf ) );
    browser->end();
}


void ContextBrowser::sqlInit()
{
    QCString path = ( KGlobal::dirs() ->saveLocation( "data", kapp->instanceName() + "/" )
                  + "collection.db" ).local8Bit();
    m_db = new CollectionDB( path );
}


#include "contextbrowser.moc"
