// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information


#include "config.h"        //for AMAZON_SUPPORT

#include "amarokconfig.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "colorgenerator.h"
#include "contextbrowser.h"
#include "coverfetcher.h"
#include "covermanager.h"
#include "enginecontroller.h"
#include "metabundle.h"
#include "playlist.h"      //appendMedia()
#include "qstringx.h"

#include <qdatetime.h>
#include <qimage.h>
#include <qregexp.h>

#include <kapplication.h> //kapp
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kiconloader.h>
#include <kimageeffect.h> // gradient backgroud image
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <krun.h>
#include <kstandarddirs.h> //locate file
#include <ktabbar.h>
#include <ktempfile.h>
#include <kurl.h>

#define escapeHTML(s)     QString(s).replace( "&", "&amp;" ).replace( "<", "&lt;" ).replace( ">", "&gt;" )
#define escapeHTMLAttr(s) QString(s).replace( "%", "%25" ).replace( "'", "%27" )


using amaroK::QStringx;


ContextBrowser::ContextBrowser( const char *name )
   : QVBox( 0, name )
   , m_db( new CollectionDB )
   , m_gradientImage( 0 )
{
    EngineController::instance()->attach( this );

    m_tabBar = new KTabBar( this );
    connect( m_tabBar, SIGNAL( selected( int ) ), SLOT( tabChanged( int ) ) );

    m_tabHome    = m_tabBar->addTab( new QTab( SmallIconSet( "gohome" ), i18n( "Home" ) ) );
    m_tabCurrent = m_tabBar->addTab( new QTab( SmallIconSet( "today" ), i18n( "Current Track" ) ) );
    m_tabLyrics  = m_tabBar->addTab( new QTab( SmallIconSet( "document" ), i18n( "Lyrics" ) ) );

    browser = new KHTMLPart( this );
    browser->setDNDEnabled( true );

    setMargin( 5 );
    setFocusProxy( browser->view() ); //so focus is given to a sensible widget when the tab is opened

    connect( browser->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                          SLOT( openURLRequest( const KURL & ) ) );
    connect( browser,                     SIGNAL( popupMenu( const QString&, const QPoint& ) ),
             this,                          SLOT( slotContextMenu( const QString&, const QPoint& ) ) );

    connect( CollectionDB::emitter(), SIGNAL( scanStarted() ), SLOT( collectionScanStarted() ) );
    connect( CollectionDB::emitter(), SIGNAL( scanDone( bool ) ), SLOT( collectionScanDone() ) );
    connect( CollectionDB::emitter(), SIGNAL( coverFetched( const QString& ) ), SLOT( coverFetched( const QString& ) ) );

    //the stylesheet will be set up and home will be shown later due to engine signals and doodaa
    //if we call it here setStyleSheet is called 3 times during startup!!
}


