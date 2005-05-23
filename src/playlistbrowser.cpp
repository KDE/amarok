// (c) 2004 Pierpaolo Di Panfilo
// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// License: GPL V2. See COPYING file for information.

#define DEBUG_PREFIX "PlaylistBrowser"

#include "amarok.h"            //actionCollection()
#include "browserToolBar.h"
#include "collectiondb.h"      //smart playlists
#include "collectionreader.h"
#include "debug.h"
#include "k3bexporter.h"
#include "party.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistbrowseritem.h"
#include "smartplaylisteditor.h"
#include "statusbar.h"         //For notifications, TODO:Remove me once all completed.
#include "tagdialog.h"         //showContextMenu()
#include "threadweaver.h"

#include <qevent.h>            //customEvent()
#include <qheader.h>           //mousePressed()
#include <qpainter.h>          //paintCell()
#include <qpixmap.h>           //paintCell()
#include <qsplitter.h>
#include <qtextstream.h>       //loadPlaylists(), saveM3U(), savePLS()
#include <qtimer.h>            //loading animation

#include <kaction.h>
#include <kactionclasses.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kfiledialog.h>       //openPlaylist()
#include <kio/job.h>           //deleteSelectedPlaylists()
#include <kiconloader.h>       //smallIcon
#include <klineedit.h>         //rename()
#include <klocale.h>
#include <kmessagebox.h>       //renamePlaylist(), deleteSelectedPlaylist()
#include <kpopupmenu.h>
#include <kstandarddirs.h>     //KGlobal::dirs()
#include <kurldrag.h>          //dragObject()

#include <stdio.h>             //rename() in renamePlaylist()

#define escapeHTML(s)     QString(s).replace( "&", "&amp;" ).replace( "<", "&lt;" ).replace( ">", "&gt;" )

PlaylistBrowser *PlaylistBrowser::s_instance = 0;

PlaylistBrowser::PlaylistBrowser( const char *name )
        : QVBox( 0, name )
        , m_lastPlaylist( 0 )
{
    s_instance = this;

    m_splitter = new QSplitter( Vertical, this );

    QVBox *browserBox = new QVBox( m_splitter );
    browserBox->setSpacing( 3 );

    //<Toolbar>
    m_ac = new KActionCollection( this );

    addMenuButton  = new KActionMenu( i18n("Add"), "fileopen", m_ac );
    addMenuButton->setDelayed( false );

    KPopupMenu *addMenu  = addMenuButton->popupMenu();
    addMenu->insertItem( i18n("Playlist"), PLAYLIST );
    addMenu->insertItem( i18n("Stream"), STREAM );
    addMenu->insertItem( i18n("Smart Playlist"), SMARTPLAYLIST );
    connect( addMenu, SIGNAL( activated(int) ), SLOT( slotAddMenu(int) ) );

    saveMenuButton = new KActionMenu( i18n("Save"), "filesave", m_ac );
    saveMenuButton->setDelayed( false );

    KPopupMenu *saveMenu = saveMenuButton->popupMenu();
    saveMenu->insertItem( i18n("Current Playlist"), CURRENT );
    saveMenu->insertItem( i18n("Current Party"), PARTY );
    connect( saveMenu, SIGNAL( activated(int) ), SLOT( slotSaveMenu(int) ) );

    renameButton   = new KAction( i18n("Rename"), "editclear", 0, this, SLOT( renameSelectedItem() ), m_ac, "Rename" );
    removeButton   = new KAction( i18n("Remove"), "edittrash", 0, this, SLOT( removeSelectedItems() ), m_ac, "Remove" );
    deleteButton   = new KAction( i18n("Delete"), "editdelete", 0, this, SLOT( deleteSelectedPlaylists() ), m_ac, "Delete" );

    viewMenuButton = new KActionMenu( i18n("View"), "configure", m_ac );
    viewMenuButton->setDelayed( false );

    KPopupMenu *viewMenu = viewMenuButton->popupMenu();

    viewMenu->setCheckable( true );
    viewMenu->insertItem( i18n("Detailed View"), DETAILEDVIEW );
    viewMenu->insertItem( i18n("List View"), LISTVIEW );
    viewMenu->insertSeparator();
    viewMenu->insertItem( i18n("Unsorted"), UNSORTED );
    viewMenu->insertItem( i18n("Sort Ascending"), ASCENDING );
    viewMenu->insertItem( i18n("Sort Descending"), DESCENDING );
    viewMenu->setItemChecked( UNSORTED, true );
    connect( viewMenu, SIGNAL( activated(int) ), SLOT( slotViewMenu(int) ) );

    m_toolbar = new Browser::ToolBar( browserBox );
    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want the open button to have text on right
    addMenuButton->plug( m_toolbar );

    m_toolbar->insertLineSeparator();
    saveMenuButton->plug( m_toolbar );

    m_toolbar->insertLineSeparator();
    m_toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance
    renameButton->plug( m_toolbar);
    removeButton->plug( m_toolbar );
    deleteButton->plug( m_toolbar);
    m_toolbar->insertLineSeparator();
    viewMenuButton->plug( m_toolbar );

    renameButton->setEnabled( false );
    removeButton->setEnabled( false );
    deleteButton->setEnabled( false );
    //</Toolbar>

    m_listview = new PlaylistBrowserView( browserBox );
    new Party( m_splitter );

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistBrowser" );
    m_viewMode = (ViewMode)config->readNumEntry( "View", DETAILEDVIEW );  //restore the view mode
    viewMenu->setItemChecked( m_viewMode, true );
    m_sortMode = config->readNumEntry( "Sorting", ASCENDING );
    slotViewMenu( m_sortMode );
    QString str = config->readEntry( "Splitter", "[228,121]" );    //default splitter position
    QTextStream stream( &str, IO_ReadOnly );
    stream >> *m_splitter;     //this sets the splitters position

    // signals and slots connections
    connect( m_listview, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
             this,         SLOT( showContextMenu( QListViewItem *, const QPoint &, int ) ) );
    connect( m_listview, SIGNAL( doubleClicked( QListViewItem *) ),
             this,         SLOT( slotDoubleClicked( QListViewItem * ) ) );
    connect( m_listview, SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
             this,         SLOT( renamePlaylist( QListViewItem*, const QString&, int ) ) );
    connect( m_listview, SIGNAL( currentChanged( QListViewItem * ) ),
             this,         SLOT( currentItemChanged( QListViewItem * ) ) );

    setMinimumWidth( m_toolbar->sizeHint().width() );

    m_playlistCategory = new PlaylistCategory( m_listview, 0, i18n( "Playlists" ) );
    m_streamsCategory  = new PlaylistCategory( m_listview, m_playlistCategory, i18n( "Streams" ) );
    m_smartCategory    = new PlaylistCategory( m_listview, m_streamsCategory,  i18n( "Smart Playlists" ) );
    m_partyCategory    = new PlaylistCategory( m_listview, m_smartCategory,    i18n( "Parties" ) );

    loadPlaylists();
    loadStreams();
    loadSmartPlaylists();
    loadParties();
}


