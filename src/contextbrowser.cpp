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
#include "playlist.h"      //appendMedia()
#include "playlistitem.h"  //statistics stuff
#include "qstringx.h"
#include "scrobbler.h"
#include "sqlite/sqlite3.h"

#include <kapplication.h> //kapp
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h> //ctor
#include <kurl.h>
#include <kimageeffect.h> // gradient backgroud image
#include <qimage.h>


using amaroK::QStringx;


ContextBrowser::ContextBrowser( const char *name )
   : QVBox( 0, name )
   , m_db( new CollectionDB )
   , m_scrobbler( new Scrobbler )
   , m_gradientImage( 0 )
{
    EngineController::instance()->attach( this );

    m_toolbar = new KToolBar( this );
    m_toolbar->setFlat( true );
    m_toolbar->setIconSize( 16 );
    m_toolbar->setIconText( KToolBar::IconTextRight );
    m_toolbar->insertButton( "gohome", Home, SIGNAL(clicked()), this, SLOT(showHome()), true, i18n("Home") );
    m_toolbar->insertButton( "today", CurrentTrack, SIGNAL(clicked()), this, SLOT(showCurrentTrack()), true, i18n("Current Track") );
    m_toolbar->insertButton( "document", Lyrics, SIGNAL(clicked()), this, SLOT(showLyrics()), true, i18n("Lyrics") );

    browser = new KHTMLPart( this );
    browser->setDNDEnabled( true );

    setSpacing( 4 );
    setMargin( 5 );
    setFocusProxy( browser->view() ); //so focus is given to a sensible widget when the tab is opened

    connect( browser->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                          SLOT( openURLRequest( const KURL & ) ) );
    connect( browser,                     SIGNAL( popupMenu( const QString&, const QPoint& ) ),
             this,                          SLOT( slotContextMenu( const QString&, const QPoint& ) ) );
    connect( m_scrobbler,                 SIGNAL( relatedArtistsFetched( QStringList& ) ),
             this,                          SLOT( relatedArtistsFetched( QStringList& ) ) );

    connect( CollectionDB::emitter(), SIGNAL(scanStarted()), SLOT(collectionScanStarted()) );
    connect( CollectionDB::emitter(), SIGNAL(scanDone( bool )), SLOT(collectionScanDone()) );
    connect( CollectionDB::emitter(), SIGNAL(metaDataEdited( const MetaBundle& )), SLOT(metaDataEdited( const MetaBundle& )) );
    connect( CollectionDB::emitter(), SIGNAL(coverFetched()), SLOT(showCurrentTrack()) );

    //the stylesheet will be set up and home will be shown later due to engine signals and doodaa
    //if we call it here setStyleSheet is called 3 times during startup!!
}