ContextBrowser::~ContextBrowser()
{
    delete m_db;

    if( m_gradientImage )
      m_gradientImage->unlink();

    EngineController::instance()->detach( this );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::setFont( const QFont &newFont )
{
//    if( newFont != font() ) {
        QWidget::setFont( newFont );
        setStyleSheet();
//    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::openURLRequest( const KURL &url )
{
    QStringList info = QStringList::split( " @@@ ", url.path() );

    if ( url.protocol() == "album" )
    {
        QString sql = "SELECT DISTINCT url FROM tags WHERE artist = %1 AND album = %2 ORDER BY track;";
        QStringList values = m_db->query( sql.arg( info[0] ).arg( info[1] ) );
        KURL::List urls;
        KURL url;

        for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it )
        {
            url.setPath( *it );
            urls.append( url );
        }

        Playlist::instance()->insertMedia( urls, Playlist::Unique );

        return;
    }

    if ( url.protocol() == "file" )
        Playlist::instance()->insertMedia( url, Playlist::DirectPlay | Playlist::Unique );

    if ( url.protocol() == "show" )
    {
        if ( url.path() == "home" )
            showHome();
        else if ( url.path() == "context" || url.path() == "stream" )
            showCurrentTrack();
        else if ( url.path().contains( "suggestLyric-" ) )
        {
            m_lyrics = QString::null;
            QString hash = url.path().mid( url.path().find( QString( "-" ) ) +1 );
            showLyrics( hash );
        }
        else if ( url.path() == "lyrics" )
            showLyrics();
        else if ( url.path() == "collectionSetup" )
        {
            //TODO if we do move the configuration to the main configdialog change this,
            //     otherwise we need a better solution
            QObject *o = parent()->child( "CollectionBrowser" );
            if ( o )
                static_cast<CollectionBrowser*>( o )->setupDirs();
        }
    }

    // When left-clicking on cover image, open browser with amazon site
    if ( url.protocol() == "fetchcover" )
    {
        QStringList info = QStringList::split( " @@@ ", url.path() );
        QImage img( m_db->albumImage( info[0], info[1], 0 ) );
        const QString amazonUrl = img.text( "amazon-url" );
        kdDebug() << "[ContextBrowser] Embedded amazon url in cover image: " << amazonUrl << endl;

        if ( amazonUrl.isEmpty() )
            KMessageBox::information( this, i18n( "<p>There is no product information available for this image.</p><p>Right-click on image for menu.</p>" ) );
        else
            kapp->invokeBrowser( amazonUrl );
    }

    /* open konqueror with musicbrainz search result for artist-album */
    if ( url.protocol() == "musicbrainz" )
    {
        const QString url = "http://www.musicbrainz.org/taglookup.html?artist='%1'&album='%2'";
        kapp->invokeBrowser( url.arg( info[0], info[1] ) );
    }
}


void ContextBrowser::collectionScanStarted()
{
    if( m_emptyDB )
       showScanning();
}


void ContextBrowser::collectionScanDone()
{
    // take care of sql updates (schema changed errors)
    delete m_db;
    m_db = new CollectionDB();

    if ( CollectionDB().isEmpty() )
    {
        showIntroduction();
        m_emptyDB = true;
    } else if ( m_emptyDB )
    {
        showHome();
        m_emptyDB = false;
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::engineNewMetaData( const MetaBundle& bundle, bool /*trackChanged*/ )
{
    // Add stream metadata history item to list
    if ( !m_metadataHistory.last().contains( bundle.prettyTitle() ) )
    {
        const QString timeString = QTime::currentTime().toString( "hh:mm" );
        m_metadataHistory << QString( "<td valign='top'>" + timeString + "&nbsp;</td><td align='left'>" + escapeHTML( bundle.prettyTitle() ) + "</td>" );
    }

    switch( m_db->isEmpty() || !m_db->isValid() )
    {
        case true:  showIntroduction();
        case false: showCurrentTrack();
    }
}


void ContextBrowser::engineStateChanged( Engine::State state )
{
    switch( state )
    {
        case Engine::Empty:
            m_metadataHistory.clear();
            m_tabBar->blockSignals( true );
            m_tabBar->setTabEnabled( m_tabCurrent, false );
            m_tabBar->setTabEnabled( m_tabLyrics, false );
            m_tabBar->blockSignals( false );
            showHome();
            break;
        case Engine::Playing:
            m_metadataHistory.clear();
            m_tabBar->blockSignals( true );
            m_tabBar->setTabEnabled( m_tabCurrent, true );
            m_tabBar->setTabEnabled( m_tabLyrics, true );
            m_tabBar->blockSignals( false );
            break;
        default:
            ;
    }
}


void ContextBrowser::paletteChange( const QPalette& pal )
{
    QVBox::paletteChange( pal );
    setStyleSheet();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void ContextBrowser::tabChanged( int id )
{
    if ( id == m_tabHome )
        showHome();

    if ( id == m_tabCurrent )
        showCurrentTrack();

    if ( id == m_tabLyrics )
        showLyrics();
}


void ContextBrowser::slotContextMenu( const QString& urlString, const QPoint& point )
{
    enum { SHOW, FETCH, CUSTOM, DELETE, APPEND, ASNEXT, MAKE, MANAGER, TITLE };

    if( urlString.isEmpty() || urlString.startsWith( "musicbrainz" ) )
       return;

    KURL url( urlString );
    if( url.path().contains( "lyric", FALSE ) )
        return;

    KPopupMenu menu;
    KURL::List urls( url );
    const QStringList info = QStringList::split( " @@@ ", url.path() );

    if ( url.protocol() == "fetchcover" )
    {
        menu.insertTitle( i18n( "Cover Image" ) );

        menu.insertItem( SmallIconSet( "viewmag" ), i18n( "&Show Fullsize" ), SHOW );
        menu.insertItem( SmallIconSet( "www" ), i18n( "&Fetch From amazon.%1" ).arg(AmarokConfig::amazonLocale()), FETCH );
        menu.insertItem( SmallIconSet( "folder_image" ), i18n( "Set &Custom Cover" ), CUSTOM );
        menu.insertSeparator();

        menu.insertItem( SmallIconSet( "editdelete" ), i18n("&Unset Cover"), DELETE );
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

        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue After Current Track" ), ASNEXT );

        if ( url.protocol() == "album" )
        {
            QString sql = "select distinct url from tags where artist = '%1' and album = '%2' order by track;";
            QStringList values = m_db->query( sql.arg( info[0] ).arg( info[1] ) );

            urls.clear(); //remove urlString
            KURL url;
            for( QStringList::ConstIterator it = values.begin(); it != values.end(); ++it )
            {
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
            i18n( "Are you sure you want to remove this cover from the Collection?" ),
            QString::null,
            i18n("&Remove") );

        if ( button == KMessageBox::Continue )
        {
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
        if ( m_db->getImageForAlbum( info[0], info[1], 0 ) == locate( "data", "amarok/images/nocover.png" ) )
        {
            /* if no cover exists, open a file dialog to add a cover */
            KURL file = KFileDialog::getImageOpenURL( ":homedir", this, i18n( "Select Cover Image File" ) );
            if ( !file.isEmpty() )
            {
                QImage img( file.directory() + '/' + file.fileName() );
                QString filename( QFile::encodeName( info[0] + " - " + info[1] ) );
                filename.replace( ' ', '_' ).append( ".png" );
                img.save( KGlobal::dirs()->saveLocation( "data", "amarok/albumcovers/"+filename.lower(), "PNG" );
                showCurrentTrack();
            }
        }
        else
            CoverManager::viewCover( info[0], info[1], this );
        break;
    #endif

    case CUSTOM:
    {
        /* This opens a file-open-dialog and copies the selected image to albumcovers, scaled and unscaled. */
        KURL file = KFileDialog::getImageOpenURL( ":homedir", this, i18n( "Select Cover Image File" ) );
        if ( !file.isEmpty() )
        {
            qApp->processEvents();    //it may takes a while so process pending events
            m_db->setAlbumImage( info[0], info[1], file );
            showCurrentTrack();
        }
        break;
    }

    case MANAGER:
        CoverManager::showOnce();
        break;
    }
}

void ContextBrowser::showHome() //SLOT
{
    m_tabBar->blockSignals( true );
    m_tabBar->setCurrentTab( m_tabHome );
    m_tabBar->blockSignals( false );

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
              "<a href=\"file:" + fave[i+1].replace( '"', QCString( "%22" ) ) + "\">"
               "<b>" + fave[i] + "</b> "
               "(" + i18n( "Score: %1" ).arg( fave[i+2] ) + ")<br>" +
               fave[i+3] + " - " + fave[i+4] +
              "</a>"
             "</td>"
/*           "<td class='sbtext' width='1'><i>" + fave[i+2] + "</i></td>"
             "<td width='1'>"
              "<div class='sbouter'>"
               "<div class='sbinner' style='width: " + QString::number( fave[i+2].toInt() / 2 ) + "px;'></div>"
              "</div>"
             "</td>"
*/          "</tr>" );

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
              "<a href=\"file:" + recent[i+1].replace( '"', QCString( "%22" ) ) + "\">"
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
    m_tabBar->blockSignals( true );
    m_tabBar->setCurrentTab( m_tabCurrent );
    m_tabBar->blockSignals( false );

    const MetaBundle &currentTrack = EngineController::instance()->bundle();

    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html>" );

    if ( EngineController::engine()->isStream() )
    {
        browser->write( QStringx(
             "<div class='rbcontent'>"
              "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
               "<tr><th>&nbsp;%1</th></tr>"
              "</table>"
              "<table width='100%' border='0' cellspacing='0' cellpadding='1'>"
               "<tr>"
                "<td height='42' valign='top' width='90%'>"
                 "<span class='stream'>"
                  "<b>%2</b>"
                  "<br/>"
                  "<br/>"
                  "%3"
                  "<br/>"
                  "<br/>"
                  "%4"
                  "<br/>"
                  "%5"
                  "<br/>"
                  "%6"
                  "<br/>"
                  "%7"
                 "</span>"
                "</td>"
               "</tr>"
              "</table>"
             "</div>" )
            .args( QStringList()
                << i18n( "Stream Details" )
                << escapeHTML( currentTrack.prettyTitle() )
                << escapeHTML( currentTrack.streamName() )
                << escapeHTML( currentTrack.genre() )
                << escapeHTML( currentTrack.prettyBitrate() )
                << escapeHTML( currentTrack.streamUrl() )
                << escapeHTML( currentTrack.prettyURL() ) ) );

        if ( m_metadataHistory.count() > 2 )
        {
            browser->write(
                    "<br/>"
                    "<div class='rbcontent'>"
                     "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                      "<tr><th>&nbsp;" + i18n( "Metadata History" ) + "</th></tr>"
                     "</table>"
                     "<table width='100%' border='0' cellspacing='0' cellpadding='1'>" );

            QStringList::const_iterator it;
            // Ignore first two items, as they don't belong in the history
            for ( it = m_metadataHistory.at( 2 ); it != m_metadataHistory.end(); ++it )
            {
                browser->write( QStringx(
                    "<tr>"
                        "%1"
                    "</tr>" )
                    .arg( *it ) );
            }

            browser->write(
                 "</table>"
                "</div>" );
        }

        browser->write("</html>" );
        browser->end();

        return;
    }

    const uint artist_id = m_db->artistID( currentTrack.artist() );
    const uint album_id  = m_db->albumID ( currentTrack.album() );

    // <Current Track Information>
    QStringList values = m_db->query( QString(
        "SELECT statistics.createdate, statistics.accessdate, "
        "statistics.playcounter, round( statistics.percentage + 0.4 ) "
        "FROM  statistics "
        "WHERE url = '%1';" )
            .arg( m_db->escapeString( currentTrack.url().path() ) ) );

    //making 2 tables is most probably not the cleanest way to do it, but it works.
    browser->write( QStringx(
        "<div class='rbcontent'>"
         "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
          "<tr>"
           "<th style='font-weight: normal;'>"
            "<a title='%1' href='musicbrainz:%2 @@@ %3'>"
             "<img src='%4' style='float: right; width: 38px; height: 22px;' />"
            "</a>"
            "<b>%5</b> - <b>%6</b>"
            "<br/>"
            "%7"
           "</th>"
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
            << i18n( "Look up this track at musicbrainz.com" )
            << escapeHTMLAttr( m_db->escapeString( currentTrack.artist() ) )
            << escapeHTMLAttr( m_db->escapeString( currentTrack.album() ) )
            << escapeHTML( locate( "data", "amarok/images/musicbrainz.png" ) )
            << escapeHTML( currentTrack.title() )
            << escapeHTML( currentTrack.artist() )
            << escapeHTML( currentTrack.album() )
            << escapeHTMLAttr( currentTrack.artist() )
            << escapeHTMLAttr( currentTrack.album() )
            << escapeHTMLAttr( m_db->albumImage( currentTrack.artist(), currentTrack.album() ) )
            << i18n( "Click for information from amazon.%1, right-click for menu." ).arg( AmarokConfig::amazonLocale() ) ) );

    if ( !values.isEmpty() )
    {
        QDateTime firstPlay = QDateTime();
        firstPlay.setTime_t( values[0].toUInt() );
        QDateTime lastPlay = QDateTime();
        lastPlay.setTime_t( values[1].toUInt() );

        const uint playtimes = values[2].toInt();
        const uint score = values[3].toInt();

        QString scoreBox = "<table><tbody><tr><td>"+ QString::number( score ) +"</td><td title='Score'><div class='sbouter'><div class='sbinner' style='width: "+ QString::number( score / 2 )+ "px;'></div></div></td></tr></tbody></table>";

        browser->write( QStringx("%1<br>%2%3<br>%4<br>")
            .args( QStringList()
                << i18n( "Track played once", "Track played %n times", playtimes )
                << scoreBox
                << i18n( "Last play: %1" ).arg( KGlobal::locale()->formatDateTime( lastPlay, true /* short */ ) )
                << i18n( "First play: %1" ).arg( KGlobal::locale()->formatDateTime( firstPlay, true /* short */ ) )
                       ) );
   }
   else
        browser->write( "" + i18n( "Never played before" )  + "" );

    browser->write(
           "</td>"
          "</tr>"
         "</table>"
        "</div>" );
    // </Current Track Information>

    if ( !m_db->isFileInCollection( currentTrack.url().path() ) )
    {
        browser->write( "<div class='warning' style='padding: 1em 0.5em 2em 0.5em'>" );
        browser->write( i18n("If you would like to see contextual information about this track, you should add it to your Collection.") );
        browser->write( "&nbsp;<a class='warning' href='show:collectionSetup'>" + i18n( "Click here to change your Collection setup" ) + "</a>.</div>" );
    }

    // <Suggested Songs>
    QStringList relArtists;
    relArtists = m_db->relatedArtists( currentTrack.artist(), 10 );
    if ( !relArtists.isEmpty() )
    {
        QString token;
        QueryBuilder qb;

        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addMatches( QueryBuilder::tabArtist, relArtists );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valScore, true );
        qb.setLimit( 0, 5 );
        values = qb.run();

        // not enough items returned, let's fill the list with score-less tracks
        if ( values.count() < 5 * qb.countReturnValues() )
        {
            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
            qb.addMatches( QueryBuilder::tabArtist, relArtists );
            qb.setOptions( QueryBuilder::optRandomize );
            qb.setLimit( 0, 5 - values.count() / 4 );

            QStringList sl;
            sl = qb.run();
            for ( uint i = 0; i < sl.count(); i += qb.countReturnValues() )
            {
                values << sl[i];
                values << sl[i + 1];
                values << sl[i + 2];
                values << "0";
            }
        }

        if ( !values.isEmpty() )
        {
            browser->write(
                "<br>"
                 "<div class='rbcontent'>"
                  "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                   "<tr><th>" + i18n( "Suggested Songs" ) + "</th></tr>"
                  "</table>"
                  "<table width='100%' border='0' cellspacing='0' cellpadding='1'>" );


            for ( uint i = 0; i < values.count(); i += 4 )
                browser->write(
                  "<tr>"
                  "<td class='song'>"
                    "<a href=\"file:" + values[i].replace( '"', QCString( "%22" ) ) + "\">" + values[i + 2] + " - " + values[i + 1] +
                    "</a>"
                  "</td>"
                  "<td class='sbtext' width='1'>" + values[i + 3] + "</td>"
                  "<td width='1' title='Score'>"
                    "<div class='sbouter'>"
                    "<div class='sbinner' style='width: " + QString::number( values[i + 3].toInt() / 2 ) + "px;'></div>"
                    "</div>"
                  "</td>"
                  "</tr>" );

            browser->write(
                "</table>"
                "</div>" );
        }
    }
    // </Suggested Songs>

    // <Favourite Tracks Information>
    QString artistName = currentTrack.artist().isEmpty() ? i18n( "This Artist" ) : escapeHTML( currentTrack.artist() );
    values = m_db->query( QString( "SELECT tags.title, tags.url, round( statistics.percentage + 0.4 ) "
                                   "FROM tags, statistics "
                                   "WHERE tags.artist = %1 AND statistics.url = tags.url "
                                   "ORDER BY statistics.percentage DESC "
                                   "LIMIT 0,5;" )
                          .arg( artist_id ) );

    if ( !values.isEmpty() )
    {
        browser->write(
            "<br>"
            "<div class='rbcontent'>"
             "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                "<tr><th>" + i18n( "Favorite Tracks By %1" ).arg( artistName ) + "</th></tr>"
             "</table>"
             "<table width='100%' border='0' cellspacing='0' cellpadding='1'>" );

        for ( uint i = 0; i < values.count(); i += 3 )
            browser->write(
              "<tr>"
               "<td class='song'>"
                "<a href=\"file:" + values[i + 1].replace( '"', QCString( "%22" ) ) + "\">" + values[i] +
                "</a>"
               "</td>"
               "<td class='sbtext' width='1'>" + values[i + 2] + "</td>"
               "<td width='1' title='Score'>"
                "<div class='sbouter'>"
                 "<div class='sbinner' style='width: " + QString::number( values[i + 2].toInt() / 2 ) + "px;'></div>"
                "</div>"
               "</td>"
              "</tr>" );

        browser->write(
             "</table>"
            "</div>" );
    }
    // </Favourite Tracks Information>

    // <Albums by this artist>
    values = m_db->query( QString( "SELECT DISTINCT album.name, album.id "
                                   "FROM tags, album "
                                   "WHERE album.id = tags.album AND tags.artist = %1 AND album.name <> '' "
                                   "ORDER BY album.name;" )
                          .arg( artist_id ) );

    if ( !values.isEmpty() )
    {
        // write the script to toggle albumlists and header
        browser->write(
            "<script type='text/javascript'>"
              "function toggleAlbumTracks(album) {"
                "if (document.getElementById(album).style.display != 'none') {"
                  "document.getElementById(album).style.display = 'none';"
                "} else {"
                  "document.getElementById(album).style.display = 'block';"
                "}"
              "}"
            "</script>"
            "<br>"
            "<div class='rbcontent'>"
             "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
              "<tr><th>" + i18n( "Albums By %1" ).arg( artistName ) + "</th></tr>"
             "</table>"
             "<table width='100%' border='0' cellspacing='0' cellpadding='0'>" );

        // place current album first
        uint vectorPlace = 0;
        while ( vectorPlace < values.count() && values[ vectorPlace+1 ] != QString::number( album_id ) )
            vectorPlace += 2;
        // if album found, swap that entry with the first one
        if ( vectorPlace != 0 && vectorPlace < values.count() )
        {
            QString tmp = values[ vectorPlace ];
            values[ vectorPlace ] = values[ 0 ];
            values[ 0 ] = tmp;
            tmp = values[ vectorPlace+1 ];
            values[ vectorPlace+1 ] = values[ 1 ];
            values[ 1 ] = tmp;
        }

        for ( uint i = 0; i < values.count(); i += 2 )
        {
            QStringList albumValues = m_db->query( QString(
                "SELECT tags.title, tags.url, tags.track, year.name "
                "FROM tags, year "
                "WHERE tags.album = %1 AND "
                "( tags.sampler = 1 OR tags.artist = %2 ) "
                "AND year.id = tags.year "
                "ORDER BY tags.track;" ).arg( values[ i+1 ] ).arg( artist_id ) );

            QString albumYear;
            if ( !albumValues.isEmpty() )
            {
                albumYear = albumValues[ 3 ];
                for ( uint j = 0; j < albumValues.count(); j += 4 )
                {
                    if ( albumValues[j + 3] != albumYear )
                    {
                        albumYear = QString::null;
                        break;
                    }
                }
            }

            browser->write( QStringx (
                "<tr>"
                 "<td>"
                  "<div class='album-header' onClick=\"toggleAlbumTracks('IDA%1')\">"
                   "<table width='100%' border='0' cellspacing='0' cellpadding='0'>"
                    "<tr><td>"
                     "<a href='fetchcover:%2 @@@ %3'><img width='50' align='left' vspace='2' hspace='2' title='%4' src='%5'/></a></div>"
                     "<div style='float:right;'>%6</div>"
                     "<a href='album:%7 @@@ %8'><b>%9</b></a>"
                     "<br><i>%10</i>"
                    "</td></tr>"
                   "</table>"
                  "</div>"
                  "<div class='album-contents' style='display:%11;' id='IDA%12'>" )
                .args( QStringList()
                    << values[ i+1 ]
                    << escapeHTMLAttr( currentTrack.artist() ) // artist name
                    << escapeHTMLAttr( values[ i ] ) // album.name
                    << i18n( "Click for information from amazon.com, right-click for menu." )
                    << escapeHTMLAttr( m_db->albumImage( currentTrack.artist(), values[ i ], 50 ) )
                    << i18n( "Single", "%n Tracks",  albumValues.count() / 4 )
                    << QString::number( artist_id )
                    << values[ i+1 ] //album.id
                    << escapeHTML( values[ i ] )
                    << albumYear
                    << ( i ? "none" : "block" )
                    << values[ i+1 ] ) );

            if ( !albumValues.isEmpty() )
                for ( uint j = 0; j < albumValues.count(); j += 4 )
                {
                    QString tmp = albumValues[j + 2].stripWhiteSpace().isEmpty() ? "" : "" + albumValues[j + 2] + ". ";
                    browser->write(
                        "<div class='song'>"
                         "<a href=\"file:" + albumValues[j + 1].replace( "\"", QCString( "%22" ) ) + "\">" + tmp + albumValues[j] + "</a>"
                        "</div>" );
                }

            browser->write(
                  "</div>"
                 "</td>"
                "</tr>" );
        }
        browser->write(
               "</table>"
              "</div>" );
    }
    // </Albums by this artist>

    browser->write( "</html>" );
    browser->end();
}


void ContextBrowser::setStyleSheet()
{
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

    //this is the style for the other blocks
    m_styleSheet += QString( ".rbcontent { border: solid %1 1px; }" ).arg( bg );
    m_styleSheet += QString( ".rbcontent:hover { border: solid %1 1px; }" ).arg( text );

    //these anchor rules are a little complex
    m_styleSheet += QString( ".rbcontent a { text-decoration: none; }" );
    m_styleSheet += QString( ".rbcontent .song a { display: block; padding: 1px 2px; }" );
    m_styleSheet += QString( ".rbcontent .song a:hover { color: %1; background-color: %1; }" ).arg( fg ).arg( bg );
    m_styleSheet += QString( ".album-header { background-color: %1; }" ).arg( colorGroup().base().name() );
    m_styleSheet += QString( ".album-header:hover { background-color: %1; cursor: pointer; }" ).arg( colorGroup().highlight().light( 120 ).name() );
    m_styleSheet += QString( ".album-contents { background-color: %1; border-bottom: solid %2 1px; border-top: solid %3 1px; }" ).arg( colorGroup().base().name() ).arg( bg ).arg( bg );

    //boxes used to display score (sb: score box)
    m_styleSheet += QString( ".sbtext { padding: 0px 4px; border-left: solid %1 1px; }" ).arg( colorGroup().base().dark( 120 ).name() );
    m_styleSheet += QString( ".sbouter { width: 52px; height: 10px; background-color: #E0E0E0; border: solid #808080 1px; }" );
    m_styleSheet += QString( ".sbinner { height: 8px; background-color: %1; border: solid %2 1px; }" ).arg( bg ).arg( fg );

    browser->setUserStyleSheet( m_styleSheet );
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

    browser->write( "<html><p>");
    browser->write( i18n( "Building Collection Database.." ) );
    browser->write( "</p></html>");

    browser->end();
}


// THE FOLLOWING CODE IS COPYRIGHT BY
// Christian Muehlhaeuser, Seb Ruiz
// <chris at chris.de>, <seb100 at optusnet.com.au>
// If I'm violating any copyright or such
// please contact / sue me. Thanks.

void ContextBrowser::showLyrics( const QString &hash )
{
    //remove all matches to the regExp and the song production type.
    //NOTE: use i18n'd and english equivalents since they are very common int'lly.
    QString replaceMe = " \\([^}]*%1[^}]*\\)";
    QStringList production;
    production << i18n( "live" ) << i18n( "acoustic" ) << i18n( "cover" ) << i18n( "mix" )
               << i18n( "edit" ) << i18n( "medley" ) << i18n( "unplugged" ) << i18n( "bonus" )
               << QString( "live" ) << QString( "acoustic" ) << QString( "cover" ) << QString( "mix" )
               << QString( "edit" ) << QString( "medley" ) << QString( "unplugged" ) << QString( "bonus" );

    QString title  = EngineController::instance()->bundle().title();

    for ( uint x = 0; x < production.count(); ++x )
    {
        QRegExp re = replaceMe.arg( production[x] );
        re.setCaseSensitive( false );
        title.remove( re );
    }

    QString url;
    if ( !hash.isEmpty() )
        url = QString( "http://lyrc.com.ar/en/tema1en.php?hash=%1" )
                  .arg( hash );
    else
        url = QString( "http://lyrc.com.ar/en/tema1en.php?artist=%1&songname=%2" )
                  .arg( EngineController::instance()->bundle().artist() )
                  .arg( title );


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
    if ( m_tabBar->keyboardFocusTab() == m_tabLyrics )
        return;

    if ( !job->error() == 0 )
    {
        kdWarning() << "[LyricsFetcher] KIO error! errno: " << job->error() << endl;
        return;
    }

    if ( m_lyrics.find( "<font size='2'>" ) != -1 )
    {
        m_lyrics = m_lyrics.mid( m_lyrics.find( "<font size='2'>" ) );
        if ( m_lyrics.find( "<p><hr" ) )
            m_lyrics = m_lyrics.mid( 0, m_lyrics.find( "<p><hr" ) );
        else
            m_lyrics = m_lyrics.mid( 0, m_lyrics.find( "<br><br>" ) );
    }
    else if ( m_lyrics.find( "Suggestions : " ) != -1 )
    {
        m_lyrics = m_lyrics.mid( m_lyrics.find( "Suggestions : " ), m_lyrics.find( "<br><br>" ) );
        showLyricSuggestions();
    }
    else
        m_lyrics = i18n( "Lyrics not found." );

    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    browser->write( "<html><div>" );
    browser->write( m_lyrics );
    browser->write( "</div></html>" );
    browser->end();
}


void
ContextBrowser::showLyricSuggestions()
{
    m_lyricHashes.clear();
    m_lyricSuggestions.clear();

    m_lyrics.replace( QString( "<font color='white'>" ), QString::null );
    m_lyrics.replace( QString( "</font>" ), QString::null );
    m_lyrics.replace( QString( "<br><br>" ), QString::null );

    while ( !m_lyrics.isEmpty() )
    {
        m_lyrics = m_lyrics.mid( m_lyrics.find( "hash=" ) );
        m_lyricHashes << m_lyrics.mid( 5, m_lyrics.find( ">" ) - 6 );
        m_lyrics = m_lyrics.mid( m_lyrics.find( ">" ) );
        m_lyricSuggestions << m_lyrics.mid( 1, m_lyrics.find( "</a>" ) - 1 );
    }
    m_lyrics = QString( "Lyrics for track not found, here are some suggestions:<br><br>" );

    for ( uint i=0; i < m_lyricHashes.count() - 1; ++i )
    {
        m_lyrics += QString( "<a href='show:suggestLyric-%1'>").arg( m_lyricHashes[i] );
        m_lyrics += QString( "%1</a><br>" ).arg( m_lyricSuggestions[i] );
    }

}


void
ContextBrowser::coverFetched( const QString& keyword )
{
    const MetaBundle &currentTrack = EngineController::instance()->bundle();
    QStringList values = QStringList::split( " - ", keyword );

    if ( currentTrack.artist() == values[0] )
        showCurrentTrack();
}


#include "contextbrowser.moc"
