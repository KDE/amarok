// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information


#include "config.h"

#include "amarokconfig.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "contextbrowser.h"
#include "coverfetcher.h"
#include "covermanager.h"
#include "enginecontroller.h"
#include "metabundle.h"
#include "playlist.h"     //appendMedia()
#include "playlistitem.h"     //appendMedia()
#include "qstringx.h"
#include "scrobbler.h"
#include "sqlite/sqlite3.h"

#include <kactioncollection.h>
#include <kapplication.h> //kapp->config(), QApplication::setOverrideCursor()
#include <kconfig.h>      //config object
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h> //ctor
#include <kurl.h>
#include <kurlcombobox.h>
#include <ktempfile.h> // gradient backgroud image
#include <kimageeffect.h> // gradient backgroud image

#include <qfile.h>
#include <qimage.h>
#include <qlabel.h>
#include <qpushbutton.h>

using amaroK::QStringx;


ContextBrowser::ContextBrowser( const char *name )
: QVBox( 0, name )
, m_currentTrack( 0 ) {
    kdDebug() << k_funcinfo << endl;
    EngineController::instance()->attach( this );
    m_db = new CollectionDB();
    m_scrobbler = new Scrobbler();

    setSpacing( 4 );
    setMargin( 5 );

    KToolBar* toolbar = new KToolBar( this );
    toolbar->setMovingEnabled( false );
    toolbar->setFlat( true );
    toolbar->setIconSize( 16 );
    toolbar->setEnableContextMenu( false );

    KActionCollection* ac = new KActionCollection( this );
    KAction* homeAction = new KAction( i18n( "Home" ), "gohome", 0, this, SLOT( showHome() ), ac, "Home" );
    KAction* trackAction = new KAction( i18n( "Current Track" ), "today", 0, this, SLOT( showCurrentTrack() ), ac, "Current Track" );
    KAction* lyricsAction = new KAction( i18n( "Lyrics" ), "document", 0, this, SLOT( showLyrics() ), ac, "Lyrics" );

    toolbar->setIconText( KToolBar::IconTextRight, false ); //we want the open button to have text on right
    homeAction->plug( toolbar );
    trackAction->plug( toolbar );
    lyricsAction->plug( toolbar );

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
    connect( m_scrobbler,                 SIGNAL( relatedArtistsFetched( QStringList& ) ),
             this,                          SLOT( relatedArtistsFetched( QStringList& ) ) );

    connect( CollectionDB::emitter(),     SIGNAL( scanStarted() ),
             SLOT( collectionScanStarted() ) );
    connect( CollectionDB::emitter(),     SIGNAL( scanDone( bool ) ),
             SLOT( collectionScanDone() ) );
    connect( CollectionDB::emitter(),     SIGNAL( metaDataEdited( const MetaBundle & ) ),
             SLOT( metaDataEdited( const MetaBundle & ) ) );

    if ( m_db->isEmpty() || !m_db->isValid() ) {
        showIntroduction();
        m_emptyDB = true;
    } else {
        showHome();
        m_emptyDB = false;
    }

    setFocusProxy( hb1 ); //so focus is given to a sensible widget when the tab is opened
}