PlaylistBrowser::~PlaylistBrowser()
{
    savePlaylists();
    saveStreams();
    saveSmartPlaylists();
    saveParties();

    KConfig *config = kapp->config();

    config->setGroup( "PlaylistBrowser" );
    config->writeEntry( "View", m_viewMode );
    config->writeEntry( "Sorting", m_sortMode );

    QString str;

    QTextStream stream( &str, IO_WriteOnly );
    stream << *m_splitter;
    config->writeEntry( "Splitter", str );
}

/**
 *************************************************************************
 *  STREAMS
 *************************************************************************
 **/

QString PlaylistBrowser::streamBrowserCache()
{
    return amaroK::saveLocation() + "streambrowser_save.xml";
}

void PlaylistBrowser::loadStreams()
{
    QFile file( streamBrowserCache() );

    if( !file.open( IO_ReadOnly ) )
    {
        loadCoolStreams();
        m_streamsCategory->setOpen( true );
        return;
    }

    m_lastStream = 0;
    loadCoolStreams();

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;

    if( !d.setContent( stream.read() ) )
        return;

    const QString STREAM( "stream" );

    QDomNode n = d.namedItem( "streambrowser" ).namedItem("stream");

    for( ; !n.isNull(); n = n.nextSibling() )
    {
        if( n.nodeName() != STREAM ) continue;

        QDomElement e = n.toElement();
        QString name  = e.attribute( "name" );
        e = n.namedItem( "url" ).toElement();
        KURL url  = KURL::KURL( e.text() );

        m_lastStream = new StreamEntry( m_streamsCategory, m_lastStream, url, name );
    }
    m_streamsCategory->setOpen( true );
}

void PlaylistBrowser::loadCoolStreams()
{
    QFile file( locate( "data","amarok/data/Cool-Streams.xml" ) );
    if( !file.open( IO_ReadOnly ) )
        return;

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;

    if( !d.setContent( stream.read() ) )
    {
        error() << "Bad Cool Streams XML file" << endl;
        return;
    }

    PlaylistCategory *folder = new PlaylistCategory( m_streamsCategory, 0, i18n("Cool-Streams") );
    KListViewItem *last = 0;

    QDomNode n = d.namedItem( "coolstreams" ).firstChild();

    for( ; !n.isNull(); n = n.nextSibling() )
    {
        QDomElement e = n.toElement();
        QString name = e.attribute( "name" );
        e = n.namedItem( "url" ).toElement();
        KURL url  = KURL::KURL( e.text() );
        last = new StreamEntry( folder, last, url, name );
    }

    m_lastStream = folder;
}


void PlaylistBrowser::addStream( QListViewItem *parent )
{
    StreamEditor dialog( i18n("Stream"), this );

    if( !parent ) parent = static_cast<QListViewItem*>(m_streamsCategory);

    if( dialog.exec() == QDialog::Accepted )
    {
        m_lastStream = new StreamEntry( parent, m_lastStream, dialog.url(), dialog.name() );
        parent->setOpen( true );
    }

}


void PlaylistBrowser::editStreamURL( StreamEntry *item )
{
    StreamEditor dialog( this, item->title(), item->url().prettyURL() );

    if( dialog.exec() == QDialog::Accepted )
    {
        item->setTitle( dialog.name() );
        item->setURL( dialog.url() );
        item->setText(0, dialog.name() );
    }
}


void PlaylistBrowser::saveStreams()
{
    QFile file( streamBrowserCache() );

    if( !file.open( IO_WriteOnly ) ) return;

    QDomDocument doc;
    QDomElement streamB = doc.createElement( "streambrowser" );
    streamB.setAttribute( "product", "amaroK" );
    streamB.setAttribute( "version", APP_VERSION );
    doc.appendChild( streamB );

    PlaylistCategory *currentCat=0;

    #define m_streamsCategory static_cast<QListViewItem *>(m_streamsCategory)

    QListViewItem *it = m_streamsCategory->firstChild();

    for( int count = 0; count < m_streamsCategory->childCount(); count++ )
    {
        QDomElement i;

        if( !isCategory( it ) )
            currentCat = static_cast<PlaylistCategory*>(it->parent() );

        if( isStream( it ) )
        {
            i = doc.createElement("stream");
            StreamEntry *item = (StreamEntry*)it;
            i.setAttribute( "name", item->title() );

            QDomElement attr = doc.createElement( "url" );
            QDomText t = doc.createTextNode( escapeHTML( item->url().prettyURL() ) );
            attr.appendChild( t );
            i.appendChild( attr );
        }

        streamB.appendChild( i );

        it = it->nextSibling();
    }

    #undef m_streamsCategory

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << doc.toString();
}

/**
 *************************************************************************
 *  SMART-PLAYLISTS
 *************************************************************************
 **/

QString PlaylistBrowser::smartplaylistBrowserCache()
{
    return amaroK::saveLocation() + "smartplaylistbrowser_save.xml";
}

void PlaylistBrowser::addSmartPlaylist( QListViewItem *parent ) //SLOT
{
    if( CollectionDB::instance()->isEmpty() )
        return;

    if( !parent ) parent = static_cast<QListViewItem*>(m_smartCategory);

    SmartPlaylistEditor dialog( i18n("Untitled"), this );
    if( dialog.exec() == QDialog::Accepted ) {
        /*SmartPlaylist *item = */ new SmartPlaylist( parent, m_lastSmart, dialog.name(), dialog.query(), dialog.result( m_smartXml ) );
        parent->setOpen( true );
    }
}

void PlaylistBrowser::loadSmartPlaylists()
{
    if( CollectionDB::instance()->isEmpty() )
        return;
    QFile file( smartplaylistBrowserCache() );

    if( !file.open( IO_ReadOnly ) )
    {
        loadDefaultSmartPlaylists();
        m_smartCategory->setOpen( true );
        return;
    }
    loadDefaultSmartPlaylists();

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    if( !m_smartXml.setContent( stream.read() ) )
        return;

    if ( m_smartXml.namedItem( "smartplaylists" ).toElement().attribute("formatversion") == "1.0" ) {
        QDomNode n =  m_smartXml.namedItem( "smartplaylists" ).firstChild();

        for( ; !n.isNull(); n = n.nextSibling() )
        {
            QDomElement e = n.toElement();
            QDomElement query = e.elementsByTagName( "sqlquery" ).item(0).toElement();
            m_lastSmart = new SmartPlaylist( m_smartCategory, m_lastSmart, e.attribute( "name" ), query.text(), e );
        }
        m_smartCategory->setOpen( true );
    }
    else {
        //This is the first version of the format, so I'm setting this only to avoid problems for people that
        //was using CVS (and sebr's xml) before. FIXME: Load a set of defaults here?
        QDomElement smartB = m_smartXml.createElement( "smartplaylists" );
        m_smartXml.appendChild( smartB );
        smartB.setAttribute( "product", "amaroK" );
        smartB.setAttribute( "version", APP_VERSION );
        smartB.setAttribute( "formatversion", "1.0" );
    }
}

