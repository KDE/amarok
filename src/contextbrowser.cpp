// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information


#include "config.h"

#include "amarokconfig.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "contextbrowser.h"
#include "coverfetcher.h"
#include "enginecontroller.h"
#include "k3bexporter.h"
#include "metabundle.h"
#include "playlist.h"     //appendMedia()
#include "qstringx.h"
#include "sqlite/sqlite3.h"

#include <kapplication.h> //kapp->config(), QApplication::setOverrideCursor()
#include <kconfig.h>      //config object
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kurlcombobox.h>

#include <qfile.h>
#include <qimage.h>
#include <qlabel.h>
#include <qpushbutton.h>

using amaroK::QStringx;


ContextBrowser::ContextBrowser( const char *name )
        : QVBox( 0, name )
        , m_currentTrack( 0 )
        , m_db( new CollectionDB() )
{
    kdDebug() << k_funcinfo << endl;
    EngineController::instance()->attach( this );

    setSpacing( 4 );
    setMargin( 5 );
    QWidget::setFont( AmarokConfig::useCustomFonts() ? AmarokConfig::playlistWindowFont() : QApplication::font() );

    QHBox *hb1 = new QHBox( this );
    hb1->setSpacing( 4 );

    browser = new KHTMLPart( hb1 );
    browser->setDNDEnabled( true );
    browser->view()->setMarginWidth( 4 );
    browser->view()->setMarginHeight( 4 );
    setStyleSheet();

    connect( browser->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                          SLOT( openURLRequest( const KURL & ) ) );
    connect( browser,                     SIGNAL( popupMenu( const QString&, const QPoint& ) ),
             this,                          SLOT( slotContextMenu( const QString&, const QPoint& ) ) );

    if ( m_db->isEmpty() || !m_db->isDbValid() ) {
        showIntroduction();
        m_emptyDB = true;
    } else {
        showHome();
        m_emptyDB = false;
    }

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
    QWidget::setFont( newFont );
    setStyleSheet();
    browser->setUserStyleSheet( m_styleSheet );
    browser->setStandardFont( newFont.key() );
    showCurrentTrack();
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::openURLRequest( const KURL &url )
{
    m_url = url;
    QStringList info = QStringList::split( " @@@ ", url.path() );

    if ( url.protocol() == "album" )
    {
        QStringList values;

        m_db->execSql( QString( "SELECT DISTINCT url FROM tags WHERE artist = %1 AND album = %2 ORDER BY track;" )
                       .arg( info[0] )
                       .arg( info[1] ), &values );

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
        else if ( m_url.path() == "stream" )
            showCurrentStream();
        else if ( m_url.path() == "collectionSetup" )
        {
            //TODO if we do move the configuration to the main configdialog change this,
            //     otherwise we need a better solution
            QObject *o = parent()->child( "CollectionBrowser" );
            if ( o ) static_cast<CollectionBrowser*>( o )->setupDirs();
        }
    }

    // When left-clicking on cover image, open browser with amazon site
    if ( m_url.protocol() == "fetchcover" )
    {
        QStringList info = QStringList::split( " @@@ ", m_url.path() );
        QImage img( m_db->getImageForAlbum( info[0], info[1], 0 ) );
        const QString amazonUrl = img.text( "amazon-url" );
        kdDebug() << "[ContextBrowser] Embedded amazon url in cover image: " << amazonUrl << endl;

        if ( amazonUrl.isEmpty() )
            KMessageBox::information( this, i18n( "Right-click on image for download menu." ) );
        else
            kapp->invokeBrowser( amazonUrl );
    }

    /* open konqueror with musicbrainz search result for artist-album */
    if ( url.protocol() == "musicbrainz" )
    {
        const QString command = "kfmclient openURL 'http://www.musicbrainz.org/taglookup.html?artist=''%1''&album=''%2'''";
        KRun::runCommand( command.arg( info[0] ).arg( info[1] ), "kfmclient", "konqueror" );
    }

}


void ContextBrowser::collectionScanStarted()
{
    if( m_emptyDB )
        showScanning();
}


void ContextBrowser::collectionScanDone()
{
    if( CollectionDB().isEmpty() ) {
        showIntroduction();
        m_emptyDB = true;
    }
    else if( m_emptyDB ) {
        showHome();
        m_emptyDB = false;
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::engineTrackEnded( int finalPosition, int trackLength )
{
    //This is where percentages are calculated
    //TODO statistics are not calculated when currentTrack doesn't exist

    if ( m_currentTrack )
    {
        // sanity check
        if ( finalPosition > trackLength || finalPosition == 0 )
            finalPosition = trackLength;

        int pct = (int) ( ( (double) finalPosition / (double) trackLength ) * 100 );

        // increase song counter & calculate new statistics
        float score = m_db->addSongPercentage( m_currentTrack->url().path(), pct );

        // TODO reimplement playlist-update
/*        if ( score )
            m_currentTrack->setText( PlaylistItem::Score, QString::number( score ) );*/
    }
}


void ContextBrowser::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    delete m_currentTrack;
    m_currentTrack = new MetaBundle( bundle );

    if ( m_db->isEmpty() || !m_db->isDbValid() )
        showIntroduction();
    else
        if ( EngineController::engine()->isStream() )
            showCurrentStream();
        else
            showCurrentTrack();
}


void ContextBrowser::engineStateChanged( Engine::State state )
{
    switch( state )
    {
        case Engine::Playing:
            if ( EngineController::engine()->isStream() )
                showCurrentStream();
            break;
        case Engine::Empty:
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
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::slotContextMenu( const QString& urlString, const QPoint& point )
{
    KURL url( urlString );

    if ( url.protocol() == "fetchcover" )
    {
        QStringList info = QStringList::split( " @@@ ", url.path() );
        enum menuIds { SHOW, FETCH, DELETE };

        KPopupMenu menu( this );
        menu.insertTitle( i18n( "Cover Image" ) );
        menu.insertItem( SmallIcon( "viewmag" ), i18n( "Show Fullsize" ), SHOW );
        menu.setItemEnabled( SHOW, !m_db->getImageForAlbum( info[0], info[1], 0 ).contains( "nocover" ) );
        menu.insertItem( SmallIcon( "www" ), i18n( "Fetch From amazon.com" ), FETCH );
    #ifndef AMAZON_SUPPORT
        menu.setItemEnabled( FETCH, false );
    #endif
        menu.insertSeparator();
        menu.insertItem( SmallIcon( "editdelete" ), i18n("Delete Image File"), DELETE );
        int id = menu.exec( point );

        switch ( id )
        {
            case FETCH:
            #ifdef AMAZON_SUPPORT
                /* fetch covers from amazon on click */
                m_db->fetchCover( this, info[0], info[1], false );
            #else
                if( m_db->getImageForAlbum( info[0], info[1], 0 ) != locate( "data", "amarok/images/nocover.png" ) )
                    viewImage( info[0], info[1] );
                else
                {
                    /* if no cover exists, open a file dialog to add a cover */
                    KURL file = KFileDialog::getImageOpenURL( ":homedir", this, i18n( "Select cover image file - amaroK" ) );
                    if ( !file.isEmpty() )
                    {
                        QImage img( file.directory() + "/" + file.fileName() );
                        QString filename( QFile::encodeName( info[0] + " - " + info[1] ) );
                        filename.replace( " ", "_" ).append( ".png" );
                        img.save( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() )+"/albumcovers/"+filename.lower(), "PNG" );
                        showCurrentTrack();
                    }
                }
            #endif
                break;

            case SHOW:
                /* open an image view widget */
                viewImage( info[0], info[1] );
                break;

            case DELETE:
                int button = KMessageBox::warningContinueCancel( this,
                                          i18n( "Are you sure you want to delete this cover?" ),
                                          QString::null,
                                          i18n("&Delete Confirmation") );

                if ( button == KMessageBox::Continue ) {
                    m_db->removeImageFromAlbum( info[0], info[1] );
                    showCurrentTrack();
                }
        }
    }

    if ( url.protocol() == "file" )
    {
        enum menuIds { APPEND, ASNEXT, MAKE, BURN_DATACD, BURN_AUDIOCD };

        KPopupMenu menu( this );
        menu.insertTitle( i18n( "Track" ) );
        menu.insertItem( SmallIcon( "player_playlist_2" ), i18n( "&Append To Playlist" ), APPEND );
        //menu.setItemEnabled( APPEND );
        menu.insertItem( SmallIcon( "next" ), i18n( "&Queue After Current Track" ), ASNEXT );
        menu.insertItem( SmallIcon( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );
        if( K3bExporter::isAvailable() ) {
            menu.insertSeparator();
            menu.insertItem( SmallIcon( "cdaudio_unmount" ), i18n( "&Burn Audio-CD" ), BURN_AUDIOCD );
            menu.insertItem( SmallIcon( "cdrom_unmount" ), i18n( "Burn &Data-CD" ), BURN_DATACD );
        }

        switch ( menu.exec( point ) )
        {
            case APPEND:
                Playlist::instance()->appendMedia( url, false, true );
                break;

            case ASNEXT:
                Playlist::instance()->queueMedia( url );
                break;

            case MAKE:
                Playlist::instance()->clear();
                Playlist::instance()->appendMedia( url, true, true );
                break;

            case BURN_DATACD:
                K3bExporter::instance()->exportTracks( url, K3bExporter::DataCD );
                break;

            case BURN_AUDIOCD:
                K3bExporter::instance()->exportTracks( url, K3bExporter::AudioCD );
                break;
        }
    }
    if ( url.protocol() == "album" )
    {
        enum menuIds { APPEND, ASNEXT, MAKE, BURN_DATACD, BURN_AUDIOCD };

        KPopupMenu menu( this );
        menu.insertTitle( i18n( "Album" ) );
        menu.insertItem( SmallIcon( "player_playlist_2" ), i18n( "&Append To Playlist" ), APPEND );
        menu.insertItem( SmallIcon( "next" ), i18n( "&Queue After Current Track" ), ASNEXT );
        menu.insertItem( SmallIcon( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );
        if( K3bExporter::isAvailable() ) {
            menu.insertSeparator();
            menu.insertItem( SmallIcon( "cdaudio_unmount" ), i18n( "&Burn Audio-CD" ), BURN_AUDIOCD );
            menu.insertItem( SmallIcon( "cdrom_unmount" ), i18n( "Burn &Data-CD" ), BURN_DATACD );
        }
         int id = menu.exec( point );

        QStringList list = QStringList::split( " @@@ ", url.path() );
        QStringList values;
        QStringList names;

        m_db->execSql( QString( "select distinct url from tags where artist = '%1' and album = '%2' order by track;" )
                       .arg( list[0] )
                       .arg( list[1] ), &values, &names );

        switch ( id )
        {
            case APPEND:
                for ( uint i = 0; i < values.count(); i++ )
                {
                    if ( values[i].isEmpty() ) continue;

                    KURL tmp;
                    tmp.setPath( values[i] );
                    if( EngineController::canDecode( tmp ) ) Playlist::instance()->appendMedia( tmp, false, true );
                }
            break;

            case ASNEXT:
                for ( uint i = 0; i < values.count(); i++ )
                {
                    if ( values[i].isEmpty() ) continue;

                    KURL tmp;
                    tmp.setPath( values[i] );
                    if( EngineController::canDecode( tmp ) ) Playlist::instance()->queueMedia( tmp );
                }
            break;

            case MAKE:
                Playlist::instance()->clear();

                for ( uint i = 0; i < values.count(); i++ )
                {
                    if ( values[i].isEmpty() ) continue;

                    KURL tmp;
                    tmp.setPath( values[i] );
                    if( EngineController::canDecode( tmp ) ) Playlist::instance()->appendMedia( tmp, false, true );
                }
            break;

            case BURN_DATACD:
                K3bExporter::instance()->exportTracks( url, K3bExporter::DataCD );
                break;

            case BURN_AUDIOCD:
                K3bExporter::instance()->exportTracks( url, K3bExporter::AudioCD );
                break;
        }
     }
}

void ContextBrowser::showHome() //SLOT
{
    QStringList values;
    QStringList names;

    // take care of sql updates (schema changed errors)
    delete m_db;
    m_db = new CollectionDB();
    // Triggers redisplay when new cover image is downloaded
    #ifdef AMAZON_SUPPORT
    connect( m_db, SIGNAL( coverFetched(const QString&) ), this, SLOT( showCurrentTrack() ) );
    #endif
    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html>" );

    QString menu = "<div class='menu'>"
                    "<a>%1</a>"
                    "&nbsp;&nbsp;"
                    "<a href='show:%2'>%3</a>"
                   "</div>";

    menu = menu.arg( i18n("Home") );

    if ( EngineController::engine()->isStream() )
        menu = menu.arg( "stream" ).arg( i18n("Current Stream") );
    else
        menu = menu.arg( "context" ).arg( i18n("Current Track") );

    browser->write( menu );

    browser->write( "<div class='rbcontent'>" );
    browser->write(  "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
    browser->write(   "<tr><th>" + i18n( "Your Favorite Tracks" ) + "</th></tr>" );
    browser->write(  "</table>" );
    browser->write(  "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    m_db->execSql( QString( "SELECT tags.title, tags.url, round( statistics.percentage + 0.5 ), artist.name, album.name "
                            "FROM tags, artist, album, statistics "
                            "WHERE artist.id = tags.artist AND album.id = tags.album AND statistics.url = tags.url "
                            "ORDER BY statistics.percentage DESC "
                            "LIMIT 0,10;" ), &values, &names );

    if ( values.count() )
    {
        for ( uint i = 0; i < values.count(); i = i + 5 )
            browser->write( QString ( "<tr><td class='song'><a href=\"file:"
                                    + values[i+1].replace( "\"", QCString( "%22" ) ) + "\"><b>" + values[i]
                                    + "</b> <i>(" + i18n( "Score:" ) + " " + values[i+2] + ")</i><br>" + values[i+3] + " - " + values[i+4] + "</a></td></tr>" ) );
    }

    values.clear();
    names.clear();

    browser->write( "</table></div><br>" );
    // </Favorite Tracks Information>

    // <Recent Tracks Information>
    browser->write( "<div class='rbcontent'>"
                     "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                      "<tr><th>" + i18n( "Your Newest Tracks" ) + "</th></tr>"
                     "</table>"
                     "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    m_db->execSql( QString( "SELECT tags.title, tags.url, artist.name, album.name "
                            "FROM tags, artist, album "
                            "WHERE artist.id = tags.artist AND album.id = tags.album "
                            "ORDER BY tags.createdate DESC "
                            "LIMIT 0,10;" ), &values, &names );

    if ( values.count() )
    {
        for ( uint i = 0; i < values.count(); i = i + 4 )
            browser->write( QString ( "<tr><td class='song'><a href=\"file:"
                                    + values[i+1].replace( "\"", QCString( "%22" ) ) + "\"'><b>" + values[i]
                                    + "</b><br>" + values[i+2] + " - " + values[i+3] + "</a></td></tr>" ) );
    }

    values.clear();
    names.clear();

    browser->write( "</table></div><br>" );
    // </Recent Tracks Information>

    browser->write( "</html>" );
    browser->end();
}


void ContextBrowser::showCurrentTrack() //SLOT
{
    #define escapeHTML(s)     QString(s).replace( "<", "&lt;" ).replace( ">", "&gt;" )
    #define escapeHTMLAttr(s) QString(s).replace( "%", "%25" ).replace( "'", "%27" )

    if ( !m_currentTrack )
        return;

    QStringList values;
    QStringList names;

    // take care of sql updates (schema changed errors)
    delete m_db;
    m_db = new CollectionDB();

    // Triggers redisplay when new cover image is downloaded
    connect( m_db, SIGNAL( coverFetched() ), this, SLOT( showCurrentTrack() ) );

    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html>" );

    QString menu = "<div class='menu'>"
                    "<a href='show:home'>%1</a>"
                    "&nbsp;&nbsp;"
                    "<a>%2</a>"
                   "</div>";

    browser->write( menu.arg( i18n("Home") ).arg( EngineController::engine()->isStream() ? i18n("Current Stream") : i18n("Current Track") ) );

    if ( !m_db->isFileInCollection( m_currentTrack->url().path() ) )
    {
        browser->write( "<div style='padding: 1em 0.5em 2em 0.5em'>");
        browser->write(   i18n("If you would like to see contextual information about this track, "
                               "you must add it to your Collection.") );
        browser->write(  "&nbsp;"
                         "<a href='show:collectionSetup'>" );
        browser->write(    i18n( "Click here to change your Collection setup" ) );
        browser->write(  "</a>."
                        "</div>" );
    }

    // <Current Track Information>
    browser->write( "<div class='rbcontent'>"
                     "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                      "<tr><th>" + i18n( "Currently Playing" ) + "</th></tr>"
                     "</table>"
                     "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    m_db->execSql( QString( "SELECT album.name, artist.name, datetime( datetime( statistics.createdate, 'unixepoch' ), 'localtime' ), "
                            "datetime( datetime( statistics.accessdate, 'unixepoch' ), 'localtime' ), statistics.playcounter, round( statistics.percentage + 0.5 ) "
                            "FROM tags, album, artist, statistics "
                            "WHERE album.id = tags.album AND artist.id = tags.artist AND statistics.url = tags.url AND tags.url = '%1';" )
                   .arg( m_db->escapeString( m_currentTrack->url().path() ) ), &values, &names );

    if ( !values.isEmpty() )
         /* making 2 tables is most probably not the cleanest way to do it, but it works. */
         browser->write( QStringx ( "<tr>"
                                     "<td height='42' valign='top' class='rbcurrent' width='90%'>"
                                      "<span class='album'><b>%1 - %2</b></span>"
                                      "<br>%3"
                                     "</td>"
                                     "<td valign='top' align='right' width='10%'>"
                                      "<a title='%4' href='musicbrainz:%5 @@@ %6'><img src='%7'></a>"
                                     "</td>"
                                    "</tr>"
                                   "</table>"
                                   "<table width='100%'>"
                                    "<tr>"
                                     "<td width='20%'>"
                                      "<a class='menu' href='fetchcover:%8 @@@ %9'>"
                                       "<img align='left' valign='center' hspace='2' title='Click for information from amazon.com, right-click for menu.' src='%10'>"
                                      "</a>"
                                     "</td>"
                                     "<td valign='bottom' align='right' width='80%'>"
                                      "%11<br>"
                                      "%12<br>"
                                      "%13<br>"
                                      "%14<br>"
                                     "</td>"
                                    "</tr>" )
                         .args( QStringList()
                                << escapeHTML( m_currentTrack->artist() )
                                << escapeHTML( m_currentTrack->title() )
                                << escapeHTML( m_currentTrack->album() )
                                << i18n( "Look up this track at musicbrainz.com" )
                                << escapeHTMLAttr( m_db->escapeString( m_currentTrack->artist() ) )
                                << escapeHTMLAttr( m_db->escapeString( m_currentTrack->album() ) )
                                << escapeHTML( locate( "data", "amarok/images/musicbrainz.png" ) )
                                << escapeHTMLAttr( m_currentTrack->artist() )
                                << escapeHTMLAttr( m_currentTrack->album() )
                                << escapeHTMLAttr( m_db->getImageForAlbum( values[1], values[0] ) )
                                << i18n( "Track played once", "Track played %n times", values[4].toInt() )
                                << i18n( "Score: %1" ).arg( values[5] )
                                << i18n( "Last play: %1" ).arg( values[3].left( values[3].length() - 3 ) )
                                << i18n( "First play: %1" ).arg( values[2].left( values[2].length() - 3 ) )
                                )
                         );
    else
    {
        m_db->execSql( QString( "SELECT album.name, artist.name "
                                "FROM tags, album, artist "
                                "WHERE album.id = tags.album AND artist.id = tags.artist AND tags.url = '%1';" )
                      .arg( m_db->escapeString( m_currentTrack->url().path() ) ), &values, &names );

             browser->write( QStringx ( "<tr><td height='42' valign='top' class='rbcurrent' width='90%'>"
                                        "<span class='album'><b>%1 - %2</b></span><br>%3</td>"
                                        "<td valign='top' align='right' width='10%'><a href='musicbrainz:%4 @@@ %5'>"
                                        "<img src='%6'></a></td></tr></table> <table width='100%'><tr><td width='20%'>"
                                        "<a class='menu' href='fetchcover:%6 @@@ %7'><img align='left' valign='center' hspace='2' title='Click for information from amazon.com, right-click for menu.' src='%8'></a>"
                                        "</td><td width='80%' valign='bottom' align='right'>"
                                        "<i>" + i18n( "Never played before" )  + "</i></td>"
                                        "</tr>" )
                             .args( QStringList()
                                    << escapeHTML( m_currentTrack->artist() )
                                    << escapeHTML( m_currentTrack->title() )
                                    << escapeHTML( m_currentTrack->album() )
                                    << escapeHTMLAttr( m_db->escapeString( m_currentTrack->artist() ) )
                                    << escapeHTMLAttr( m_db->escapeString( m_currentTrack->album() ) )
                                    << escapeHTML( locate( "data", "amarok/images/musicbrainz.png" ) )
                                    << escapeHTMLAttr( m_currentTrack->artist() )
                                    << escapeHTMLAttr( m_currentTrack->album() )
                                    << escapeHTMLAttr( m_db->getImageForAlbum( values[1], values[0] ) )
                                    )
                             );
    }
    values.clear();
    names.clear();

    browser->write( "</table></div>" );
    // </Current Track Information>

    // <Favourite Tracks Information>
    m_db->execSql( QString( "SELECT tags.title, tags.url, round( statistics.percentage + 0.5 ) "
                            "FROM tags, artist, statistics "
                            "WHERE tags.artist = artist.id AND artist.name LIKE '%1' AND statistics.url = tags.url "
                            "ORDER BY statistics.percentage DESC "
                            "LIMIT 0,5;" )
                   .arg( m_db->escapeString( m_currentTrack->artist() ) ), &values, &names );

    if ( !values.isEmpty() )
    {
        browser->write( "<br><div class='rbcontent'>" );
        browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
        browser->write( "<tr><th>" + i18n( "Favorite Tracks By This Artist" ) + "</th></tr>" );
        browser->write( "</table>" );
        browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

        for ( uint i = 0; i < values.count(); i += 3 )
            browser->write( QString ( "<tr><td class='song'><a href=\"file:" + values[i + 1].replace( "\"", QCString( "%22" ) ) + "\">"
                                      + values[i] + " <i>(" + i18n( "Score:" ) + " " + values[i + 2] + ")</i></a></td></tr>" ) );

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
                                      "tags.artist = artist.id AND "
                                      "( tags.sampler = 1 OR artist.name LIKE '%2' ) "
                                "ORDER BY tags.track;" )
                       .arg( m_db->escapeString( m_currentTrack->album() ) )
                       .arg( m_db->escapeString( m_currentTrack->artist() ) ), &values, &names );

        if ( !values.isEmpty() )
        {
            browser->write( "<br><div class='rbcontent'>"
                            "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                             "<tr><th>" + i18n( "Tracks On This Album" ) + "</th></tr>"
                            "</table>"
                            "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

            for ( uint i = 0; i < values.count(); i += 3 )
            {
                QString tmp = values[i + 2].stripWhiteSpace() == "" ? "" : values[i + 2] + ". ";
                browser->write( QString ( "<tr><td class='song'><a href=\"file:" + values[i + 1].replace( "\"", QCString( "%22" ) ) + "\">" + tmp + values[i] + "</a></td></tr>" ) );
            }

            values.clear();
            names.clear();

            browser->write( "</table></div>" );
        }
    }
    // </Tracks on this album>

    // <Albums by this artist>
    m_db->execSql( QString( "SELECT DISTINCT album.name, artist.name, album.id, artist.id "
                            "FROM tags, album, artist "
                            "WHERE album.id = tags.album AND tags.artist = artist.id AND album.name <> '' AND artist.name LIKE '%1' "
                            "ORDER BY album.name;" )
                   .arg( m_db->escapeString( m_currentTrack->artist() ) ), &values, &names );

    if ( !values.isEmpty() )
    {
        browser->write( "<br><div class='rbcontent'>" );
        browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
        browser->write( "<tr><th>&nbsp;" + i18n( "Albums By This Artist" ) + "</th></tr>" );
        browser->write( "</table>" );
        browser->write( "<table width='100%' border='0' cellspacing='2' cellpadding='0'>" );

        for ( uint i = 0; i < values.count(); i += 4 )
        {
            browser->write( QStringx ( "<tr>"
                                        "<td class='rbalbum' onClick='window.location.href=\"album:%1 @@@ %2\"' height='42' valign='top'>"
                                         "<a href='fetchcover:%3 @@@ %4'><img align='left' hspace='2' title='Click for information from amazon.com, right-click for menu.' src='%5'></a>"
                                         /* *** UGLY HACK ALERT ***
                                            Without the 2 <br> after %9, hover borks on mouseover.
                                            TODO: find out why + make it nice ;) */
                                         "<b>%6</b><br>%7<br><br>"
                                        "</td>"
                                       "</tr>" )
                            .args( QStringList()
                                    << values[i + 3].replace( "\"", "%22" ) // artist.id
                                    << values[i + 2].replace( "\"", "%22" ) // album.id
                                    << escapeHTMLAttr( values[i + 1] ) // artist.name
                                    << escapeHTMLAttr( values[i + 0] ) // album.name
                                    << escapeHTMLAttr( m_db->getImageForAlbum( values[i + 1], values[i + 0], 50 ) )
                                    << escapeHTML( values[i + 0] ) // album.name
                                    << i18n( "1 Track", "%n Tracks", m_db->albumSongCount( values[i + 3], values[i + 2] ).toInt() )
                                    )
                             );
        }
        browser->write( "</table></div>" );
    }
    // </Albums by this artist>

    browser->write( "<br></html>" );
    browser->end();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::viewImage( const QString& artist, const QString& album )
{
    // Show an image viewer widget
    QWidget *widget = new QWidget( 0, 0, WDestructiveClose );
    widget->setCaption( kapp->makeStdCaption( artist + " - " + album ) );
    QPixmap pixmap( m_db->getImageForAlbum( artist, album, 0 ) );
    widget->setPaletteBackgroundPixmap( pixmap );
    widget->setMinimumSize( pixmap.size() );
    widget->setFixedSize( pixmap.size() );

    widget->show();
}


void ContextBrowser::setStyleSheet()
{
    int pxSize = fontMetrics().height() - 4;

    const QString text = colorGroup().text().name();
    const QString fg   = colorGroup().highlightedText().name();
    const QString bg   = colorGroup().highlight().name();

    //we have to set the color for body due to a KHTML bug
    //KHTML sets the base color but not the text color
    m_styleSheet  = QString( "body { font-size: %1px; color: %2; }" ).arg( pxSize ).arg( text );
    m_styleSheet += QString( "body a { color: %1; }" ).arg( text );

    m_styleSheet += QString( ".menu { margin: 0.4em 0.0em; font-weight: bold; }" );

    //used in the currentlyPlaying block
    //m_styleSheet += QString( ".album { font-weight: bold; }" );

    //header for all sections
    m_styleSheet += QString( "th { text-align: left; color: %1; font-size: %2px; font-weight: bold; background-color: %3; padding: 1px 0.5em; border-bottom: 1px solid #000; }" )
                    .arg( fg ).arg( pxSize + 2 ).arg( bg );

    //rb? dunno, but this is the style for the currentlyPlaying block
    m_styleSheet += QString( ".rbcurrent { border: solid %1 1px; }" ).arg( colorGroup().base().name() );

    //this is the style for the other blocks
    m_styleSheet += QString( ".rbcontent { border: solid %1 1px; }" ).arg( bg );
    m_styleSheet += QString( ".rbcontent:hover { border: solid %1 1px; }" ).arg( text );

    //these anchor rules are a little complex
    m_styleSheet += QString( ".rbcontent a { text-decoration: none; }" );
    m_styleSheet += QString( ".rbcontent .song a { display: block; padding: 1px 2px; }" );
    m_styleSheet += QString( ".rbcontent .song a:hover { color: %1; background-color: %1; }" ).arg( fg ).arg( bg );

    m_styleSheet += QString( ".rbcontent .rbalbum:hover { background-color: %1; cursor: pointer; }" ).arg( bg );
    m_styleSheet += QString( ".rbcontent .rbalbum:hover a { color: %1; }" ).arg( fg );
}


void ContextBrowser::showIntroduction()
{
    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html><div><h2>");
    browser->write( i18n( "Hello amaroK user!" ) );
    browser->write( "</h2><br><br>" );
    browser->write( i18n( "This is the Context Browser: it shows you contextual information about the currently playing track."
                          "In order to use this feature of amaroK, you need to build a collection." ) );
    browser->write( "&nbsp;<a href='show:collectionSetup'>" );
    browser->write( i18n( "Click here to build one..." ) );
    browser->write( "</a></div></html>");

    browser->end();
}


void ContextBrowser::showScanning()
{
    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html><div>");
    browser->write( i18n( "Building Collection Database.." ) );
    browser->write( "</div></html>");

    browser->end();
}


void ContextBrowser::showCurrentStream()
{
    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html><div class='menu'><a class='menu' href='show:home'>" + i18n( "Home" ) + "</a>&nbsp;&nbsp;<a class='menu' href='show:stream'>"
                    + i18n( "Current Stream" ) + "</a></div>");

    // <Stream Information>
    browser->write( "<div class='rbcontent'>" );
    browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
    browser->write( "<tr><th>&nbsp;" + i18n( "Playing Stream:" ) + "</th></tr>" );
    browser->write( "</table>" );
    browser->write( "<table width='100%' border='0' cellspacing='2' cellpadding='0'>" );

    if ( m_currentTrack )
    {
        browser->write( QStringx ( "<tr><td height='42' valign='top' class='rbcurrent' width='90%'>"
                                   "<span class='album'><b>%1 - %2</b></span><br>%3</td><td valign='top' align='right' width='10%'>"
                                   "</td></tr></table>"
                                   "<table width='100%'><tr><td width='20%'><a class='menu' href='fetchcover:%7 @@@ %8'>"
                                   "<img hspace='2' src='%9'></a></td></tr>" )
                        .args( QStringList()
                            << escapeHTML( m_currentTrack->artist() )
                            << escapeHTML( m_currentTrack->title() )
                            << escapeHTML( m_currentTrack->album() )
                            << escapeHTMLAttr( m_currentTrack->artist() )
                            << escapeHTMLAttr( m_currentTrack->album() )
                            << escapeHTMLAttr( m_db->getImageForAlbum( "stream", "stream" ) )
                            )
                        );
    }
    else
    {
        browser->write( QStringx ( "<tr><td height='42' valign='top' class='rbcurrent' width='90%'>"
                                   "<span class='album'><b>%1 - %2</b></span><br>%3</td><td valign='top' align='right' width='10%'>"
                                   "</td></tr></table>"
                                   "<table width='100%'><tr><td width='20%'><a class='menu' href='fetchcover:%7 @@@ %8'>"
                                   "<img hspace='2' src='%9'></a></td></tr>" )
                        .args( QStringList()
                            << escapeHTML( m_url.path() )
                            << escapeHTML( QString::null )
                            << escapeHTML( QString::null )
                            << escapeHTMLAttr( QString::null )
                            << escapeHTMLAttr( QString::null )
                            << escapeHTMLAttr( m_db->getImageForAlbum( "stream", "stream" ) )
                            )
                        );
    }

    browser->write( "</table></div>" );
    // </Stream Information>

    browser->write( "</div></html>");

    browser->end();
}


#include "contextbrowser.moc"