ContextBrowser::~ContextBrowser() {
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

void ContextBrowser::openURLRequest( const KURL &url ) {
    m_url = url;
    QStringList info = QStringList::split( " @@@ ", url.path() );

    if ( url.protocol() == "album" ) {
        QStringList values;

        values = m_db->query( QString( "SELECT DISTINCT url FROM tags WHERE artist = %1 AND album = %2 ORDER BY track;" )
                              .arg( info[0] )
                              .arg( info[1] ), &values );

        for ( uint i = 0; i < values.count(); i++ ) {
            if ( values[i].isEmpty() )
                continue;

            KURL tmp;
            tmp.setPath( values[i] );
            Playlist::instance()->appendMedia( tmp, false, true );
        }
    }

    if ( url.protocol() == "file" )
        Playlist::instance()->appendMedia( url, true, true );

    if ( m_url.protocol() == "show" ) {
        if ( m_url.path() == "home" )
            showHome();
        else if ( m_url.path() == "context" )
            showCurrentTrack();
        else if ( m_url.path() == "stream" )
            showCurrentStream();
        else if ( m_url.path() == "lyrics" )
            showLyrics();
        else if ( m_url.path() == "collectionSetup" ) {
            //TODO if we do move the configuration to the main configdialog change this,
            //     otherwise we need a better solution
            QObject *o = parent()->child( "CollectionBrowser" );
            if ( o )
                static_cast<CollectionBrowser*>( o )->setupDirs();
        }
    }

    // When left-clicking on cover image, open browser with amazon site
    if ( m_url.protocol() == "fetchcover" ) {
        QStringList info = QStringList::split( " @@@ ", m_url.path() );
        QImage img( m_db->albumImage( info[0], info[1], 0 ) );
        const QString amazonUrl = img.text( "amazon-url" );
        kdDebug() << "[ContextBrowser] Embedded amazon url in cover image: " << amazonUrl << endl;

        if ( amazonUrl.isEmpty() )
            KMessageBox::information( this, i18n( "Right-click on image for download menu." ) );
        else
            kapp->invokeBrowser( amazonUrl );
    }

    /* open konqueror with musicbrainz search result for artist-album */
    if ( url.protocol() == "musicbrainz" ) {
        const QString command = "kfmclient openURL 'http://www.musicbrainz.org/taglookup.html?artist=''%1''&album=''%2'''";
        KRun::runCommand( command.arg( info[0] ).arg( info[1] ), "kfmclient", "konqueror" );
    }

}


void ContextBrowser::collectionScanStarted() {
    if( m_emptyDB )
        showScanning();
}


void ContextBrowser::collectionScanDone() {
    // take care of sql updates (schema changed errors)
    delete m_db;
    m_db = new CollectionDB();

    if ( CollectionDB().isEmpty() ) {
        showIntroduction();
        m_emptyDB = true;
    } else if ( m_emptyDB ) {
        showHome();
        m_emptyDB = false;
    }
}


void ContextBrowser::metaDataEdited( const MetaBundle &bundle ) {
    if ( m_currentTrack->url() == bundle.url() ) {
        kdDebug() << "current song edited, updating view" << endl;

        delete m_currentTrack;
        m_currentTrack = new MetaBundle( bundle );
        showCurrentTrack();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::engineTrackEnded( int finalPosition, int trackLength ) {
    //This is where percentages are calculated
    //TODO statistics are not calculated when currentTrack doesn't exist

    if ( m_currentTrack ) {
        // sanity check
        if ( finalPosition > trackLength || finalPosition == 0 )
            finalPosition = trackLength;

        int pct = (int) ( ( (double) finalPosition / (double) trackLength ) * 100 );

        // increase song counter & calculate new statistics
        float score = m_db->addSongPercentage( m_currentTrack->url().path(), pct );

        // TODO speedtest
        if ( score ) {
            QListViewItemIterator it( Playlist::instance() );
            for( ; it.current(); ++it ) {
                PlaylistItem *item = static_cast<PlaylistItem*>(*it);
                if ( item->url() == m_currentTrack->url() )
                    item->setText( PlaylistItem::Score, QString::number( score ) );
            }
        }
    }
}


void ContextBrowser::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ ) {
    m_relatedArtists.clear();
    delete m_currentTrack;
    m_currentTrack = new MetaBundle( bundle );

    if ( m_db->isEmpty() || !m_db->isValid() )
        showIntroduction();
    else
        if ( EngineController::engine()->isStream() )
            showCurrentStream();
        else
            showCurrentTrack();
}


void ContextBrowser::engineStateChanged( Engine::State state ) {
    switch( state ) {
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


void ContextBrowser::paletteChange( const QPalette& pal ) {
    kdDebug() << k_funcinfo << endl;

    QVBox::paletteChange( pal );

    setStyleSheet();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::slotContextMenu( const QString& urlString, const QPoint& point ) {
    KURL url( urlString );

    if ( url.protocol() == "fetchcover" ) {
        QStringList info = QStringList::split( " @@@ ", url.path() );
        enum menuIds { SHOW, FETCH, DELETE, MANAGER };

        KPopupMenu menu( this );
        menu.insertTitle( i18n( "Cover Image" ) );
        menu.insertItem( SmallIcon( "viewmag" ), i18n( "Show Fullsize" ), SHOW );
        menu.setItemEnabled( SHOW, !m_db->albumImage( info[0], info[1], 0 ).contains( "nocover" ) );
        menu.insertItem( SmallIcon( "www" ), i18n( "Fetch From amazon.com" ), FETCH );
        #ifndef AMAZON_SUPPORT

        menu.setItemEnabled( FETCH, false );
        #endif

        menu.insertItem( QPixmap( locate( "data", "amarok/images/covermanager.png" ) ), i18n( "Cover Manager" ), MANAGER );
        menu.insertSeparator();
        menu.insertItem( SmallIcon( "editdelete" ), i18n("Delete Image File"), DELETE );
        int id = menu.exec( point );

        CoverManager *coverManager; // Forward declaration

        switch ( id ) {
        case SHOW:
            /* open an image view widget */
            CoverManager::viewCover( info[0], info[1], this );
            break;

        case FETCH:
            #ifdef AMAZON_SUPPORT
            /* fetch covers from amazon on click */
            m_db->fetchCover( this, info[0], info[1], false );
            #else

            if( m_db->getImageForAlbum( info[0], info[1], 0 ) != locate( "data", "amarok/images/nocover.png" ) )
                CoverManager::viewCover( info[0], info[1], this );
            else {
                /* if no cover exists, open a file dialog to add a cover */
                KURL file = KFileDialog::getImageOpenURL( ":homedir", this, i18n( "Select cover image file - amaroK" ) );
                if ( !file.isEmpty() ) {
                    QImage img( file.directory() + "/" + file.fileName() );
                    QString filename( QFile::encodeName( info[0] + " - " + info[1] ) );
                    filename.replace( " ", "_" ).append( ".png" );
                    img.save( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() )+"/albumcovers/"+filename.lower(), "PNG" );
                    showCurrentTrack();
                }
            }
            #endif
            break;

        case MANAGER:
            coverManager = new CoverManager();
            coverManager->show();
            break;

        case DELETE:
            int button = KMessageBox::warningContinueCancel( this,
                         i18n( "Are you sure you want to delete this cover?" ),
                         QString::null,
                         i18n("&Delete Confirmation") );

            if ( button == KMessageBox::Continue ) {
                m_db->removeAlbumImage( info[0], info[1] );
                showCurrentTrack();
            }
        }
    }

    if ( url.protocol() == "file" ) {
        enum menuIds { APPEND, ASNEXT, MAKE };

        KPopupMenu menu( this );
        menu.insertTitle( i18n( "Track" ) );
        menu.insertItem( SmallIcon( "player_playlist_2" ), i18n( "&Append To Playlist" ), APPEND );
        //menu.setItemEnabled( APPEND );
        menu.insertItem( SmallIcon( "next" ), i18n( "&Queue After Current Track" ), ASNEXT );
        menu.insertItem( SmallIcon( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );

        switch ( menu.exec( point ) ) {
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

        }
    }
    if ( url.protocol() == "album" ) {
        enum menuIds { APPEND, ASNEXT, MAKE };

        KPopupMenu menu( this );
        menu.insertTitle( i18n( "Album" ) );
        menu.insertItem( SmallIcon( "player_playlist_2" ), i18n( "&Append To Playlist" ), APPEND );
        menu.insertItem( SmallIcon( "next" ), i18n( "&Queue After Current Track" ), ASNEXT );
        menu.insertItem( SmallIcon( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );

        int id = menu.exec( point );

        QStringList list = QStringList::split( " @@@ ", url.path() );
        QStringList values;

        values = m_db->query( QString( "select distinct url from tags where artist = '%1' and album = '%2' order by track;" )
                              .arg( list[0] )
                              .arg( list[1] ) );

        switch ( id ) {
        case APPEND:
            for ( uint i = 0; i < values.count(); i++ ) {
                if ( values[i].isEmpty() )
                    continue;

                KURL tmp;
                tmp.setPath( values[i] );
                Playlist::instance()->appendMedia( tmp, false, true );
            }
            break;

        case ASNEXT:
            for ( uint i = 0; i < values.count(); i++ ) {
                if ( values[i].isEmpty() )
                    continue;

                KURL tmp;
                tmp.setPath( values[i] );
                Playlist::instance()->queueMedia( tmp );
            }
            break;

        case MAKE:
            Playlist::instance()->clear();

            for ( uint i = 0; i < values.count(); i++ ) {
                if ( values[i].isEmpty() )
                    continue;

                KURL tmp;
                tmp.setPath( values[i] );
                Playlist::instance()->appendMedia( tmp, false, true );
            }
            break;
        }
    }
}

void ContextBrowser::showHome() //SLOT
{
    QStringList values;

    // Triggers redisplay when new cover image is downloaded
    #ifdef AMAZON_SUPPORT

    connect( CollectionDB::emitter(), SIGNAL( coverFetched(const QString&) ), this, SLOT( showCurrentTrack() ) );
    #endif

    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html>" );

    browser->write( "<div class='rbcontent'>" );
    browser->write(  "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
    browser->write(   "<tr><th>" + i18n( "Your Favorite Tracks" ) + "</th></tr>" );
    browser->write(  "</table>" );
    browser->write(  "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    values = m_db->query( "SELECT tags.title, tags.url, round( statistics.percentage + 0.4 ), artist.name, album.name "
                          "FROM tags, artist, album, statistics "
                          "WHERE artist.id = tags.artist AND album.id = tags.album AND statistics.url = tags.url "
                          "ORDER BY statistics.percentage DESC "
                          "LIMIT 0,10;" );

    if ( !values.isEmpty() ) {
        for ( uint i = 0; i < values.count(); i = i + 5 )
            browser->write( QString ( "<tr><td class='song'><a href=\"file:"
                                      + values[i+1].replace( "\"", QCString( "%22" ) ) + "\"><b>" + values[i]
                                      + "</b> <i>(" + i18n( "Score:" ) + " " + values[i+2] + ")</i><br>" + values[i+3] + " - " + values[i+4] + "</a></td></tr>" ) );
    }

    browser->write( "</table></div><br>" );
    // </Favorite Tracks Information>

    // <Recent Tracks Information>
    browser->write( "<div class='rbcontent'>"
                    "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                    "<tr><th>" + i18n( "Your Newest Tracks" ) + "</th></tr>"
                    "</table>"
                    "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    values = m_db->query( "SELECT tags.title, tags.url, artist.name, album.name "
                          "FROM tags, artist, album "
                          "WHERE artist.id = tags.artist AND album.id = tags.album "
                          "ORDER BY tags.createdate DESC "
                          "LIMIT 0,10;" );

    if ( !values.isEmpty() ) {
        for ( uint i = 0; i < values.count(); i = i + 4 )
            browser->write( QString ( "<tr><td class='song'><a href=\"file:"
                                      + values[i+1].replace( "\"", QCString( "%22" ) ) + "\"'><b>" + values[i]
                                      + "</b><br>" + values[i+2] + " - " + values[i+3] + "</a></td></tr>" ) );
    }

    browser->write( "</table></div><br>" );
    // </Recent Tracks Information>

    browser->write( "</html>" );
    browser->end();
}


void ContextBrowser::showCurrentTrack() //SLOT
{
    #define escapeHTML(s)     QString(s).replace( "<", "&lt;" ).replace( ">", "&gt;" )
    #define escapeHTMLAttr(s) QString(s).replace( "%", "%25" ).replace( "'", "%27" )

    if ( EngineController::engine()->isStream() )
        showCurrentStream();

    if ( !m_currentTrack )
        return;

    QStringList values;

    uint artist_id = m_db->artistID( m_currentTrack->artist() );
    uint album_id  = m_db->albumID ( m_currentTrack->album() );

    // Triggers redisplay when new cover image is downloaded
    connect( CollectionDB::emitter(), SIGNAL( coverFetched() ), this, SLOT( showCurrentTrack() ) );

    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html>" );

    // <Current Track Information>
    browser->write( "<div class='rbcontent'>"
                    "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                    "<tr><th>" + i18n( "Currently Playing" ) + "</th></tr>"
                    "</table>"
                    "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    values = m_db->query( QString( "SELECT datetime( datetime( statistics.createdate, 'unixepoch' ), 'localtime' ), "
                                   "datetime( datetime( statistics.accessdate, 'unixepoch' ), 'localtime' ), statistics.playcounter, round( statistics.percentage + 0.4 ) "
                                   "FROM  statistics "
                                   "WHERE url = '%1';" )
                          .arg( m_db->escapeString( m_currentTrack->url().path() ) ) );

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
                                   "<img align='left' hspace='2' title='Click for information from amazon.com, right-click for menu.' src='%10'>"
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
                               << escapeHTMLAttr( m_db->albumImage( m_currentTrack->artist(), m_currentTrack->album() ) )
                               << i18n( "Track played once", "Track played %n times", values[2].toInt() )
                               << i18n( "Score: %1" ).arg( values[3] )
                               << i18n( "Last play: %1" ).arg( values[1].left( values[1].length() - 3 ) )
                               << i18n( "First play: %1" ).arg( values[0].left( values[0].length() - 3 ) )
                             )
                      );
    else {
        browser->write( QStringx ( "<tr><td height='42' valign='top' class='rbcurrent' width='90%'>"
                                   "<span class='album'><b>%1 - %2</b></span><br>%3</td>"
                                   "<td valign='top' align='right' width='10%'><a href='musicbrainz:%4 @@@ %5'>"
                                   "<img src='%6'></a></td></tr></table> <table width='100%'><tr><td width='20%'>"
                                   "<a class='menu' href='fetchcover:%6 @@@ %7'><img align='left' hspace='2' title='Click for information from amazon.com, right-click for menu.' src='%8'></a>"
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
                               << escapeHTMLAttr( m_db->albumImage( m_currentTrack->artist(), m_currentTrack->album() ) )
                             )
                      );
    }
    values.clear();

    browser->write( "</table></div>" );
    // </Current Track Information>

    if ( !m_db->isFileInCollection( m_currentTrack->url().path() ) ) {
        browser->write( "<div class='warning' style='padding: 1em 0.5em 2em 0.5em'>");
        browser->write(   i18n("If you would like to see contextual information about this track, "
                               "you must add it to your Collection.") );
        browser->write(  "&nbsp;"
                         "<a class='warning' href='show:collectionSetup'>" );
        browser->write(    i18n( "Click here to change your Collection setup" ) );
        browser->write(  "</a>."
                         "</div>" );
    }

    // scrobblaaaaaar
    if ( m_relatedArtists.isEmpty() )
        m_scrobbler->relatedArtists( m_currentTrack->artist() );
    else {
        QString token;

        for ( uint i = 0; i < m_relatedArtists.count(); i++ ) {
            if ( i > 0 )
                token += " OR ";
            token += " artist.name = '" + m_db->escapeString( m_relatedArtists[i] ) + "' ";
        }

        values = m_db->query( QString( "SELECT tags.title, tags.url, round( statistics.percentage + 0.4 ), artist.name "
                                       "FROM tags, artist, statistics "
                                       "WHERE tags.artist = artist.id AND ( %1 ) AND statistics.url = tags.url "
                                       "ORDER BY statistics.percentage DESC "
                                       "LIMIT 0,5;" )
                              .arg( token ) );

        if ( !values.isEmpty() ) {
            browser->write( "<br><div class='rbcontent'>" );
            browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
            browser->write( "<tr><th>" + i18n( "Suggested Songs" ) + "</th></tr>" );
            browser->write( "</table>" );
            browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

            for ( uint i = 0; i < values.count(); i += 4 )
                browser->write( QString ( "<tr><td class='song'><a href=\"file:" + values[i + 1].replace( "\"", QCString( "%22" ) ) + "\">"
                                          + values[i + 3] + " - " + values[i] + " <i>(" + i18n( "Score:" ) + " " + values[i + 2] + ")</i></a></td></tr>" ) );

            browser->write( "</table></div>" );
        }

        values.clear();
    }

    // <Favourite Tracks Information>
    values = m_db->query( QString( "SELECT tags.title, tags.url, round( statistics.percentage + 0.4 ) "
                                   "FROM tags, statistics "
                                   "WHERE tags.artist = %1 AND statistics.url = tags.url "
                                   "ORDER BY statistics.percentage DESC "
                                   "LIMIT 0,5;" )
                          .arg( artist_id ) );

    if ( !values.isEmpty() ) {
        browser->write( "<br><div class='rbcontent'>" );
        browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
        browser->write( "<tr><th>" + i18n( "Favorite Tracks By This Artist" ) + "</th></tr>" );
        browser->write( "</table>" );
        browser->write( "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

        for ( uint i = 0; i < values.count(); i += 3 )
            browser->write( QString ( "<tr><td class='song'><a href=\"file:" + values[i + 1].replace( "\"", QCString( "%22" ) ) + "\">"
                                      + values[i] + " <i>(" + i18n( "Score:" ) + " " + values[i + 2] + ")</i></a></td></tr>" ) );

        values.clear();

        browser->write( "</table></div>" );
    }
    // </Favourite Tracks Information>

    // <Tracks on this album>
    if ( !m_currentTrack->album().isEmpty() && !m_currentTrack->artist().isEmpty() ) {
        values = m_db->query( QString( "SELECT title, url, track "
                                       "FROM tags "
                                       "WHERE album = %1 AND "
                                       "( tags.sampler = 1 OR tags.artist = %2 ) "
                                       "ORDER BY tags.track;" )
                              .arg( album_id )
                              .arg( artist_id ) );

        if ( !values.isEmpty() ) {
            browser->write( "<br><div class='rbcontent'>"
                            "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                            "<tr><th>" + i18n( "Tracks On This Album" ) + "</th></tr>"
                            "</table>"
                            "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

            for ( uint i = 0; i < values.count(); i += 3 ) {
                QString tmp = values[i + 2].stripWhiteSpace() == "" ? "" : values[i + 2] + ". ";
                browser->write( QString ( "<tr><td class='song'><a href=\"file:" + values[i + 1].replace( "\"", QCString( "%22" ) ) + "\">" + tmp + values[i] + "</a></td></tr>" ) );
            }

            values.clear();

            browser->write( "</table></div>" );
        }
    }
    // </Tracks on this album>

    // <Albums by this artist>
    values = m_db->query( QString( "SELECT DISTINCT album.name, album.id "
                                   "FROM tags, album "
                                   "WHERE album.id = tags.album AND tags.artist = %1 AND album.name <> '' "
                                   "ORDER BY album.name;" )
                          .arg( artist_id ) );

    if ( !values.isEmpty() ) {
        browser->write( "<br><div class='rbcontent'>" );
        browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
        browser->write( "<tr><th>&nbsp;" + i18n( "Albums By This Artist" ) + "</th></tr>" );
        browser->write( "</table>" );
        browser->write( "<table width='100%' border='0' cellspacing='2' cellpadding='0'>" );

        for ( uint i = 0; i < values.count(); i += 2 ) {
            browser->write( QStringx ( "<tr>"
                                       "<td class='rbalbum' onClick='window.location.href=\"album:%1 @@@ %2\"' height='42' valign='top'>"
                                       "<a href='fetchcover:%3 @@@ %4'><img align='left' hspace='2' title='Click for information from amazon.com, right-click for menu.' src='%5'></a>"
                                       /* *** UGLY HACK ALERT ***
                                          Without the 2 <br> after %9, hover borks on mouseover.
                                          TODO: find out why + make it nice ;) */
                                       "<a href='album:%6 @@@ %7'><b>%8</b><br>%9<br><br></a>"
                                       "</td>"
                                       "</tr>" )
                            .args( QStringList()
                                   << QString::number( artist_id )
                                   << values[ i+1 ] //album.id
                                   << escapeHTMLAttr( m_currentTrack->artist() ) // artist name
                                   << escapeHTMLAttr( values[ i ] ) // album.name
                                   << escapeHTMLAttr( m_db->albumImage( m_currentTrack->artist(), values[ i ], 50 ) )
                                   << QString::number( artist_id )
                                   << values[ i+1 ] //album.id
                                   << escapeHTML( values[ i ] ) // album.name
                                   << i18n( "1 Track", "%n Tracks", m_db->albumSongCount( QString::number(artist_id), values[ i+1 ] ).toInt() )
                                 ) );
        }
        browser->write( "</table></div>" );
    }
    // </Albums by this artist>

    browser->write( "<br></html>" );
    browser->end();
}


namespace amaroK {
class Color : public QColor {
        static const int CONTRAST = 130;
        static const int SATURATION_TARGET = 30;
public:
        Color( const QColor &c )
: QColor( c ) {
            int h,s1,s,v1,v;
            getHsv( &h, &s1, &v1 );

            kdDebug() << "Initial Color Properties: s:" << s1 << " v:" << v1 << endl;

            //we want the new colour to be low saturation
            //TODO what if s is less than SATURATION_TARGET to start with
            s = s1 - CONTRAST;
            v = v1;

            if ( s < SATURATION_TARGET ) {
                int remainingContrast = SATURATION_TARGET - s;
                s = SATURATION_TARGET;

                kdDebug() << "Unapplied Contrast: " << remainingContrast << endl;

                //we only add to the value to avoid the dreaded "grey-gradient"
                v += remainingContrast;

                if ( v > 255 ) {
                    int error = v - 255;
                    kdDebug() << "Over-compensation: " << error << endl;

                    //if the error is significant then this must be a pretty bright colour
                    //it would look better if the gradient was dark
                    if( error > CONTRAST/2 )
                        v = v1 - error;
                    else
                        v = 255;
                }
            }

            setHsv( h, s, v );

            kdDebug() << "Final Colour Properties: s:" << s << " v:" << v << endl;
        }
    };
}

void ContextBrowser::setStyleSheet() {
    kdDebug() << k_funcinfo << endl;

    int pxSize = fontMetrics().height() - 4;

    const QString text = colorGroup().text().name();
    const QString fg   = colorGroup().highlightedText().name();
    const QString bg   = colorGroup().highlight().name();

    amaroK::Color gradient = colorGroup().highlight();

    //writing temp gradient image
    KTempFile temp_img(locateLocal("tmp", "gradient"), ".png", 0600);
    QImage grad = KImageEffect::gradient(QSize(600,1), gradient, gradient.light(), KImageEffect::PipeCrossGradient, 3);
    grad.save( temp_img.file(), "PNG" );
    temp_img.close();

    //we have to set the color for body due to a KHTML bug
    //KHTML sets the base color but not the text color
    m_styleSheet  = QString( "body { margin: 8px; font-size: %1px; color: %2; background-color: %3; background-image: url( %4 ); backgroud-repeat: repeat-y; }" )
                    .arg( pxSize ).arg( text ).arg( AmarokConfig::schemeAmarok() ? fg : gradient.name() )
                    .arg( temp_img.name() );
    m_styleSheet += QString( "a { font-size: %1px; color: %2; }" ).arg( pxSize ).arg( text );

    m_styleSheet += QString( ".menu { color: %1; background-color: %2; margin: 0.4em 0.0em; font-weight: bold; }" ).arg( fg ).arg( bg );

    //used in the currentlyPlaying block
    m_styleSheet += QString( ".stream { font-size: %1px; }" ).arg( pxSize );
    m_styleSheet += QString( ".warning { font-size: %1px; color: %2; font-weight: bold; }" ).arg( pxSize ).arg( bg );

    //set background for all tables
    m_styleSheet += QString( "table { background-color: %1; }" ).arg( colorGroup().base().name() );

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

    browser->setUserStyleSheet( m_styleSheet );
}


void ContextBrowser::showIntroduction() {
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


void ContextBrowser::showScanning() {
    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html><div>");
    browser->write( i18n( "Building Collection Database.." ) );
    browser->write( "</div></html>");

    browser->end();
}


void ContextBrowser::showCurrentStream() {
    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html>" );

    // <Stream Information>
    browser->write( "<div class='rbcontent'>" );
    browser->write( "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );
    browser->write( "<tr><th>&nbsp;" + i18n( "Playing Stream:" ) + "</th></tr>" );
    browser->write( "</table>" );
    browser->write( "<table width='100%' border='0' cellspacing='2' cellpadding='0'>" );

    if ( m_currentTrack ) {
        browser->write( QStringx ( "<tr><td height='42' valign='top' class='rbcurrent' width='90%'>"
                                   "<span class='stream'><b>%1</b><br/>%2<br/>%3<br/>%4</span></td>"
                                   "<td valign='top' align='right' width='10%'> </td></tr></table>" )
                        .args( QStringList()
                               << escapeHTML( m_currentTrack->prettyTitle() )
                               << escapeHTML( m_currentTrack->genre() )
                               << escapeHTML( m_currentTrack->prettyBitrate() )
                               << escapeHTML( m_currentTrack->prettyURL() )
                             )
                      );
    } else {
        browser->write( QStringx ( "<tr><td height='42' valign='top' class='rbcurrent' width='90%'>"
                                   "<span class='stream'><b>%1</b></span></td>"
                                   "<td valign='top' align='right' width='10%'></td></tr></table>" )
                        .args( QStringList()
                               << escapeHTML( m_url.path() )
                             )
                      );
    }

    browser->write( "</table></div>" );
    // </Stream Information>

    browser->write( "</div></html>" );

    browser->end();
}


// THE FOLLOWING CODE IS COPYRIGHT BY
// Christian Muehlhaeuser
// <chris at chris.de>
// If I'm violating any copyright or such
// please contact / sue me. Thanks.

void ContextBrowser::showLyrics() {
    if ( !m_currentTrack )
        return;

    QString url = QString( "http://lyrc.com.ar/en/tema1en.php?artist=%1&songname=%2" )
                  .arg( m_currentTrack->artist() )
                  .arg( m_currentTrack->title() );

    kdDebug() << "Using this url: " << url << endl;
    m_lyrics = "";

    KIO::TransferJob* job = KIO::get
                                ( url, false, false );
    connect( job, SIGNAL( result( KIO::Job* ) ),
             this,  SLOT( lyricsResult( KIO::Job* ) ) );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,  SLOT( lyricsData( KIO::Job*, const QByteArray& ) ) );
}


void
ContextBrowser::lyricsData( KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
    m_lyrics += QString( data );
}


void
ContextBrowser::lyricsResult( KIO::Job* job ) //SLOT
{
    if ( !job->error() == 0 ) {
        kdWarning() << "[LyricsFetcher] KIO error! errno: " << job->error() << endl;
        return;
    }

    m_lyrics = m_lyrics.mid( m_lyrics.find( "<font size='2'>" ) );
    if ( m_lyrics.find( "<p><hr" ) )
        m_lyrics = m_lyrics.mid( 0, m_lyrics.find( "<p><hr" ) );
    else
        m_lyrics = m_lyrics.mid( 0, m_lyrics.find( "<br><br>" ) );

    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<div>" );
    browser->write( m_lyrics );
    browser->write( "</div></html>" );
    browser->end();
}


void
ContextBrowser::relatedArtistsFetched( QStringList& artists ) {
    kdDebug() << "artists retrieved" << endl;
    m_relatedArtists = artists;

    // trigger update
    showCurrentTrack();
}


#include "contextbrowser.moc"