void PlaylistBrowser::loadDefaultSmartPlaylists()
{
    const QStringList genres  = CollectionDB::instance()->query( "SELECT DISTINCT name FROM genre;" );
    const QStringList artists = CollectionDB::instance()->artistList();
    SmartPlaylist *item;
    QueryBuilder qb;
    m_lastSmart = 0;

    PlaylistCategory *folder = new PlaylistCategory( m_smartCategory, 0, i18n("Collection") );

    /********** All Collection **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

    item = new SmartPlaylist( folder, m_lastSmart, i18n( "All Collection" ), qb.query() );
    item->setPixmap( 0, SmallIcon("kfm") );

    /********** Favorite Tracks **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( folder, item, i18n( "Favorite Tracks" ), qb.query() );
    foreach( artists ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabArtist, *it );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.setLimit( 0, 15 );

        m_lastSmart = new SmartPlaylist( item, m_lastSmart, i18n( "By %1" ).arg( *it ), qb.query() );
    }

    /********** Most Played **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( folder, item, i18n( "Most Played" ), qb.query() );
    m_lastSmart = 0;
    foreach( artists ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabArtist, *it );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
        qb.setLimit( 0, 15 );

        m_lastSmart = new SmartPlaylist( item, m_lastSmart, i18n( "By %1" ).arg( *it ), qb.query() );
    }

    /********** Newest Tracks **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( folder, item, i18n( "Newest Tracks" ), qb.query() );
    m_lastSmart = 0;
    foreach( artists ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabArtist, *it );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
        qb.setLimit( 0, 15 );

        m_lastSmart = new SmartPlaylist( item, m_lastSmart, i18n( "By %1" ).arg( *it ), qb.query() );
    }

    /********** Last Played **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valAccessDate, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( folder, item, i18n( "Last Played" ), qb.query() );

    /********** Never Played **************/
    qb.initSQLDrag();
    qb.exclusiveFilter( QueryBuilder::tabSong, QueryBuilder::tabStats, QueryBuilder::valURL );
    qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

    item = new SmartPlaylist( folder, item, i18n( "Never Played" ), qb.query() );

    /********** Ever Played **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valScore );

    item = new SmartPlaylist( folder, item, i18n( "Ever Played" ), qb.query() );

    /********** Genres **************/
    item = new SmartPlaylist( folder, item, i18n( "Genres" ), QString() );
    m_lastSmart = 0;
    foreach( genres ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabGenre, *it );
        qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

        m_lastSmart = new SmartPlaylist( item, m_lastSmart, i18n( "%1" ).arg( *it ), qb.query() );
    }

    /********** 50 Random Tracks **************/
    qb.initSQLDrag();
    qb.setOptions( QueryBuilder::optRandomize );
    qb.setLimit( 0, 50 );
    item = new SmartPlaylist( folder, item, i18n( "50 Random Tracks" ), qb.query() );

    m_lastSmart = folder;

}

void PlaylistBrowser::editSmartPlaylist( SmartPlaylist* item )
{
    SmartPlaylistEditor dialog( this, item->xml() );
    if( dialog.exec() == QDialog::Accepted ) {
        item->setXml( dialog.result( m_smartXml ) );
        item->sqlForTags = dialog.query();
        item->setText(0, dialog.name());
    }

}

void PlaylistBrowser::saveSmartPlaylists()
{
    QFile file( smartplaylistBrowserCache() );

    if( !file.open( IO_WriteOnly ) ) return;

    QDomNode node = m_smartXml.namedItem( "smartplaylists" );
    if ( !node.isNull() ) {
        m_smartXml.removeChild (node);
    }
    node = m_smartXml.namedItem( "xml" );
    if ( !node.isNull() ) {
        m_smartXml.removeChild (node);
    }
    QDomElement smartB = m_smartXml.createElement( "smartplaylists" );
    m_smartXml.appendChild( smartB );
    smartB.setAttribute( "product", "amaroK" );
    smartB.setAttribute( "version", APP_VERSION );
    smartB.setAttribute( "formatversion", "1.0" );


    PlaylistCategory *currentCat=0;

    #define m_smartCategory static_cast<QListViewItem *>(m_smartCategory)

    QListViewItem *it = m_smartCategory->firstChild();
    it = it->nextSibling();

    for( int count = 1 ; count < m_smartCategory->childCount(); count++ )
    {


        if( !isCategory( it ) )
            currentCat = static_cast<PlaylistCategory*>(it->parent() );

        if( isSmartPlaylist( it ) )
        {

            SmartPlaylist *item = (SmartPlaylist*)it;
            QDomElement i = m_smartXml.createElement( "smartplaylist" );
            smartB.appendChild( item->xml() );
        }



        it = it->nextSibling();
    }

    #undef m_streamsCategory

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << m_smartXml.toString();
}

// Warning - unpredictable when a smartplaylist which doesn't exist is requested.
SmartPlaylist *
PlaylistBrowser::getSmartPlaylist( QString name )
{
    QListViewItemIterator it( m_smartCategory );

    for( ; it.current(); ++it )
    {
        if( (*it)->text( 0 ) == name )
            break;
    }

    return static_cast<SmartPlaylist*>(*it);
}


/**
 *************************************************************************
 *  PARTIES
 *************************************************************************
 **/

void PlaylistBrowser::addPartyConfig()
{
    // Save the current party information for later use
}

void PlaylistBrowser::loadParties()
{
    // We can't dance if we have no legs!
}

void PlaylistBrowser::editPartyConfig()
{
    // Edit the chosen party
}

void PlaylistBrowser::saveParties()
{
    // Called by the dtor
}


/**
 *************************************************************************
 *  PLAYLISTS
 *************************************************************************
 **/

QString PlaylistBrowser::playlistBrowserCache()
{
    //returns the playlists stats cache file
    return amaroK::saveLocation() + "playlistbrowser_save.xml";
}

void PlaylistBrowser::loadPlaylists()
{
    QFile file( playlistBrowserCache() );

    m_lastPlaylist = 0;

    if( !file.open( IO_ReadOnly ) )
    {
        loadOldPlaylists();
        return;
    }

    QTextStream pStream( &file );
    pStream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;

    if( !d.setContent( pStream.read() ) )
    {
        error() << "Bad XML file" << endl;
        return;
    }

    //so we don't construct these QString all the time
    const QString PLAYLIST( "playlist" );

    QDomNode n = d.namedItem( "playlistbrowser" ).namedItem("playlist");

    for( ; !n.isNull(); n = n.nextSibling() )
    {
        if( n.nodeName() != PLAYLIST ) continue;
        QDomElement e = n.toElement();

        KURL url;
        url.setPath( e.attribute( "file" ) );

        e = n.namedItem( "tracks" ).toElement();
        int tracks = e.text().toInt();

        e = n.namedItem( "length" ).toElement();
        int length = e.text().toInt();

        m_lastPlaylist = new PlaylistEntry( m_playlistCategory, m_lastPlaylist, url, tracks, length );

    }

    m_playlistCategory->setOpen( true );
}

