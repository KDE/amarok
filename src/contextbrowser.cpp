// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#include "app.h" //openUrlRequest() FIXME remove this if possible
#include "amarokconfig.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "contextbrowser.h"
#include "enginecontroller.h"
#include "metabundle.h"
#include "playlist.h"     //appendMedia()
#include "sqlite/sqlite.h"

#include <kapplication.h> //kapp->config(), QApplication::setOverrideCursor()
#include <kconfig.h>      //config object
#include <kcursor.h>      //waitCursor()
#include <kdebug.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <klineedit.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kurlcombobox.h>

#include <qlabel.h>
#include <qpushbutton.h>

#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>


ContextBrowser::ContextBrowser( const char *name )
        : QVBox( 0, name )
        , m_currentTrack( 0 )
{
    EngineController::instance()->attach( this );

    m_db = new CollectionDB();

    setSpacing( 4 );
    setMargin( 5 );
    QWidget::setFont( AmarokConfig::useCustomFonts() ? AmarokConfig::playlistWindowFont() : QApplication::font() );

    QHBox *hb1 = new QHBox( this );
    hb1->setSpacing( 4 );

    browser = new KHTMLPart( hb1 );
    browser->setDNDEnabled( true );
    setStyleSheet();

    connect( browser->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                          SLOT( openURLRequest( const KURL & ) ) );

    if ( m_db->isEmpty() || !m_db->isDbValid() )
        showIntroduction();
    else
        showHome();

    setFocusProxy( hb1 ); //so focus is given to a sensible widget when the tab is opened
}


