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
    sqlInit();

    setSpacing( 4 );
    setMargin( 5 );

    QHBox *hb1 = new QHBox( this );
    hb1->setSpacing( 4 );

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
    kdDebug() << url.path().latin1() << endl;
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
    QStringList values;
    QStringList names;

    // take care of sql updates (schema changed errors)
    delete m_db;
    sqlInit();

    browser->begin();

    QString styleSheet;
    styleSheet =  QString( "div { color: %1; font-size: 8px; text-decoration: none; }" )
                  .arg( colorGroup().text().name() );
    styleSheet += QString( "td { color: %1; font-size: 8px; text-decoration: none; }" )
                  .arg( colorGroup().text().name() );
    styleSheet += QString( ".song { color: %1; font-size: 8px; text-decoration: none; }" )
                  .arg( colorGroup().text().name() );
    styleSheet += QString( ".song:hover { color: %1; cursor: default; font-weight: bold; background-color: %2; }" )
                  .arg( colorGroup().base().name() ).arg( colorGroup().highlight().name() );
    styleSheet += QString( ".album { font-weight: bold; font-size: 8px; text-decoration: none; }" );
    styleSheet += QString( ".title { color: %1; font-size: 11px; font-weight: bold; }" )
                  .arg( colorGroup().text().name() );
    styleSheet += QString( ".head { color: %1; font-size: 10px; font-weight: bold; background-color: %2; }" )
                  .arg( colorGroup().base().name() ).arg( colorGroup().highlight().name() );
    styleSheet += QString( ".rbcurrent { color: %1; border: solid %2 1px; }" )
                  .arg( colorGroup().text().name() ).arg( colorGroup().base().name() );
    styleSheet += QString( ".rbalbum { color: %1; border: solid %2 1px; }" )
                  .arg( colorGroup().text().name() ).arg( colorGroup().base().name() );
    styleSheet += QString( ".rbalbum:hover { color: %1; cursor: default; background-color: %2; border: solid %3 1px; }" )
                  .arg( colorGroup().base().name() ).arg( colorGroup().highlight().name() ).arg( colorGroup().text().name() );
    styleSheet += QString( ".rbcontent { border: solid %1 1px; }" )
                  .arg( colorGroup().highlight().name() );
    styleSheet += QString( ".rbcontent:hover { border: solid %1 1px; }" )
                  .arg( colorGroup().text().name() );

    browser->setUserStyleSheet( styleSheet );

    // <Current Track Information>
    browser->write( "<html><div class='rbcontent'>" );
    browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
    browser->write( "<tr><td class='head'>&nbsp;" + i18n( "Currently playing:" ) + "</td></tr>" );
    browser->write( "<tr><td height='1' bgcolor='black'></td></tr>" );
    browser->write( "</table>" );
    browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    m_db->execSql( QString( "SELECT album.id, artist.id, datetime( datetime(statistics.accessdate, 'unixepoch'), 'localtime' ), statistics.playcounter "
                            "FROM album, tags, artist, statistics "
                            "WHERE album.id = tags.album AND artist.id = tags.artist AND statistics.url = tags.url AND tags.url = '%1';" )
                   .arg( m_db->escapeString( bundle.url().path() ) ), &values, &names );

    if ( !values.count() )
    {
        values << "0";
        values << "0";
        values << i18n( "Never" );
        values << "0";
    }
    browser->write( QString ( "<tr><td height='42' valign='top' class='rbcurrent'>"
                              "<span class='album'>%1 - %2</span><br><br><img align='left' valign='center' hspace='2' width='40' height='40' src='%3'>"
                              "%4<br>Last play: %5<br>Total plays: %6</td>"
                              "</tr>" )
                    .arg( bundle.artist() )
                    .arg( bundle.title() )
                    .arg( m_db->getImageForAlbum( values[1], values[0], locate( "data", "amarok/images/sound.png" ) ) )
                    .arg( bundle.album() )
                    .arg( values[2] )
                    .arg( values[3] ) );

    values.clear();
    names.clear();

    browser->write( "</table></div>" );
    // </Current Track Information>

    // <Favourite Tracks Information>
    m_db->execSql( QString( "SELECT tags.title, tags.url, statistics.playcounter "
                            "FROM tags, artist, statistics "
                            "WHERE tags.artist = artist.id AND artist.name LIKE '%1' AND statistics.url = tags.url "
                            "ORDER BY statistics.playcounter DESC "
                            "LIMIT 0,5;" )
                   .arg( m_db->escapeString( bundle.artist() ) ), &values, &names );

    if ( values.count() )
    {
        browser->write( "<br><div class='rbcontent'>" );
        browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
        browser->write( "<tr><td class='head'>&nbsp;" + i18n( "Favorite tracks by this artist:" ) + "</td></tr>" );
        browser->write( "<tr><td height='1' bgcolor='black'></td></tr>" );
        browser->write( "</table>" );
        browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );
    
        for ( uint i = 0; i < ( values.count() / 3 ); i++ )
        {
            browser->write( QString ( "<tr><td class='song' onClick='window.location.href=\"file:%1\"'>%2 <i>(%3)</i></a></td></tr>" )
                            .arg( values[i*3 + 1] )
                            .arg( values[i*3] )
                            .arg( values[i*3 + 2] ) );
        }
    
        values.clear();
        names.clear();
    
        browser->write( "</table></div>" );
    }
    // </Favourite Tracks Information>

    // <Tracks on this album>
    m_db->execSql( QString( "SELECT tags.title, tags.url, tags.track "
                            "FROM tags, artist, album "
                            "WHERE tags.album = album.id AND album.name LIKE '%1' AND "
                                  "tags.artist = artist.id AND artist.name LIKE '%2' "
                            "ORDER BY tags.track;" )
                   .arg( m_db->escapeString( bundle.album() ) )
                   .arg( m_db->escapeString( bundle.artist() ) ), &values, &names );

    if ( values.count() )
    {
        browser->write( "<br><div class='rbcontent'>" );
        browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
        browser->write( "<tr><td class='head'>&nbsp;" + i18n( "Tracks on this album:" ) + "</td></tr>" );
        browser->write( "<tr><td height='1' bgcolor='black'></td></tr>" );
        browser->write( "</table>" );
        browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );
    
        for ( uint i = 0; i < ( values.count() / 3 ); i++ )
        {
            browser->write( QString ( "<tr><td class='song' onClick='window.location.href=\"file:%1\"'>%2%3</a></td></tr>" )
                            .arg( values[i*3 + 1] )
                            .arg( ( values[i*3 + 2] == "" ) ? "" : values[i*3 + 2] + ". " )
                            .arg( values[i*3] ) );
        }
    
        values.clear();
        names.clear();
    
        browser->write( "</table></div>" );
    }
    // </Tracks on this album>

    // <Albums by this artist>
    m_db->execSql( QString( "SELECT DISTINCT album.name, album.id, artist.id "
                            "FROM album, tags, artist "
                            "WHERE album.id = tags.album AND tags.artist = artist.id AND artist.name LIKE '%1' "
                            "ORDER BY album.name;" )
                   .arg( m_db->escapeString( bundle.artist() ) ), &values, &names );

    if ( values.count() )
    {
        browser->write( "<br><div class='rbcontent'>" );
        browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
        browser->write( "<tr><td class='head'>&nbsp;" + i18n( "Albums by this artist:" ) + "</td></tr>" );
        browser->write( "<tr><td height='1' bgcolor='black'></td></tr>" );
        browser->write( "</table>" );
        browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );
    
        for ( uint i = 0; i < ( values.count() / 3 ); i++ )
        {
            if ( values[i].isEmpty() ) continue;
    
            browser->write( QString ( "<tr><td onClick='window.location.href=\"album:%1/%2\"' height='42' valign='top' class='rbalbum'>"
                                      "<img align='left' hspace='2' width='40' height='40' src='%3'><span class='album'>%4</span><br>%5 Tracks</td>"
                                      "</tr>" )
                            .arg( values[i*3 + 2] )
                            .arg( values[i*3 + 1] )
                            .arg( m_db->getImageForAlbum( values[i*3 + 2], values[i*3 + 1], locate( "data", "amarok/images/sound.png" ) ) )
                            .arg( values[i*3] )
                            .arg( m_db->albumSongCount( values[i*3 + 2], values[i*3 + 1] ) ) );
        }

        browser->write( "</table></div>" );
    }
    // </Albums by this artist>
    
    browser->write( "<br></html>" );
    browser->end();

    m_db->incSongCounter( bundle.url().path() );
}


void ContextBrowser::sqlInit()
{
    m_db = new CollectionDB();
}


#include "contextbrowser.moc"