// In case this is the first run using the QDomDocument, for users upgrading from amaroK < 1.3
void PlaylistBrowser::loadOldPlaylists()
{
    QFile path( amaroK::saveLocation() + "playlistbrowser_save" );

    if( !path.open( IO_ReadOnly ) )
        return;

    QTextStream stream( &path );
    QString str, file;
    int tracks=0, length=0;
    QDateTime lastModified;
    KURL url;

    while ( !( str = stream.readLine() ).isNull() ) {
        if ( str.startsWith( "File=" ) ) {
            file = str.mid( 5 );
        }
        else {
            tracks = str.section( ',', 0, 0 ).toInt();
            length = str.section( ',', 1, 1 ).toInt();
            int time_t = str.section( ',', 2, 2 ).toInt();
            lastModified.setTime_t( time_t );

            QFileInfo fi( file );
            if( fi.exists() ) {
                if( fi.lastModified() != lastModified )
                    addPlaylist( file ); //load the playlist
                else {
                    url.setPath(file);
                    m_lastPlaylist = new PlaylistEntry( m_playlistCategory, m_lastPlaylist, url, tracks, length );
                }
            }
        }
    }
    m_playlistCategory->setOpen( true );
}

void PlaylistBrowser::addPlaylist( QString path, QListViewItem *parent, bool force )
{
    // this function adds a playlist to the playlist browser

    QFile file( path );
    if( !file.exists() ) return;

    bool exists = false;
    for( QListViewItemIterator it( m_listview ); *it; ++it )
        if( isPlaylist( *it ) && path == ((PlaylistEntry *)*it)->url().path() ) {
            exists = true; //the playlist is already in the playlist browser
            if( force )
                ((PlaylistEntry *)*it)->load(); //reload the playlist
        }

    if( !exists ) {
        if( m_lastPlaylist == 0 ) {    //first child
            removeButton->setEnabled( true );
            renameButton->setEnabled( true );
            deleteButton->setEnabled( true );
        }

        if( !parent ) parent = static_cast<QListViewItem*>(m_playlistCategory);

        KURL auxKURL;
        auxKURL.setPath(path);
        m_lastPlaylist = new PlaylistEntry( parent, m_lastPlaylist, auxKURL );
        parent->setOpen( true );
    }
}

void PlaylistBrowser::openPlaylist( QListViewItem *parent ) //SLOT
{
    // open a file selector to add playlists to the playlist browser
    QStringList files;
    files = KFileDialog::getOpenFileNames( QString::null, "*.m3u *.pls|" + i18n("Playlist Files"), this, i18n("Add Playlists") );

    const QStringList::ConstIterator end  = files.constEnd();
    for( QStringList::ConstIterator it = files.constBegin(); it != end; ++it )
        addPlaylist( *it, parent );
}

void PlaylistBrowser::savePlaylists()
{
    QFile file( playlistBrowserCache() );

    if( !file.open( IO_WriteOnly ) ) return;

    QDomDocument doc;
    QDomElement playlistB = doc.createElement( "playlistbrowser" );
    playlistB.setAttribute( "product", "amaroK" );
    playlistB.setAttribute( "version", APP_VERSION );
    doc.appendChild( playlistB );

    PlaylistCategory *currentCat=0;

    #define m_playlistCategory static_cast<QListViewItem *>(m_playlistCategory)

    QListViewItem *it = m_playlistCategory->firstChild();

    for( int count = 0; count < m_playlistCategory->childCount(); count++ )
    {
        QDomElement i;

        if( !isCategory( it ) )
            currentCat = static_cast<PlaylistCategory*>(it->parent() );

        if( isPlaylist( it ) )
        {
            i = doc.createElement("playlist");
            PlaylistEntry *item = (PlaylistEntry*)it;
            i.setAttribute( "file", item->url().path() );

            QDomElement attr = doc.createElement( "tracks" );
            QDomText t = doc.createTextNode( QString::number(item->trackCount()) );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = doc.createElement( "length" );
            t = doc.createTextNode( QString::number(item->length()) );
            attr.appendChild( t );
            i.appendChild( attr );

            QFileInfo fi( item->url().path() );
            attr = doc.createElement( "modified" );
            t = doc.createTextNode( QString::number(fi.lastModified().toTime_t()) );
            attr.appendChild( t );
            i.appendChild( attr );
        }

        playlistB.appendChild( i );

        it = it->nextSibling();
    }
    #undef m_playlistCategory
    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << doc.toString();

}

PlaylistEntry *
PlaylistBrowser::getPlaylist( QString name )
{
    QListViewItemIterator it( m_playlistCategory );

    for( ; it.current(); ++it )
    {
        if( (*it)->text( 0 ) == name )
            break;
    }

    return static_cast<PlaylistEntry*>(*it);
}


/**
 *************************************************************************
 *  General Methods
 *************************************************************************
 **/

void PlaylistBrowser::saveCurrentPlaylist()
{
    PlaylistSaver dialog( i18n("Current Playlist"), this );

    if( dialog.exec() == QDialog::Accepted )
    {
        QString name = dialog.title();

        // TODO Remove this hack for 1.2. It's needed because playlists was a file once.
        QString folder = KGlobal::dirs()->saveLocation( "data", "amarok/playlists", false );
        QFileInfo info( folder );
        if ( !info.isDir() ) QFile::remove( folder );

        QString path = KGlobal::dirs()->saveLocation( "data", "amarok/playlists/", true ) + name + ".m3u";
        debug() << "Saving Current-Playlist to: " << path << endl;
        if ( !Playlist::instance()->saveM3U( path ) ) {
            KMessageBox::sorry( this, i18n( "Cannot write playlist (%1).").arg(path) );
            return;
        }
        addPlaylist( path );
    }

}

void PlaylistBrowser::slotDoubleClicked( QListViewItem *item ) //SLOT
{
    if( !item ) return;

    if( isPlaylist( item ) ) {
        // open the playlist
        #define item static_cast<PlaylistEntry *>(item)
        //don't replace, it generally makes people think amaroK behaves like JuK
        //and we don't so they then get really confused about things
        Playlist::instance()->insertMedia( item->tracksURL(), Playlist::Replace );
        #undef item
    }
    else if( isStream( item ) )
    {
        Playlist::instance()->insertMedia( static_cast<StreamEntry *>(item)->url(), Playlist::Replace );
    }
    else if( isSmartPlaylist( item ) )
    {
        #define item static_cast<SmartPlaylist *>(item)
        if( !item->sqlForTags.isEmpty() )
            Playlist::instance()->insertMediaSql( item->sqlForTags, Playlist::Clear );
        #undef item
    }
    else if( isCategory( item ) )
    {
        item->setOpen( !item->isOpen() );
    }
    else if( isPlaylistTrackItem( item ) )
    {
        KURL::List list( static_cast<PlaylistTrackItem *>(item)->url() );
        Playlist::instance()->insertMedia( list, Playlist::DirectPlay );
    }
    else
        //If you remove this, please also remove #include "statusbar.h", thanks.
        amaroK::StatusBar::instance()->shortMessage( i18n("No functionality for item double click implemented") );
}