ContextBrowser::~ContextBrowser()
{
    delete m_db;
    delete m_scrobbler;

    if( m_gradientImage )
      m_gradientImage->unlink();

    EngineController::instance()->detach( this );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::setFont( const QFont &newFont ) {
    if( newFont != font() ) {
        QWidget::setFont( newFont );
        setStyleSheet();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::openURLRequest( const KURL &url ) {
    QStringList info = QStringList::split( " @@@ ", url.path() );

    if ( url.protocol() == "album" ) {
        QString sql = "SELECT DISTINCT url FROM tags WHERE artist = %1 AND album = %2 ORDER BY track;";
        QStringList values = m_db->query( sql.arg( info[0] ).arg( info[1] ) );
        KURL::List urls;
        KURL url;

        for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it ) {
            url.setPath( *it );
            urls.append( url );
        }

        Playlist::instance()->insertMedia( urls, Playlist::Unique );

        return;
    }

    if ( url.protocol() == "file" )
        Playlist::instance()->insertMedia( url, Playlist::DirectPlay | Playlist::Unique );

    if ( url.protocol() == "show" ) {
        if ( url.path() == "home" )
            showHome();
        else if ( url.path() == "context" || url.path() == "stream" )
            showCurrentTrack();
        else if ( url.path() == "lyrics" )
            showLyrics();
        else if ( url.path() == "collectionSetup" ) {
            //TODO if we do move the configuration to the main configdialog change this,
            //     otherwise we need a better solution
            QObject *o = parent()->child( "CollectionBrowser" );
            if ( o )
                static_cast<CollectionBrowser*>( o )->setupDirs();
        }
    }

    // When left-clicking on cover image, open browser with amazon site
    if ( url.protocol() == "fetchcover" ) {
        QStringList info = QStringList::split( " @@@ ", url.path() );
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
    if ( EngineController::instance()->bundle().url() == bundle.url() ) {
        kdDebug() << "Current song edited, updating Context Browser" << endl;
        showCurrentTrack();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::engineTrackEnded( int finalPosition, int trackLength ) {
    //This is where percentages are calculated
    //TODO statistics are not calculated when currentTrack doesn't exist

    const KURL &url = EngineController::instance()->bundle().url();

    // sanity check
    if ( finalPosition > trackLength || finalPosition == 0 )
        finalPosition = trackLength;

    int pct = (int) ( ( (double) finalPosition / (double) trackLength ) * 100 );

    // increase song counter & calculate new statistics
    float score = m_db->addSongPercentage( url.path(), pct );

    // TODO speedtest
    if( score ) {
        QListViewItemIterator it( Playlist::instance() );
        for( ; it.current(); ++it ) {
            PlaylistItem *item = static_cast<PlaylistItem*>(*it);
            if ( item->url() == url )
                item->setText( PlaylistItem::Score, QString::number( score ) );
        }
    }
}


void ContextBrowser::engineNewMetaData( const MetaBundle&, bool /*trackChanged*/ ) {
    m_relatedArtists.clear();

    switch( m_db->isEmpty() || !m_db->isValid() ) {
        case true:  showIntroduction();
        case false: showCurrentTrack();
    }
}


void ContextBrowser::engineStateChanged( Engine::State state ) {
    switch( state ) {
    case Engine::Empty:
        m_toolbar->getButton( CurrentTrack )->setEnabled( false );
        m_toolbar->getButton( Lyrics )->setEnabled( false );
        showHome();
        break;
    case Engine::Playing:
        m_toolbar->getButton( CurrentTrack )->setEnabled( true );
        m_toolbar->getButton( Lyrics )->setEnabled( true );
        break;
    default:
        ;
    }
}


void ContextBrowser::paletteChange( const QPalette& pal ) {
    QVBox::paletteChange( pal );
    setStyleSheet();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::slotContextMenu( const QString& urlString, const QPoint& point )
{
    enum { SHOW, FETCH, DELETE, APPEND, ASNEXT, MAKE, MANAGER, TITLE };

    if( urlString.isEmpty() || urlString.startsWith( "musicbrainz" ) )
       return;

    KPopupMenu menu;
    KURL url( urlString );
    KURL::List urls( url );
    const QStringList info = QStringList::split( " @@@ ", url.path() );

    if ( url.protocol() == "fetchcover" ) {
        menu.insertTitle( i18n( "Cover Image" ) );

        menu.insertItem( SmallIcon( "viewmag" ), i18n( "&Show Fullsize" ), SHOW );
        menu.insertItem( i18n( "&Fetch From amazon.com" ), FETCH );
        menu.insertSeparator();

        menu.insertItem( SmallIcon( "editdelete" ), i18n("&Delete Image File"), DELETE );
        menu.insertSeparator();
        menu.insertItem( QPixmap( locate( "data", "amarok/images/covermanager.png" ) ), i18n( "Cover Manager" ), MANAGER );

        #ifndef AMAZON_SUPPORT
        menu.setItemEnabled( FETCH, false );
        #endif
        menu.setItemEnabled( SHOW, !m_db->albumImage( info[0], info[1], 0 ).contains( "nocover" ) );
    }
    else {
        //TODO it would be handy and more usable to have this menu under the cover one too

        menu.insertTitle( i18n("Track"), TITLE );

        menu.insertItem( SmallIcon( "player_playlist_2" ), i18n( "&Append To Playlist" ), APPEND );
        menu.insertItem( SmallIcon( "next" ), i18n( "&Queue After Current Track" ), ASNEXT );
        menu.insertItem( SmallIcon( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );

        if ( url.protocol() == "album" ) {
            QString sql = "select distinct url from tags where artist = '%1' and album = '%2' order by track;";
            QStringList values = m_db->query( sql.arg( info[0] ).arg( info[1] ) );

            urls.clear(); //remove urlString
            KURL url;
            for( QStringList::ConstIterator it = values.begin(); it != values.end(); ++it ) {
                url.setPath( *it );
                urls.append( url );
            }

            menu.changeTitle( TITLE, i18n("Album") );
        }
    }

    //Not all these are used in the menu, it depends on the context
    switch( menu.exec( point ) )
    {
    case SHOW:
        CoverManager::viewCover( info[0], info[1], this );
        break;

    case DELETE:
    {
        const int button = KMessageBox::warningContinueCancel( this,
            i18n( "This cover will be permanantly deleted" ),
            QString::null,
            i18n("&Delete") );

        if ( button == KMessageBox::Continue ) {
            m_db->removeAlbumImage( info[0], info[1] );
            showCurrentTrack();
        }
        break;
    }

    case ASNEXT:
        Playlist::instance()->insertMedia( urls, Playlist::Queue );
        break;

    case MAKE:
        Playlist::instance()->clear();

        //FALL_THROUGH

    case APPEND:
        Playlist::instance()->insertMedia( urls, Playlist::Unique );
        break;

    case FETCH:
    #ifdef AMAZON_SUPPORT
        /* fetch covers from amazon on click */
        m_db->fetchCover( this, info[0], info[1], false );
        break;
    #else
        if ( m_db->getImageForAlbum( info[0], info[1], 0 ) == locate( "data", "amarok/images/nocover.png" ) ) {
            /* if no cover exists, open a file dialog to add a cover */
            KURL file = KFileDialog::getImageOpenURL( ":homedir", this, i18n( "Select Cover Image File" ) );
            if ( !file.isEmpty() ) {
                QImage img( file.directory() + '/' + file.fileName() );
                QString filename( QFile::encodeName( info[0] + " - " + info[1] ) );
                filename.replace( ' ', '_' ).append( ".png" );
                img.save( KGlobal::dirs()->saveLocation( "data", "amarok/albumcovers/"+filename.lower(), "PNG" );
                showCurrentTrack();
            }
        }
        else CoverManager::viewCover( info[0], info[1], this );
        break;
    #endif

    case MANAGER:
        (new CoverManager())->show();
        break;
    }
}

void ContextBrowser::showHome() //SLOT
{
    QStringList fave = m_db->query(
        "SELECT tags.title, tags.url, round( statistics.percentage + 0.4 ), artist.name, album.name "
        "FROM tags, artist, album, statistics "
        "WHERE artist.id = tags.artist AND album.id = tags.album AND statistics.url = tags.url "
        "ORDER BY statistics.percentage DESC "
        "LIMIT 0,10;" );

    QStringList recent = m_db->query(
        "SELECT tags.title, tags.url, artist.name, album.name "
        "FROM tags, artist, album "
        "WHERE artist.id = tags.artist AND album.id = tags.album "
        "ORDER BY tags.createdate DESC "
        "LIMIT 0,10;" );

    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );
    browser->write( "<html>" );

    // <Favorite Tracks Information>
    browser->write(
        "<div class='rbcontent'>"
         "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
          "<tr><th>" + i18n( "Your Favorite Tracks" ) + "</th></tr>"
         "</table>"
         "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    for( uint i = 0; i < fave.count(); i = i + 5 )
        browser->write(
            "<tr>"
             "<td class='song'>"
              "<a href='file:" + fave[i+1].replace( '"', QCString("%22") ) + "'>"
               "<b>" + fave[i] + "</b> "
               "<i>(" + i18n("Score: %1").arg( fave[i+2] ) + ")</i><br>" +
               fave[i+3] + " - " + fave[i+4] +
              "</a>"
             "</td>"
            "</tr>" );

    browser->write(
         "</table>"
        "</div>"
        "<br>"

    // </Favorite Tracks Information>

    // <Recent Tracks Information>

        "<div class='rbcontent'>"
         "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
          "<tr><th>" + i18n( "Your Newest Tracks" ) + "</th></tr>"
         "</table>"
         "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

    for( uint i = 0; i < recent.count(); i = i + 4 )
        browser->write(
            "<tr>"
             "<td class='song'>"
              "<a href='file:" + recent[i+1].replace( '"', QCString("%22") ) + "'>"
               "<b>" + recent[i] + "</b><br>" +
               recent[i+2] + " - " + recent[i+3] +
              "</a>"
             "</td>"
            "</tr>" );

    browser->write(
         "</table>"
        "</div>" );

    // </Recent Tracks Information>

    browser->write( "</html>" );
    browser->end();
}


void ContextBrowser::showCurrentTrack() //SLOT
{
    #define escapeHTML(s)     QString(s).replace( "<", "&lt;" ).replace( ">", "&gt;" )
    #define escapeHTMLAttr(s) QString(s).replace( "%", "%25" ).replace( "'", "%27" )

    const MetaBundle &currentTrack = EngineController::instance()->bundle();

    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write(
            "<html>"
             "<div class='rbcontent'>"
              "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
               "<tr><th>&nbsp;" + i18n( "Currently Playing" ) + "</th></tr>"
              "</table>"
              "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );


    if ( EngineController::engine()->isStream() ) {
        browser->write( QStringx(
               "<tr>"
                "<td height='42' valign='top' class='rbcurrent' width='90%'>"
                 "<span class='stream'><b>%1</b><br/>%2<br/>%3<br/>%4</span>"
                "</td>"
                "<td valign='top' align='right' width='10%'></td>"
               "</tr>" )
            .args( QStringList()
                << escapeHTML( currentTrack.prettyTitle() )
                << escapeHTML( currentTrack.genre() )
                << escapeHTML( currentTrack.prettyBitrate() )
                << escapeHTML( currentTrack.prettyURL() ) ) );

        browser->write(
              "</table>"
             "</div>"
            "</html>" );

        browser->end();

        return;
    }


    const uint artist_id = m_db->artistID( currentTrack.artist() );
    const uint album_id  = m_db->albumID ( currentTrack.album() );


    QStringList values = m_db->query( QString(
        "SELECT datetime( datetime( statistics.createdate, 'unixepoch' ), 'localtime' ), "
            "datetime( datetime( statistics.accessdate, 'unixepoch' ), 'localtime' ), "
            "statistics.playcounter, round( statistics.percentage + 0.4 ) "
        "FROM  statistics "
        "WHERE url = '%1';" )
            .arg( m_db->escapeString( currentTrack.url().path() ) ) );

    //making 2 tables is most probably not the cleanest way to do it, but it works.
    browser->write( QStringx(
        "<tr>"
         "<td height='42' valign='top' class='rbcurrent' width='90%'>"
          "<span class='album'>"
           "<b>%1 - %2</b>"
          "</span>"
          "<br>%3"
         "</td>"
         "<td valign='top' align='right' width='10%'>"
          "<a title='%4' href='musicbrainz:%5 @@@ %6'>"
           "<img src='%7'>"
          "</a>"
         "</td>"
        "</tr>"
       "</table>"
       "<table width='100%'>"
        "<tr>"
         "<td width='20%'>"
          "<a class='menu' href='fetchcover:%8 @@@ %9'>"
           "<img align='left' hspace='2' src='%11' title='%10'>"
          "</a>"
         "</td>"
         "<td valign='bottom' align='right' width='80%'>" )
        .args( QStringList()
            << escapeHTML( currentTrack.artist() )
            << escapeHTML( currentTrack.title() )
            << escapeHTML( currentTrack.album() )
            << i18n( "Look up this track at musicbrainz.com" )
            << escapeHTMLAttr( m_db->escapeString( currentTrack.artist() ) )
            << escapeHTMLAttr( m_db->escapeString( currentTrack.album() ) )
            << escapeHTML( locate( "data", "amarok/images/musicbrainz.png" ) )
            << escapeHTMLAttr( currentTrack.artist() )
            << escapeHTMLAttr( currentTrack.album() )
            << escapeHTMLAttr( m_db->albumImage( currentTrack.artist(), currentTrack.album() ) )
            << i18n( "Click for information from amazon.com, right-click for menu." ) ) );

    if ( !values.isEmpty() )
        browser->write( QStringx("%1<br>%2<br>%3<br>%4<br>")
            .args( QStringList()
                << i18n( "Track played once", "Track played %n times", values[2].toInt() )
                << i18n( "Score: %1" ).arg( values[3] )
                << i18n( "Last play: %1" ).arg( values[1].left( values[1].length() - 3 ) )
                << i18n( "First play: %1" ).arg( values[0].left( values[0].length() - 3 ) ) ) );
    else
        browser->write( "<i>" + i18n( "Never played before" )  + "</i>" );

    browser->write(
         "</td>"
        "</tr>"
       "</table>"
      "</div>" );

    // </Current Track Information>


    if ( !m_db->isFileInCollection( currentTrack.url().path() ) ) {
        browser->write( "<div class='warning' style='padding: 1em 0.5em 2em 0.5em'>" );
        browser->write( i18n("If you would like to see contextual information about this track, you should add it to your Collection.") );
        browser->write( "&nbsp;<a class='warning' href='show:collectionSetup'>" + i18n( "Click here to change your Collection setup" ) + "</a>.</div>" );
    }


    // scrobblaaaaaar
    if ( m_relatedArtists.isEmpty() )
        m_scrobbler->relatedArtists( currentTrack.artist() );
    else {
        QString token;

        for ( uint i = 0; i < m_relatedArtists.count(); i++ ) {
            if ( i > 0 )
                token += " OR ";
            token += " artist.name = '" + m_db->escapeString( m_relatedArtists[i] ) + "' ";
        }

        values = m_db->query( QString( "SELECT tags.title, tags.url, artist.name "
                                       "FROM tags, artist "
                                       "WHERE tags.artist = artist.id AND ( %1 ) "
                                       "ORDER BY random() "
                                       "LIMIT 0,5;" )
                              .arg( token ) );

        if ( !values.isEmpty() ) {
            browser->write(
                "<br>"
                 "<div class='rbcontent'>"
                  "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                   "<tr><th>" + i18n("Suggested Songs") + "</th></tr>"
                  "</table>"
                  "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

            for ( uint i = 0; i < values.count(); i += 3 )
                browser->write(
                   "<tr>"
                    "<td class='song'>"
                     "<a href='file:" + values[i + 1].replace( '"', QCString("%22") ) + "'>" +
                      values[i + 2] + " - " + values[i] +
                     "</a>"
                    "</td>"
                   "</tr>" );

            browser->write(
                  "</table>"
                 "</div>" );
        }
    }

    // <Favourite Tracks Information>
    values = m_db->query( QString( "SELECT tags.title, tags.url, round( statistics.percentage + 0.4 ) "
                                   "FROM tags, statistics "
                                   "WHERE tags.artist = %1 AND statistics.url = tags.url "
                                   "ORDER BY statistics.percentage DESC "
                                   "LIMIT 0,5;" )
                          .arg( artist_id ) );

    if ( !values.isEmpty() ) {
        browser->write(
            "<br>"
            "<div class='rbcontent'>"
             "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
              "<tr><th>" + i18n( "Favorite Tracks By This Artist" ) + "</th></tr>"
             "</table>"
             "<table width='100%' border='0' cellspacing='1' cellpadding='1'>" );

        for ( uint i = 0; i < values.count(); i += 3 )
            browser->write(
              "<tr>"
               "<td class='song'>"
                "<a href='file:" + values[i + 1].replace( '"', QCString("%22") ) + "'>" +
                 values[i] + " <i>(" + i18n("Score: %1").arg( values[i + 2] ) + ")</i>"
                "</a>"
               "</td>"
              "</tr>" );

        browser->write(
             "</table>"
            "</div>" );
    }
    // </Favourite Tracks Information>

    // <Tracks on this album>
    if ( !currentTrack.album().isEmpty() && !currentTrack.artist().isEmpty() ) {
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
                QString tmp = values[i + 2].stripWhiteSpace().isEmpty() ? "" : values[i + 2] + ". ";
                browser->write( QString ( "<tr><td class='song'><a href=\"file:" + values[i + 1].replace( "\"", QCString( "%22" ) ) + "\">" + tmp + values[i] + "</a></td></tr>" ) );
            }

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
                                   << escapeHTMLAttr( currentTrack.artist() ) // artist name
                                   << escapeHTMLAttr( values[ i ] ) // album.name
                                   << escapeHTMLAttr( m_db->albumImage( currentTrack.artist(), values[ i ], 50 ) )
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
    m_gradientImage = new KTempFile( locateLocal( "tmp", "gradient" ), ".png", 0600 );
    QImage image = KImageEffect::gradient( QSize( 600, 1 ), gradient, gradient.light(), KImageEffect::PipeCrossGradient, 3 );
    image.save( m_gradientImage->file(), "PNG" );
    m_gradientImage->close();

    //we have to set the color for body due to a KHTML bug
    //KHTML sets the base color but not the text color
    m_styleSheet  = QString( "body { margin: 8px; font-size: %1px; color: %2; background-color: %3; background-image: url( %4 ); backgroud-repeat: repeat-y; }" )
                    .arg( pxSize ).arg( text ).arg( AmarokConfig::schemeAmarok() ? fg : gradient.name() )
                    .arg( m_gradientImage->name() );
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

    browser->write( "<html><p>");
    browser->write( i18n( "Building Collection Database.." ) );
    browser->write( "</p></html>");

    browser->end();
}


// THE FOLLOWING CODE IS COPYRIGHT BY
// Christian Muehlhaeuser
// <chris at chris.de>
// If I'm violating any copyright or such
// please contact / sue me. Thanks.

void ContextBrowser::showLyrics() {



    QString url = QString( "http://lyrc.com.ar/en/tema1en.php?artist=%1&songname=%2" )
                  .arg( EngineController::instance()->bundle().artist() )
                  .arg( EngineController::instance()->bundle().title() );

    kdDebug() << "Using this url: " << url << endl;

    m_lyrics = QString::null;

    KIO::TransferJob* job = KIO::get( url, false, false );

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