ContextBrowser::~ContextBrowser()
{
    delete m_db;
    delete m_currentTrack;

    EngineController::instance()->detach( this );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::setFont( const QFont &newFont ) //virtual
{
    if( font() != newFont )
    {
        QWidget::setFont( newFont );
        setStyleSheet();
        browser->setUserStyleSheet( m_styleSheet );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::openURLRequest( const KURL &url )
{
    m_url = url;

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
            Playlist::instance()->appendMedia( tmp, false, true );
        }
    }

    if ( url.protocol() == "file" )
        Playlist::instance()->appendMedia( url, true, true );

    if ( m_url.protocol() == "show" )
    {
        if ( m_url.path() == "home" )
            showHome();
        else if ( m_url.path() == "context" )
            showCurrentTrack();
        else if ( m_url.path() == "collectionSetup" )
        {
            //TODO if we do move the configuration to the main configdialog change this,
            //     otherwise we need a better solution
            QObject *o = pApp->playlistWindow()->child( "CollectionBrowser" );
            if( o ) static_cast<CollectionBrowser*>(o)->setupDirs();
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    //prevents segfault when playing streams
    if ( !bundle.url().isLocalFile() ) return;

    delete m_currentTrack;
    m_currentTrack = new MetaBundle( bundle );

    if ( !m_db->isEmpty() )
        showCurrentTrack();

    // increase song counter
    m_db->incSongCounter( m_currentTrack->url().path() );
}


void ContextBrowser::engineStateChanged( EngineBase::EngineState state )
{
    if ( m_db->isEmpty() )
        return;

    switch( state )
    {
        case EngineBase::Empty:
            showHome();
        default:
            break;
    }
}


void ContextBrowser::paletteChange( const QPalette& pal )
{
    kdDebug() << k_funcinfo << endl;

    QVBox::paletteChange( pal );

    setStyleSheet();
    showHome();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::setStyleSheet()
{
    int pxSize = fontMetrics().height() - 4;

    m_styleSheet =  QString( "div { color: %1; font-size: %2px; text-decoration: none; }" )
                    .arg( colorGroup().text().name() ).arg( pxSize );
    m_styleSheet += QString( "td { color: %1; font-size: %2px; text-decoration: none; }" )
                    .arg( colorGroup().text().name() ).arg( pxSize );
    m_styleSheet += QString( ".menu { color: %1; font-weight: bold; }" )
                    .arg( colorGroup().text().name() );
    m_styleSheet += QString( ".song { color: %1; font-size: %2px; text-decoration: none; }" )
                    .arg( colorGroup().text().name() ).arg( pxSize );
    m_styleSheet += QString( ".song:hover { color: %1; cursor: default; background-color: %2; }" )
                    .arg( colorGroup().base().name() ).arg( colorGroup().highlight().name() );
    m_styleSheet += QString( "A.song { color: %1; font-size: %2px; text-decoration: none; display: block; }" )
                    .arg( colorGroup().text().name() ).arg( pxSize );
    m_styleSheet += QString( "A.song:hover { color: %1; font-size: %2px; text-decoration: none; display: block; }" )
                    .arg( colorGroup().base().name() ).arg( colorGroup().highlight().name() );
    m_styleSheet += QString( ".album { font-weight: bold; font-size: %1px; text-decoration: none; }" )
                    .arg( pxSize );
    m_styleSheet += QString( ".title { color: %1; font-size: %2px; font-weight: bold; }" )
                    .arg( colorGroup().text().name() ).arg( pxSize + 3 );
    m_styleSheet += QString( ".head { color: %1; font-size: %2px; font-weight: bold; background-color: %3; }" )
                    .arg( colorGroup().base().name() ).arg( pxSize + 2 ).arg( colorGroup().highlight().name() );
    m_styleSheet += QString( ".rbcurrent { color: %1; border: solid %2 1px; }" )
                    .arg( colorGroup().text().name() ).arg( colorGroup().base().name() );
    m_styleSheet += QString( ".rbalbum { color: %1; border: solid %2 1px; }" )
                    .arg( colorGroup().text().name() ).arg( colorGroup().base().name() );
    m_styleSheet += QString( ".rbalbum:hover { color: %1; cursor: default; background-color: %2; border: solid %3 1px; }" )
                    .arg( colorGroup().base().name() ).arg( colorGroup().highlight().name() ).arg( colorGroup().text().name() );
    m_styleSheet += QString( ".rbcontent { border: solid %1 1px; }" )
                    .arg( colorGroup().highlight().name() );
    m_styleSheet += QString( ".rbcontent:hover { border: solid %1 1px; }" )
                    .arg( colorGroup().text().name() );
}


void ContextBrowser::showIntroduction()
{
    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    // <Favorite Tracks Information>
    browser->write( "<html><div>");
    browser->write( i18n( "Hello amaroK user!" )
                    + "<br><br>" + i18n( "To use the extended features of amaroK, you need to build a collection." )
                    + "&nbsp;<a href='show:collectionSetup'>" + i18n( "Click here to create one." ) + "</a>" );
    browser->write( "</div></html>");

    browser->end();
}


void ContextBrowser::showHome()
{
    QStringList values;
    QStringList names;

    // take care of sql updates (schema changed errors)
    delete m_db;
    m_db = new CollectionDB();

    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    // <Favorite Tracks Information>
    browser->write( "<html><div class='menu'><a class='menu' href='show:home'>Home</a>&nbsp;&nbsp;<a class='menu' href='show:context'>Current Track</a></div>");
    browser->write( "<div class='rbcontent'>" );
    browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
    browser->write( "<tr><td class='head'>&nbsp;" + i18n( "Your Favorite Tracks:" ) + "</td></tr>" );
    browser->write( "<tr><td height='1' bgcolor='black'></td></tr>" );
    browser->write( "</table>" );
    browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    m_db->execSql( QString( "SELECT tags.title, tags.url, statistics.playcounter, artist.name, album.name "
                            "FROM tags, artist, album, statistics "
                            "WHERE artist.id = tags.artist AND album.id = tags.album AND statistics.url = tags.url "
                            "ORDER BY statistics.playcounter DESC "
                            "LIMIT 0,10;" ), &values, &names );

    if ( values.count() )
    {
        for ( uint i = 0; i < values.count(); i = i + 5 )
            browser->write( QString ( "<tr><td class='song'><a class='song' href=\"file:"
                                    + values[i+1].replace( "\"", QCString( "%22" ) ) + "\"><b>" + values[i]
                                    + "</b> <i>(" + values[i+2] + ")</i><br>" + values[i+3] + " - " + values[i+4] + "</a></td></tr>" ) );
    }

    values.clear();
    names.clear();

    browser->write( "</table></div>" );
    // </Favorite Tracks Information>

    // <Recent Tracks Information>
    browser->write( "<br><div class='rbcontent'>" );
    browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
    browser->write( "<tr><td class='head'>&nbsp;" + i18n( "Newest Tracks:" ) + "</td></tr>" );
    browser->write( "<tr><td height='1' bgcolor='black'></td></tr>" );
    browser->write( "</table>" );
    browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    m_db->execSql( QString( "SELECT tags.title, tags.url, artist.name, album.name "
                            "FROM tags, artist, album "
                            "WHERE artist.id = tags.artist AND album.id = tags.album "
                            "ORDER BY tags.createdate DESC "
                            "LIMIT 0,10;" ), &values, &names );

    if ( values.count() )
    {
        for ( uint i = 0; i < values.count(); i = i + 4 )
            browser->write( QString ( "<tr><td class='song'><a class='song' href=\"file:"
                                    + values[i+1].replace( "\"", QCString( "%22" ) ) + "\"'><b>" + values[i]
                                    + "</b><br>" + values[i+2] + " - " + values[i+3] + "</a></td></tr>" ) );
    }

    values.clear();
    names.clear();

    browser->write( "</table></div>" );
    // </Recent Tracks Information>

    browser->write( "<br></html>" );
    browser->end();
}


void ContextBrowser::showCurrentTrack()
{
    if ( !m_currentTrack )
        return;

    QStringList values;
    QStringList names;

    // take care of sql updates (schema changed errors)
    delete m_db;
    m_db = new CollectionDB();

    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    // <Current Track Information>
    browser->write( "<html><div class='menu'><a class='menu' href='show:home'>Home</a>&nbsp;&nbsp;<a class='menu' href='show:context'>Current Track</a></div>");
    browser->write( "<div class='rbcontent'>" );
    browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
    browser->write( "<tr><td class='head'>&nbsp;" + i18n( "Currently playing:" ) + "</td></tr>" );
    browser->write( "<tr><td height='1' bgcolor='black'></td></tr>" );
    browser->write( "</table>" );
    browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    m_db->execSql( QString( "SELECT album.id, artist.id, datetime( datetime( statistics.createdate, 'unixepoch' ), 'localtime' ), datetime( datetime( statistics.accessdate, 'unixepoch' ), 'localtime' ), statistics.playcounter "
                            "FROM album, tags, artist, statistics "
                            "WHERE album.id = tags.album AND artist.id = tags.artist AND statistics.url = tags.url AND tags.url = '%1';" )
                   .arg( m_db->escapeString( m_currentTrack->url().path() ) ), &values, &names );

    if ( values.count() )
        browser->write( QString ( "<tr><td height='42' valign='top' class='rbcurrent'>"
                                  "<span class='album'>%1 - %2</span><br>%3<br><br><img align='left' valign='center' hspace='2' width='40' height='40' src='%4'>"
                                  "<i>First play: %5<br>Last play: %6<br>Total plays: %7</i></td>"
                                  "</tr>" )
                        .arg( m_currentTrack->artist() )
                        .arg( m_currentTrack->title() )
                        .arg( m_currentTrack->album() )
                        .arg( m_db->getImageForPath( m_currentTrack->url().directory(), locate( "data", "amarok/images/sound.png" ) ) )
                        .arg( values[2].left( values[2].length() - 3 ) )
                        .arg( values[3].left( values[3].length() - 3 ) )
                        .arg( values[4] ) );
    else
    {
        m_db->execSql( QString( "SELECT album.id, artist.id "
                                "FROM album, tags, artist "
                                "WHERE album.id = tags.album AND artist.id = tags.artist AND tags.url = '%1';" )
                      .arg( m_db->escapeString( m_currentTrack->url().path() ) ), &values, &names );

        if ( values.count() )
            browser->write( QString ( "<tr><td height='42' valign='top' class='rbcurrent'>"
                                      "<span class='album'>%1 - %2</span><br>%3<br><br><img align='left' valign='center' hspace='2' width='40' height='40' src='%4'>"
                                      "<i>Never played before</i></td>"
                                      "</tr>" )
                            .arg( m_currentTrack->artist() )
                            .arg( m_currentTrack->title() )
                            .arg( m_currentTrack->album() )
                            .arg( m_db->getImageForPath( m_currentTrack->url().directory(), locate( "data", "amarok/images/sound.png" ) ) ) );
    }

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
                   .arg( m_db->escapeString( m_currentTrack->artist() ) ), &values, &names );

    if ( values.count() )
    {
        browser->write( "<br><div class='rbcontent'>" );
        browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
        browser->write( "<tr><td class='head'>&nbsp;" + i18n( "Favorite tracks by this artist:" ) + "</td></tr>" );
        browser->write( "<tr><td height='1' bgcolor='black'></td></tr>" );
        browser->write( "</table>" );
        browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

        for ( uint i = 0; i < ( values.count() / 3 ); i++ )
            browser->write( QString ( "<tr><td class='song'><a class='song' href=\"file:" + values[i*3 + 1].replace( "\"", QCString( "%22" ) ) + "\">" + values[i*3] + " <i>(" + values[i*3 + 2] + ")</i></a></td></tr>" ) );

        values.clear();
        names.clear();

        browser->write( "</table></div>" );
    }
    // </Favourite Tracks Information>

    // <Tracks on this album>
    if ( !m_currentTrack->album().isEmpty() && !m_currentTrack->artist().isEmpty() )
    {
        m_db->execSql( QString( "SELECT tags.title, tags.url, tags.track "
                                "FROM tags, artist, album "
                                "WHERE tags.album = album.id AND album.name LIKE '%1' AND "
                                      "tags.artist = artist.id AND artist.name LIKE '%2' "
                                "ORDER BY tags.track;" )
                      .arg( m_db->escapeString( m_currentTrack->album() ) )
                      .arg( m_db->escapeString( m_currentTrack->artist() ) ), &values, &names );

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
                QString tmp = values[i*3 + 2] == "" ? "" : values[i*3 + 2] + ". ";
                browser->write( QString ( "<tr><td class='song'><a class='song' href=\"file:" + values[i*3 + 1].replace( "\"", QCString( "%22" ) ) + "\">" + tmp + values[i*3] + "</a></td></tr>" ) );
            }

            values.clear();
            names.clear();

            browser->write( "</table></div>" );
        }
    }
    // </Tracks on this album>

    // <Albums by this artist>
    m_db->execSql( QString( "SELECT DISTINCT album.name, album.id, artist.id "
                            "FROM album, tags, artist "
                            "WHERE album.id = tags.album AND tags.artist = artist.id AND album.name <> '' AND artist.name LIKE '%1' "
                            "ORDER BY album.name;" )
                   .arg( m_db->escapeString( m_currentTrack->artist() ) ), &values, &names );

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
}


#include "contextbrowser.moc"