void PlaylistBrowser::removeSelectedItems() //SLOT
{
    // this function remove selected playlists and tracks

    //remove currentItem, no matter if selected or not
    m_listview->setSelected( m_listview->currentItem(), true );

    QPtrList<QListViewItem> selected;
    QListViewItemIterator it( m_listview, QListViewItemIterator::Selected);
    for( ; it.current(); ++it ) {
        // if the playlist containing this item is already selected the current item will be skipped
        // it will be deleted from the parent
        QListViewItem *parent = it.current()->parent();
        if( parent && parent->isSelected() )
            continue;

        if( isCategory( *it ) )
        {
            if( static_cast<PlaylistCategory*>(*it)->isFolder() )
            {
                if( (*it)->parent()->text(0) == i18n("Cool-Streams") )
                    continue;
            }
            else
                continue;
        }

        selected.append( it.current() );
    }

    for( QListViewItem *item = selected.first(); item; item = selected.next() ) {

        if( isPlaylist( item ) ) {
            //remove the playlist
            if( item == m_lastPlaylist ) {
                QListViewItem *above = item->itemAbove();
                m_lastPlaylist = above ? (PlaylistEntry *)above : 0;
            }
            delete item;
        }
        else if( isStream( item ) ) {
            if( item == m_lastStream ) {
                QListViewItem *above = item->itemAbove();
                m_lastStream = above ? (StreamEntry *)above : 0;
            }
            delete item;
        }
        else if( isSmartPlaylist( item ) ) {
            if( item == m_lastSmart ) {
                QListViewItem *above = item->itemAbove();
                m_lastSmart = above ? (SmartPlaylist *)above : 0;
            }
            delete item;
        }
        else if( isCategory( item ) ) {
            if( item == m_lastSmart ) {
                QListViewItem *above = item->itemAbove();
                m_lastSmart = above ? (SmartPlaylist *)above : 0;
            } else if( item == m_lastStream ) {
                QListViewItem *above = item->itemAbove();
                m_lastStream = above ? (StreamEntry *)above : 0;
            } else if( item == m_lastPlaylist ) {
                QListViewItem *above = item->itemAbove();
                m_lastPlaylist = above ? (PlaylistEntry *)above : 0;
            }
            delete item;
        }
        else if( isPlaylistTrackItem( item ) ) {
            //remove the track
            PlaylistEntry *playlist = (PlaylistEntry *)item->parent();
            playlist->removeTrack( item );
        }
    }
}


void PlaylistBrowser::renameSelectedItem() //SLOT
{
    QListViewItem *item = m_listview->currentItem();
    if( !item ) return;

    if( isCategory( item ) )
    {
        if( static_cast<PlaylistCategory*>(item)->isFolder() )
        {
            if( item->parent()->text(0) == i18n("Cool-Streams") )
                return;
            item->setRenameEnabled( 0, true );
            m_listview->rename( item, 0 );
        }
    }
    else if( isPlaylist( item ) || isStream( item ) || isSmartPlaylist( item ) ) {
        item->setRenameEnabled( 0, true );
        m_listview->rename( item, 0 );
    }
}


void PlaylistBrowser::renamePlaylist( QListViewItem* item, const QString& newName, int ) //SLOT
{
    if( isPlaylist( item ) )
    {
        #define item static_cast<PlaylistEntry*>(item)

        QString oldPath = item->url().path();
        QString newPath = fileDirPath( oldPath ) + newName + fileExtension( oldPath );

        if ( rename( QFile::encodeName( oldPath ), QFile::encodeName( newPath ) ) == -1 )
            KMessageBox::error( this, i18n("Error renaming the file.") );
        else
            item->setUrl( newPath );

        #undef item
    }

    item->setRenameEnabled( 0, false );
}


void PlaylistBrowser::deleteSelectedPlaylists() //SLOT
{
    KURL::List urls;

    //delete currentItem, no matter if selected or not
    m_listview->setSelected( m_listview->currentItem(), true );

    QListViewItemIterator it( m_listview, QListViewItemIterator::Selected );
    for( ; it.current(); ++it ) {
        if( isPlaylist( *it ) )
            urls.append( ((PlaylistEntry *)*it)->url() );
        else    //we want to delete only playlists
            m_listview->setSelected( it.current(), false );
    }

    if ( urls.isEmpty() ) return;

    int button = KMessageBox::warningContinueCancel( this, i18n( "<p>You have selected 1 playlist to be <b>irreversibly</b> deleted.",
                                                                 "<p>You have selected %n playlists to be <b>irreversibly</b> deleted.",
                                                                 urls.count() ),
                                                     QString::null,
                                                     KGuiItem(i18n("&Delete"),"editdelete") );

    if ( button == KMessageBox::Continue )
    {
        // TODO We need to check which files have been deleted successfully
        KIO::DeleteJob* job = KIO::del( urls );
        connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( removeSelectedItems() ) );
    }
}

QStringList
PlaylistBrowser::selectedList()
{
    QStringList selected;

    QListViewItemIterator it( m_listview, QListViewItemIterator::Selected);
    while( *it )
    {
        if( !isCategory( *it ) && !isPlaylistTrackItem( *it ) )
        {
            selected << (*it)->text(0);
            if( isPlaylist( *it ) )
                selected << i18n("Playlist");
            else if( isSmartPlaylist( *it ) )
                selected << i18n("Smart Playlist");
        }
        ++it;
    }

    return selected;
}

void PlaylistBrowser::savePlaylist( PlaylistEntry *item )
{
    bool append = false;

    if( item->trackList().count() == 0 ) //the playlist hasn't been loaded so we append the dropped tracks
        append = true;

    //save the modified playlist in m3u or pls format
    const QString ext = fileExtension( item->url().path() );
    if( ext.lower() == ".m3u" )
        saveM3U( item, append );
    else
        savePLS( item, append );

    item->setModified( false );    //don't show the save icon
}


void PlaylistBrowser::saveM3U( PlaylistEntry *item, bool append )
{
    QFile file( item->url().path() );

    if( append ? file.open( IO_WriteOnly | IO_Append ) : file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        if( !append )
            stream << "#EXTM3U\n";
        QPtrList<TrackItemInfo> trackList = append ? item->droppedTracks() : item->trackList();
        for( TrackItemInfo *info = trackList.first(); info; info = trackList.next() )
        {
            stream << "#EXTINF:";
            stream << info->length();
            stream << ',';
            stream << info->title();
            stream << '\n';
            stream << (info->url().protocol() == "file" ? info->url().path() : info->url().url());
            stream << "\n";
        }

        file.close();
    }
}


void PlaylistBrowser::savePLS( PlaylistEntry *item, bool append )
{
    QFile file( item->url().path() );

    if( append ? file.open( IO_WriteOnly | IO_Append ) : file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        QPtrList<TrackItemInfo> trackList = append ? item->droppedTracks() : item->trackList();
        for( TrackItemInfo *info = trackList.first(); info; info = trackList.next() )
        {
            stream << "File=";
            stream << (info->url().protocol() == "file" ? info->url().path() : info->url().url());
            stream << "\nTitle=";
            stream << info->title();
            stream << "\nLength=";
            stream << info->length();
            stream << "\n";
        }

        file.close();
    }
}


void PlaylistBrowser::currentItemChanged( QListViewItem *item )    //SLOT
{
    // rename remove and delete buttons are disabled if there are no playlists
    // rename and delete buttons are disabled for track items

    bool enable_remove = false;
    bool enable_rename = false;
    bool enable_delete = false;

    if( !item )
        goto enable_buttons;

    else if( isPlaylist( item ) )
    {
        enable_remove = true;
        enable_rename = true;
        enable_delete = true;
    }
    else if( isStream( item ) )
    {
        enable_remove = ( item->parent()->text(0) != i18n("Cool-Streams") );
        enable_rename = ( item->parent()->text(0) != i18n("Cool-Streams") );
        enable_delete = false;
    }
    else if( isCategory( item ) )
    {
        if( static_cast<PlaylistCategory*>(item)->isFolder() )
        {
            if( item->text(0) != i18n("Cool-Streams") ) {
                enable_remove = true;
                enable_rename = true;
                enable_delete = true;
            }
        }
    }
    else
        enable_remove = true;


    enable_buttons:

    removeButton->setEnabled( enable_remove );
    renameButton->setEnabled( enable_rename );
    deleteButton->setEnabled( enable_delete );
}


void PlaylistBrowser::customEvent( QCustomEvent *e )
{
    //if a playlist is found in collection folders it will be automatically added to the playlist browser

    // the CollectionReader sends a PlaylistFoundEvent when a playlist is found
    CollectionReader::PlaylistFoundEvent* p = (CollectionReader::PlaylistFoundEvent*)e;
    addPlaylist( p->path() );
}

void PlaylistBrowser::slotAddMenu( int id ) //SLOT
{
    switch( id )
    {
        case PLAYLIST:
            openPlaylist();
            break;

        case STREAM:
            addStream();
            break;

        case SMARTPLAYLIST:
            addSmartPlaylist();
            break;

        default:
            break;
    }
}

void PlaylistBrowser::slotSaveMenu( int id ) // SLOT
{
    switch( id )
    {
        case PLAYLIST:
            saveCurrentPlaylist();
            break;

        case PARTY:
            break;

        default:
            break;
    }
}

void PlaylistBrowser::slotViewMenu( int id ) //SL0T
{
    if( m_viewMode == (ViewMode) id )
        return;

    switch ( id ) {
        case UNSORTED:
            m_sortMode = id;
            m_listview->setSorting( -1 );
            viewMenuButton->popupMenu()->setItemChecked( UNSORTED, true );
            viewMenuButton->popupMenu()->setItemChecked( ASCENDING, false );
            viewMenuButton->popupMenu()->setItemChecked( DESCENDING, false );
            return;
        case ASCENDING:
            m_sortMode = id;
            m_listview->setSorting( 0, true );
            viewMenuButton->popupMenu()->setItemChecked( UNSORTED, false );
            viewMenuButton->popupMenu()->setItemChecked( ASCENDING, true );
            viewMenuButton->popupMenu()->setItemChecked( DESCENDING, false );
            return;
        case DESCENDING:
            m_sortMode = id;
            m_listview->setSorting( 0, false );
            viewMenuButton->popupMenu()->setItemChecked( UNSORTED, false );
            viewMenuButton->popupMenu()->setItemChecked( ASCENDING, false );
            viewMenuButton->popupMenu()->setItemChecked( DESCENDING, true );
            return;
        default:
            break;
    }

    viewMenuButton->popupMenu()->setItemChecked( m_viewMode, false );
    viewMenuButton->popupMenu()->setItemChecked( id, true );
    m_viewMode = (ViewMode) id;

    QListViewItemIterator it( m_listview );
    for( ; it.current(); ++it )
        it.current()->setup();
}

/**
 ************************
 *  Context Menu Entries
 ************************
 **/

void PlaylistBrowser::showContextMenu( QListViewItem *item, const QPoint &p, int )  //SLOT
{
    if( !item ) return;

    KPopupMenu menu( this );

    if( isPlaylist( item ) ) {
        #define item static_cast<PlaylistEntry*>(item)
        enum Id { LOAD, ADD, SAVE, RESTORE, RENAME, REMOVE, DELETE };

        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );
        menu.insertSeparator();
        if( item->isModified() )
        {
            menu.insertItem( SmallIconSet("filesave"), i18n( "&Save" ), SAVE );
            menu.insertItem( i18n( "Res&tore" ), RESTORE );
            menu.insertSeparator();
        }
        menu.insertItem( SmallIconSet("editclear"), i18n( "&Rename" ), RENAME );
        menu.insertItem( SmallIconSet("edittrash"), i18n( "R&emove" ), REMOVE );
        menu.insertItem( SmallIconSet("editdelete"), i18n( "&Delete" ), DELETE );
        menu.setAccel( Key_Space, LOAD );
        menu.setAccel( Key_F2, RENAME );
        menu.setAccel( Key_Delete, REMOVE );
        menu.setAccel( SHIFT+Key_Delete, DELETE );

        switch( menu.exec( p ) )
        {
            case LOAD:
                slotDoubleClicked( item );
                break;
            case ADD:
                Playlist::instance()->insertMedia( item->tracksURL() );
                break;
            case SAVE:
                savePlaylist( item );
                break;
            case RESTORE:
                item->restore();
                break;
            case RENAME:
                renameSelectedItem();
                break;
            case REMOVE:
                removeSelectedItems();
                break;
            case DELETE:
                deleteSelectedPlaylists();
                break;
        }
        #undef item
    }
    else if( isSmartPlaylist( item ) )
    {
        enum Actions { LOAD, ADD, EDIT, REMOVE };

        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );
        menu.insertSeparator();
        // Forbid removal of Cool-Streams
        if( item->parent()->text(0) != i18n("Collection") )
        {
            if ( static_cast<SmartPlaylist *>(item)->isEditable() )
                menu.insertItem( SmallIconSet("editclear"), i18n( "E&dit" ), EDIT );
            menu.insertItem( SmallIconSet("edittrash"), i18n( "R&emove" ), REMOVE );
        }

        switch( menu.exec( p ) )
        {
            case LOAD:
                slotDoubleClicked( item );
                break;
            case ADD:
                Playlist::instance()->insertMediaSql( static_cast<SmartPlaylist *>(item)->sqlForTags, Playlist::Clear );
                break;
            case EDIT:
                editSmartPlaylist( static_cast<SmartPlaylist *>(item) );
                break;
            case REMOVE:
                removeSelectedItems();
                break;
        }
    }
    else if( isStream( item ) )
    {
        enum Actions { LOAD, ADD, EDIT, REMOVE };

        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );
        menu.insertSeparator();
        // Forbid removal of Cool-Streams
        if( item->parent()->text(0) != i18n("Cool-Streams") )
        {
            menu.insertItem( SmallIconSet("editclear"), i18n( "E&dit" ), EDIT );
            menu.insertItem( SmallIconSet("edittrash"), i18n( "R&emove" ), REMOVE );
        }

        switch( menu.exec( p ) )
        {
            case LOAD:
                slotDoubleClicked( item );
                break;
            case ADD:
                Playlist::instance()->insertMedia( static_cast<StreamEntry *>(item)->url() );
                break;
            case EDIT:
                editStreamURL( static_cast<StreamEntry *>(item) );
                break;
            case REMOVE:
                removeSelectedItems();
                break;
        }
    }
    else if( isCategory( item ) ) {
        #define item static_cast<PlaylistCategory*>(item)
        enum Actions { RENAME, REMOVE, CREATE, PLAYLIST, SMART, STREAM, FOLDER };

        QListViewItem *parentCat = item;

        while( parentCat->parent() )
            parentCat = parentCat->parent();

        if( item->text(0) == i18n("Cool-Streams") && item->parent()->text(0) == i18n("Streams") ) return;
        if( item->text(0) == i18n("Collection") && item->parent()->text(0) == i18n("Smart Playlists") ) return;

        if( item->isFolder() ) {
            menu.insertItem( SmallIconSet("editclear"), i18n( "&Rename" ), RENAME );
            menu.insertItem( SmallIconSet("edittrash"), i18n( "R&emove" ), REMOVE );
            menu.insertSeparator();
        }

        if( parentCat == static_cast<QListViewItem*>(m_playlistCategory) )
            menu.insertItem( SmallIconSet("edit_add"), i18n("Add Playlist"), PLAYLIST );

        else if( parentCat == static_cast<QListViewItem*>(m_smartCategory) )
            menu.insertItem( SmallIconSet("edit_add"), i18n("Add Smart-Playlist"), SMART );

        else if( parentCat == static_cast<QListViewItem*>(m_streamsCategory) )
            menu.insertItem( SmallIconSet("edit_add"), i18n("Add Stream"), STREAM );

        menu.insertItem( SmallIconSet("folder"), i18n("Create Sub-Folder"), CREATE );

        switch( menu.exec( p ) ) {
            case RENAME:
                renameSelectedItem();
                break;

            case REMOVE:
                removeSelectedItems();
                break;

            case PLAYLIST:
                openPlaylist( item );
                break;

            case SMART:
                addSmartPlaylist( item );
                break;

            case STREAM:
                addStream( item );
                break;

            case CREATE:
                QListViewItem *tracker = item->firstChild();
                uint c=0;
                for(  ; isCategory( tracker ); tracker = tracker->nextSibling() )
                {
                    if( tracker->text(0).startsWith("Folder") )
                        c++;
                    if( !isCategory( tracker->nextSibling() ) )
                        break;
                }
                QString name = i18n("Folder");
                if( c ) name = i18n("Folder %1").arg(c);

                new PlaylistCategory( item, tracker, name, true );

                break;
        }
        #undef item
    }
    else if( isPlaylistTrackItem( item ) )
    {
    //******** track menu ***********
        #define item static_cast<PlaylistTrackItem*>(item)

        enum Actions { MAKE, APPEND, QUEUE, BURN_DATACD, BURN_AUDIOCD, REMOVE, INFO };

        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue After Current Track" ), QUEUE );
        menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );


        menu.insertSeparator();

        menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn to CD as Data"), BURN_DATACD );
        menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() && item->url().isLocalFile() );
        menu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("Burn to CD as Audio"), BURN_AUDIOCD );
        menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() && item->url().isLocalFile() );

        menu.insertSeparator();

        menu.insertItem( SmallIconSet("edittrash"), i18n( "&Remove" ), REMOVE );
        menu.insertItem( SmallIconSet("info"), i18n( "&View/Edit Meta Information" ), INFO );

        switch( menu.exec( p ) ) {
            case MAKE:
                Playlist::instance()->clear(); //FALL THROUGH
            case APPEND:
                Playlist::instance()->insertMedia( item->url() );
                break;
            case QUEUE:
                Playlist::instance()->insertMedia( item->url(), Playlist::Queue );
                break;
            case BURN_DATACD:
                 K3bExporter::instance()->exportTracks( item->url(), K3bExporter::DataCD );
                 break;
            case BURN_AUDIOCD:
                 K3bExporter::instance()->exportTracks( item->url(), K3bExporter::AudioCD );
                 break;
            case REMOVE:
                removeSelectedItems();
                break;
            case INFO:
                if( QFile::exists( item->url().path() ) ) {
                    TagDialog* dialog = new TagDialog( item->url() );
                    dialog->show();
                }
                else KMessageBox::sorry( this, i18n( "This file does not exist: %1" ).arg( item->url().path() ) );
        }
        #undef item
   }
}

/////////////////////////////////////////////////////////////////////////////
//    CLASS PlaylistBrowserView
////////////////////////////////////////////////////////////////////////////

PlaylistBrowserView::PlaylistBrowserView( QWidget *parent, const char *name )
    : KListView( parent, name )
    , m_marker( 0 )
{
    addColumn( i18n("Playlists") );
    setSelectionMode( QListView::Extended );
    setShowSortIndicator( true );
    setRootIsDecorated( true );

    setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    setDropHighlighter( true );    //and the highligther (a focus rect) is drawn when dragging over playlists
    setDropVisualizerWidth( 3 );
    setAcceptDrops( true );

    setFullWidth( true );
    setTreeStepSize( 20 );

    //<loading animation>
    m_loading1 = new QPixmap( locate("data", "amarok/images/loading1.png" ) );
    m_loading2 = new QPixmap( locate("data", "amarok/images/loading2.png" ) );
    m_animationTimer = new QTimer();
    connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );
    //</loading animation>

    connect( this, SIGNAL( mouseButtonPressed ( int, QListViewItem *, const QPoint &, int ) ),
            this, SLOT( mousePressed( int, QListViewItem *, const QPoint &, int ) ) );

    //TODO moving tracks
    //connect( this, SIGNAL( moved(QListViewItem *, QListViewItem *, QListViewItem * )),
    //        this, SLOT( itemMoved(QListViewItem *, QListViewItem *, QListViewItem * )));
}


PlaylistBrowserView::~PlaylistBrowserView()
{
    delete m_animationTimer;
    delete m_loading1;
    delete m_loading2;
}


void PlaylistBrowserView::startAnimation( PlaylistEntry *item )
{
    //starts the loading animation for item
    m_loadingItems.append( item );
    if( !m_animationTimer->isActive() )
        m_animationTimer->start( 100 );
}


void PlaylistBrowserView::stopAnimation( PlaylistEntry *item )
{
    //stops the loading animation for item
    m_loadingItems.remove( item );
    if( !m_loadingItems.count() )
        m_animationTimer->stop();
}


void PlaylistBrowserView::slotAnimation() //SLOT
{
    static uint iconCounter=1;

    for( QListViewItem *item = m_loadingItems.first(); item; item = m_loadingItems.next() )
        ((PlaylistEntry *)item)->setLoadingPix( iconCounter==1 ? m_loading1 : m_loading2 );

    iconCounter++;
    if( iconCounter > 2 )
        iconCounter = 1;
}


void PlaylistBrowserView::contentsDragEnterEvent( QDragEnterEvent *e )
{
    e->accept( e->source() != viewport() && KURLDrag::canDecode( e ) );
}

void PlaylistBrowserView::contentsDragMoveEvent( QDragMoveEvent* e )
{
    //Get the closest item _before_ the cursor
    const QPoint p = contentsToViewport( e->pos() );
    QListViewItem *item = itemAt( p );
    if( !item ) {
        eraseMarker();
        return;
    }

    //only for track items (for playlist items we draw the highlighter)
    if( isPlaylistTrackItem( item ) )
        item = item->itemAbove();

    if( item != m_marker )
    {
        eraseMarker();
        m_marker = item;
        viewportPaintEvent( 0 );
    }
}


void PlaylistBrowserView::contentsDragLeaveEvent( QDragLeaveEvent* )
{
     eraseMarker();
}


void PlaylistBrowserView::contentsDropEvent( QDropEvent *e )
{
    QListViewItem *parent = 0;
    QListViewItem *after;

    const QPoint p = contentsToViewport( e->pos() );
    QListViewItem *item = itemAt( p );
    if( !item ) {
        eraseMarker();
        return;
    }

    if( !isPlaylist( item ) )
        findDrop( e->pos(), parent, after );

    eraseMarker();

    if( e->source() == viewport() )
        e->ignore();    //TODO add support to move tracks
    else {
        KURL::List list;
        QMap<QString, QString> map;
        if( KURLDrag::decode( e, list, map ) ) {
            if( parent ) {
                //insert the dropped tracks
                PlaylistEntry *playlist = (PlaylistEntry *)parent;
                playlist->insertTracks( after, list, map );
            }
            else //dropped on a playlist item
            {
                PlaylistEntry *playlist = (PlaylistEntry *)item;
                //append the dropped tracks
                playlist->insertTracks( 0, list, map );
            }
        }
        else
            e->ignore();
    }

}


void PlaylistBrowserView::eraseMarker() //SLOT
{
    if( m_marker )
    {
        QRect spot;
        if( isPlaylist( m_marker ) )
            spot = drawItemHighlighter( 0, m_marker );
        else
            spot = drawDropVisualizer( 0, 0, m_marker );

        m_marker = 0;
        viewport()->repaint( spot, false );
    }
}


void PlaylistBrowserView::viewportPaintEvent( QPaintEvent *e )
{
    if( e ) KListView::viewportPaintEvent( e ); //we call with 0 in contentsDropEvent()

    if( m_marker )
    {
        QPainter painter( viewport() );
        if( isPlaylist( m_marker ) )    //when dragging on a playlist we draw a focus rect
            drawItemHighlighter( &painter, m_marker );
        else //when dragging on a track we draw a line marker
            painter.fillRect( drawDropVisualizer( 0, 0, m_marker ),
                                   QBrush( colorGroup().highlight(), QBrush::Dense4Pattern ) );
    }
}


void PlaylistBrowserView::mousePressed( int button, QListViewItem *item, const QPoint &pnt, int )    //SLOT
{
    // this function expande/collapse the playlist if the +/- symbol has been pressed
    // and show the save menu if the save icon has been pressed

    if( !item || button != LeftButton ) return;

    if( isPlaylist( item ) ) {

        QPoint p = mapFromGlobal( pnt );
        p.setY( p.y() - header()->height() );

        QRect itemrect = itemRect( item );

        QRect expandRect = QRect( 4, itemrect.y() + (item->height()/2) - 5, 15, 15 );
        if( expandRect.contains( p ) ) {    //expand symbol clicked
            setOpen( item, !item->isOpen() );
            return;
        }

        if( static_cast<PlaylistEntry*>(item)->isModified() ) {
            QRect saveRect = QRect( 23, itemrect.y() + 3, 16, 16 );
            if( saveRect.contains( p ) ) {

                enum Id { SAVE, RESTORE };

                KPopupMenu saveMenu( this );
                saveMenu.insertItem( SmallIconSet("filesave"), i18n( "&Save" ), SAVE );
                saveMenu.insertItem( i18n( "&Restore" ), RESTORE );

                switch( saveMenu.exec( pnt ) ) {
                    case SAVE:
                        PlaylistBrowser::instance()->savePlaylist( static_cast<PlaylistEntry*>(item) );
                        break;

                    case RESTORE:
                        static_cast<PlaylistEntry*>(item)->restore();
                        break;
                }
            }

        }

    }


}


void PlaylistBrowserView::rename( QListViewItem *item, int c )
{
    KListView::rename( item, c );

    QRect rect( itemRect( item ) );
    int fieldX = rect.x() + treeStepSize() + 2;
    int fieldW = rect.width() - treeStepSize() - 2;

    KLineEdit *renameEdit = renameLineEdit();
    renameEdit->setGeometry( fieldX, rect.y(), fieldW, rect.height() );
    renameEdit->show();
}


void PlaylistBrowserView::keyPressEvent( QKeyEvent *e )
{
    switch( e->key() ) {
         case Key_Space:    //load
            PlaylistBrowser::instance()->slotDoubleClicked( currentItem() );
            break;

        case Key_Delete:    //remove
            PlaylistBrowser::instance()->removeSelectedItems();
            break;

        case Key_F2:    //rename
            PlaylistBrowser::instance()->renameSelectedItem();
            break;

        case SHIFT+Key_Delete:    //delete
            PlaylistBrowser::instance()->deleteSelectedPlaylists();
            break;

        default:
            KListView::keyPressEvent( e );
            break;
    }
}


void PlaylistBrowserView::startDrag()
{
    KURL::List urls;

    QListViewItemIterator it( this, QListViewItemIterator::Selected );
    for( ; it.current(); ++it ) {
        if( isPlaylist( *it ) )
            urls += ((PlaylistEntry*)*it)->tracksURL();

        else if( isStream( *it ) )
            urls += ((StreamEntry*)*it)->url();

        else if( isSmartPlaylist( *it ) )
        {
            kdDebug() << "{PLAYLIST BROWSER] DRAGGING A SMARTPLAYLIST!" << endl;
            SmartPlaylist *item = (SmartPlaylist*)*it;
            if( !item->sqlForTags.isEmpty() )
            {
                QStringList list = CollectionDB::instance()->query( item->sqlForTags );
                if( list.isEmpty() ) kdDebug() << "Query returned nothing" << endl;
                for( uint c=0; c < list.count(); c++ )
                    urls += KURL( list[c] );
            }
        }
        else if( isPlaylistTrackItem( *it ) )
            urls += ((PlaylistTrackItem*)*it)->url();
    }

    KURLDrag *d = new KURLDrag( urls, viewport() );
    d->dragCopy();

}

#include "playlistbrowser.moc"
