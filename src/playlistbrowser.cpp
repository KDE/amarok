// (c) 2004 Pierpaolo Di Panfilo
// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// (c) 2005 Gábor Lehel <illissius@gmail.com>
// License: GPL V2. See COPYING file for information.

#define DEBUG_PREFIX "PlaylistBrowser"

#include "amarok.h"            //actionCollection()
#include "browserToolBar.h"
#include "collectiondb.h"      //smart playlists
#include "collectionreader.h"
#include "debug.h"
#include "k3bexporter.h"
#include "mediabrowser.h"
#include "party.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistbrowseritem.h"
#include "smartplaylisteditor.h"
#include "tagdialog.h"         //showContextMenu()
#include "threadweaver.h"

#include <qevent.h>            //customEvent()
#include <qheader.h>           //mousePressed()
#include <qlabel.h>
#include <qpainter.h>          //paintCell()
#include <qpixmap.h>           //paintCell()
#include <qtextstream.h>       //loadPlaylists(), saveM3U(), savePLS()
#include <qtimer.h>            //loading animation

#include <kaction.h>
#include <kactionclasses.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kfiledialog.h>       //openPlaylist()
#include <kio/job.h>           //deleteSelectedPlaylists()
#include <kiconloader.h>       //smallIcon
#include <kinputdialog.h>
#include <klineedit.h>         //rename()
#include <klocale.h>
#include <kmessagebox.h>       //renamePlaylist(), deleteSelectedPlaylist()
#include <kmultipledrag.h>     //dragObject()
#include <kpopupmenu.h>
#include <kstandarddirs.h>     //KGlobal::dirs()
#include <kurldrag.h>          //dragObject()

#include <cstdio>              //rename() in renamePlaylist()


inline QString
fileExtension( const QString &fileName )
{
    return amaroK::extension( fileName );
}


PlaylistBrowser *PlaylistBrowser::s_instance = 0;


static inline bool isDynamicEnabled() { return AmarokConfig::dynamicMode(); }


PlaylistBrowser::PlaylistBrowser( const char *name )
        : QVBox( 0, name )
        , m_polished( false )
        , m_smartCategory( 0 )
        , m_coolStreams( 0 )
        , m_smartDefaults( 0 )
        , m_ac( new KActionCollection( this ) )
        , m_podcastTimer( new QTimer( this ) )
{
    s_instance = this;

    QVBox *browserBox = new QVBox( this );
    browserBox->setSpacing( 3 );

    //<Toolbar>
    addMenuButton  = new KActionMenu( i18n("Add"), "fileopen", m_ac );
    addMenuButton->setDelayed( false );

    KPopupMenu *addMenu  = addMenuButton->popupMenu();
    addMenu->insertItem( i18n("Playlist..."), PLAYLIST );
    addMenu->insertItem( i18n("Radio Stream..."), STREAM );
    addMenu->insertItem( i18n("Smart Playlist..."), SMARTPLAYLIST );
    addMenu->insertItem( i18n("Podcast..."), PODCAST );
    connect( addMenu, SIGNAL( activated(int) ), SLOT( slotAddMenu(int) ) );

    saveMenuButton = new KActionMenu( i18n("Save"), "filesave", m_ac );
    saveMenuButton->setDelayed( false );

    KPopupMenu *saveMenu = saveMenuButton->popupMenu();
    saveMenu->insertItem( i18n("Current Playlist..."), CURRENT );
    saveMenu->insertItem( i18n("Dynamic Playlist..."), DYNAMIC );
    connect( saveMenu, SIGNAL( activated(int) ), SLOT( slotSaveMenu(int) ) );

    renameButton   = new KAction( i18n("Rename"), "editclear", 0, this, SLOT( renameSelectedItem() ), m_ac );
    removeButton   = new KAction( i18n("Remove"), "edittrash", 0, this, SLOT( removeSelectedItems() ), m_ac );

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
    saveMenuButton->plug( m_toolbar );

    m_toolbar->setIconText( KToolBar::IconOnly, false );      //default appearance
    m_toolbar->insertLineSeparator();
    renameButton->plug( m_toolbar);
    removeButton->plug( m_toolbar );
    m_toolbar->insertLineSeparator();
    viewMenuButton->plug( m_toolbar );
    m_toolbar->insertSeparator();
    m_toolbar->setIconText( KToolBar::IconTextRight, false );

    renameButton->setEnabled( false );
    removeButton->setEnabled( false );
    //</Toolbar>

    m_listview = new PlaylistBrowserView( browserBox );

    KConfig *config = amaroK::config( "PlaylistBrowser" );
    m_viewMode = (ViewMode)config->readNumEntry( "View", LISTVIEW );  //restore the view mode
    viewMenu->setItemChecked( m_viewMode, true );
    m_sortMode = config->readNumEntry( "Sorting", ASCENDING );
    slotViewMenu( m_sortMode );

    m_podcastTimerInterval = config->readNumEntry( "Podcast Interval", 14400000 );

    new Party( browserBox );

    // signals and slots connections
    connect( m_listview, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
             this,         SLOT( showContextMenu( QListViewItem *, const QPoint &, int ) ) );
    connect( m_listview, SIGNAL( doubleClicked( QListViewItem *) ),
             this,         SLOT( slotDoubleClicked( QListViewItem * ) ) );
    connect( m_listview, SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
             this,         SLOT( renamePlaylist( QListViewItem*, const QString&, int ) ) );
    connect( m_listview, SIGNAL( currentChanged( QListViewItem * ) ),
             this,         SLOT( currentItemChanged( QListViewItem * ) ) );
    connect( CollectionDB::instance(), SIGNAL( scanDone( bool ) ), SLOT( collectionScanDone() ) );

    setMinimumWidth( m_toolbar->sizeHint().width() );


    // FIXME the following code moved here from polish(), until the width
    // forgetting issue is fixed:

    m_polished = true;

    m_playlistCategory = loadPlaylists();
    if( !CollectionDB::instance()->isEmpty() ) {
        m_smartCategory = loadSmartPlaylists();
        loadDefaultSmartPlaylists();
        m_smartCategory->setOpen( true );
    }
    m_dynamicCategory = loadDynamics();
    m_streamsCategory = loadStreams();
    loadCoolStreams();

    // must be loaded after streams
    m_podcastCategory = loadPodcasts();

    m_playlistCategory->setOpen( true );
    m_podcastCategory->setOpen( true );
    m_streamsCategory->setOpen( true );
    m_dynamicCategory->setOpen( true );

    QStringList playlists = AmarokConfig::dynamicCustomList();

    for( uint i=0; i < playlists.count(); i++ )
    {
        QListViewItem *item = m_listview->findItem( playlists[i], 0, Qt::ExactMatch );
        if( item )
        {
            m_dynamicEntries.append( item );
            if ( item->rtti() == PlaylistEntry::RTTI )
                static_cast<PlaylistEntry*>( item )->setDynamic( true );
            if ( item->rtti() == SmartPlaylist::RTTI )
                static_cast<SmartPlaylist*>( item )->setDynamic( true );
        }
    }

    // ListView item state restoration:
    // First we check if the number of items in the listview is the same as it was on last
    // application exit. If true, we iterate over all items and restore their open/closed state.
    // Note: We ignore podcast items, because they are added dynamically added to the ListView.

    QValueList<int> stateList = config->readIntListEntry( "Item State" );
    QListViewItemIterator it( m_listview );
    uint count = 0;
    while ( it.current() ) {
        if( !isPodcastItem( it.current() ) )
            ++count;
        ++it;
    }

    if ( count == stateList.count() ) {
        uint index = 0;
        it = QListViewItemIterator( m_listview );
        while ( it.current() ) {
            if( !isPodcastItem( it.current() ) ) {
                it.current()->setOpen( stateList[index] );
                ++index;
            }
            ++it;
        }
    }
}


void
PlaylistBrowser::polish()
{
    // we make startup faster by doing the slow bits for this
    // only when we are shown on screen

    DEBUG_BLOCK

//     amaroK::OverrideCursor allocate_on_stack;

    QVBox::polish();

    // FIXME the following code moved to the ctor, until the width forgetting
    // issue is fixed:

/*    m_polished = true;

    KConfig *config = amaroK::config( "PlaylistBrowser" );

    m_playlistCategory = loadPlaylists();
    m_streamsCategory  = loadStreams();
    loadCoolStreams();

    if( !CollectionDB::instance()->isEmpty() ) {
        m_smartCategory = loadSmartPlaylists();
        loadDefaultSmartPlaylists();
        m_smartCategory->setOpen( true );
    }
    // must be loaded after streams
    m_podcastCategory = loadPodcasts();

    m_dynamicCategory = loadDynamics();

    m_playlistCategory->setOpen( true );
    m_podcastCategory->setOpen( true );
    m_streamsCategory->setOpen( true );
    m_dynamicCategory->setOpen( true );

    QStringList playlists = AmarokConfig::dynamicCustomList();

    for( uint i=0; i < playlists.count(); i++ )
    {
        QListViewItem *item = m_listview->findItem( playlists[i], 0, Qt::ExactMatch );
        if( item )
        {
            item->setPixmap( 1, SmallIcon("favorites") );
            m_dynamicEntries.append( item );
        }
    }

    // ListView item state restoration:
    // First we check if the number of items in the listview is the same as it was on last
    // application exit. If true, we iterate over all items and restore their open/closed state.
    // Note: We ignore podcast items, because they are added dynamically added to the ListView.

    QValueList<int> stateList = config->readIntListEntry( "Item State" );
    QListViewItemIterator it( m_listview );
    uint count = 0;
    while ( it.current() ) {
        if( !isPodcastItem( it.current() ) )
            ++count;
        ++it;
    }

    if ( count == stateList.count() ) {
        uint index = 0;
        it = QListViewItemIterator( m_listview );
        while ( it.current() ) {
            if( !isPodcastItem( it.current() ) ) {
                it.current()->setOpen( stateList[index] );
                ++index;
            }
            ++it;
        }
    }*/
}


PlaylistBrowser::~PlaylistBrowser()
{
    DEBUG_BLOCK

    if( m_polished )
    {
        // <markey> Not sure if these calls are still needed, now that we're saving
        //          the state after each change.
        savePlaylists();
        savePodcasts();
        saveStreams();
        saveSmartPlaylists();
        saveDynamics();

        QStringList list;
        for( uint i=0; i < m_dynamicEntries.count(); i++ )
        {
            QListViewItem *item = m_dynamicEntries.at( i );
            list.append( item->text(0) );
        }

        AmarokConfig::setDynamicCustomList( list );

        KConfig *config = amaroK::config( "PlaylistBrowser" );
        config->writeEntry( "View", m_viewMode );
        config->writeEntry( "Sorting", m_sortMode );

        // Save open/closed state of each listview item
        QValueList<int> stateList;
        QListViewItemIterator it( m_listview );
        while ( it.current() ) {
            if( !isPodcastItem( it.current() ) )
                stateList.append( it.current()->isOpen() ? 1 : 0 );
            ++it;
        }
        config->writeEntry( "Item State", stateList );
        config->writeEntry( "Podcast Interval", m_podcastTimerInterval );
    }
}


/**
 *************************************************************************
 *  STREAMS
 *************************************************************************
 **/

QString PlaylistBrowser::streamBrowserCache() const
{
    return amaroK::saveLocation() + "streambrowser_save.xml";
}


PlaylistCategory* PlaylistBrowser::loadStreams()
{
    QFile file( streamBrowserCache() );

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;
    QDomElement e;

    QListViewItem *after = m_dynamicCategory;

    if( !file.open( IO_ReadOnly ) || !d.setContent( stream.read() ) )
    { /*Couldn't open the file or it had invalid content, so let's create an empty element*/
        return new PlaylistCategory(m_listview, after , i18n("Radio Streams") );
    }
    else {
        e = d.namedItem( "category" ).toElement();
        if ( e.attribute("formatversion") =="1.1" ) {
            PlaylistCategory* p = new PlaylistCategory(m_listview, after, e );
            p->setText(0, i18n("Radio Streams") );
            return p;
        }
        else { // Old unversioned format
            PlaylistCategory* p = new PlaylistCategory(m_listview, after, i18n("Radio Streams") );
            QListViewItem *last = 0;
            QDomNode n = d.namedItem( "streambrowser" ).namedItem("stream");
            for( ; !n.isNull();  n = n.nextSibling() ) {
                last = new StreamEntry( p, last, n.toElement() );
            }
            return p;
        }
    }
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

    m_coolStreams = new PlaylistCategory( m_streamsCategory, 0, i18n("Cool-Streams") );
    KListViewItem *last = 0;

    QDomNode n = d.namedItem( "coolstreams" ).firstChild();

    for( ; !n.isNull(); n = n.nextSibling() )
    {
        QDomElement e = n.toElement();
        QString name = e.attribute( "name" );
        e = n.namedItem( "url" ).toElement();
        KURL url  = KURL::KURL( e.text() );
        last = new StreamEntry( m_coolStreams, last, url, name );
    }
}


void PlaylistBrowser::addStream( QListViewItem *parent )
{
    StreamEditor dialog( this );

    if( !parent ) parent = static_cast<QListViewItem*>(m_streamsCategory);

    if( dialog.exec() == QDialog::Accepted )
    {
        new StreamEntry( parent, 0, dialog.url(), dialog.name() );
        parent->sortChildItems( 0, true );
        parent->setOpen( true );

        saveStreams();
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

    QDomDocument doc;
    QDomElement streamB = m_streamsCategory->xml();
    streamB.setAttribute( "product", "amaroK" );
    streamB.setAttribute( "version", APP_VERSION );
    streamB.setAttribute( "formatversion", "1.1" );
    QDomNode streamsNode = doc.importNode( streamB, true );
    doc.appendChild( streamsNode );

    QString temp( doc.toString() );

    // Only open the file after all data is ready. If it crashes, data is not lost!
    if ( !file.open( IO_WriteOnly ) ) return;

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << temp;
}

/**
 *************************************************************************
 *  SMART-PLAYLISTS
 *************************************************************************
 **/

QString PlaylistBrowser::smartplaylistBrowserCache() const
{
    return amaroK::saveLocation() + "smartplaylistbrowser_save.xml";
}

void PlaylistBrowser::addSmartPlaylist( QListViewItem *parent ) //SLOT
{
    if( CollectionDB::instance()->isEmpty() || !m_smartCategory )
        return;

    if( !parent ) parent = static_cast<QListViewItem*>(m_smartCategory);

    SmartPlaylistEditor dialog( i18n("Untitled"), this );
    if( dialog.exec() == QDialog::Accepted ) {
        new SmartPlaylist( parent, 0, dialog.result() );
        parent->sortChildItems( 0, true );
        parent->setOpen( true );

        saveSmartPlaylists();
    }
}

PlaylistCategory* PlaylistBrowser::loadSmartPlaylists()
{

    QFile file( smartplaylistBrowserCache() );
    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    QListViewItem *after = m_playlistCategory;

    QDomDocument d;
    QDomElement e;

    if( !file.open( IO_ReadOnly ) || !d.setContent( stream.read() ) )
    { /*Couldn't open the file or it had invalid content, so let's create an empty element*/
        return new PlaylistCategory(m_listview, after, i18n("Smart Playlists") );
    }
    else {
        e = d.namedItem( "category" ).toElement();
        if ( e.attribute("formatversion") =="1.1" ) {
            PlaylistCategory* p = new PlaylistCategory(m_listview, after, e );
            p->setText( 0, i18n("Smart Playlists") );
            return p;
        }
        else { // Old unversioned format
            PlaylistCategory* p = new PlaylistCategory(m_listview, after , i18n("Smart Playlists") );
            QListViewItem *last = 0;
            QDomNode n = d.namedItem( "smartplaylists" ).namedItem("smartplaylist");
            for( ; !n.isNull();  n = n.nextSibling() ) {
                last = new SmartPlaylist( p, last, n.toElement() );
            }
            return p;
        }
    }
}

void PlaylistBrowser::loadDefaultSmartPlaylists()
{
    const QStringList genres  = CollectionDB::instance()->query( "SELECT DISTINCT name FROM genre;" );
    const QStringList artists = CollectionDB::instance()->artistList();
    SmartPlaylist *item;
    QueryBuilder qb;
    QListViewItem *last = 0;

    m_smartDefaults = new PlaylistCategory( m_smartCategory, 0, i18n("Collection") );

    /********** All Collection **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

    item = new SmartPlaylist( m_smartDefaults, 0, i18n( "All Collection" ), qb.query() );
    item->setPixmap( 0, SmallIcon("collection") );

    /********** Favorite Tracks **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( m_smartDefaults, item, i18n( "Favorite Tracks" ), qb.query() );
    last = 0;
    foreach( artists ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabArtist, *it );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.setLimit( 0, 15 );

        last = new SmartPlaylist( item, last, i18n( "By %1" ).arg( *it ), qb.query() );
    }

    /********** Most Played **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( m_smartDefaults, item, i18n( "Most Played" ), qb.query() );
    last = 0;
    foreach( artists ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabArtist, *it );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
        qb.setLimit( 0, 15 );

        last = new SmartPlaylist( item, last, i18n( "By %1" ).arg( *it ), qb.query() );
    }

    /********** Newest Tracks **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( m_smartDefaults, item, i18n( "Newest Tracks" ), qb.query() );
    last = 0;
    foreach( artists ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabArtist, *it );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
        qb.setLimit( 0, 15 );

        last = new SmartPlaylist( item, last, i18n( "By %1" ).arg( *it ), qb.query() );
    }

    /********** Last Played **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valAccessDate, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( m_smartDefaults, item, i18n( "Last Played" ), qb.query() );

    /********** Never Played **************/
    qb.initSQLDrag();
    qb.exclusiveFilter( QueryBuilder::tabSong, QueryBuilder::tabStats, QueryBuilder::valURL );
    qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

    item = new SmartPlaylist( m_smartDefaults, item, i18n( "Never Played" ), qb.query() );

    /********** Ever Played **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valScore );

    item = new SmartPlaylist( m_smartDefaults, item, i18n( "Ever Played" ), qb.query() );

    /********** Genres **************/
    item = new SmartPlaylist( m_smartDefaults, item, i18n( "Genres" ), QString() );
    last = 0;
    foreach( genres ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabGenre, *it );
        qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

        last = new SmartPlaylist( item, last, i18n( "%1" ).arg( *it ), qb.query() );
    }

    /********** 50 Random Tracks **************/
    qb.initSQLDrag();
    qb.setOptions( QueryBuilder::optRandomize );
    qb.setLimit( 0, 50 );
    item = new SmartPlaylist( m_smartDefaults, item, i18n( "50 Random Tracks" ), qb.query() );
}

void PlaylistBrowser::editSmartPlaylist( SmartPlaylist* item )
{
    SmartPlaylistEditor dialog( this, item->xml() );
    if( dialog.exec() == QDialog::Accepted ) {
        item->setXml( dialog.result() );
        item->setText(0, dialog.name());
    }
}

void PlaylistBrowser::saveSmartPlaylists()
{
    QFile file( smartplaylistBrowserCache() );

    // If the user hadn't set a collection, we didn't create the Smart Playlist Item
    if( !m_smartCategory ) return;

    QDomDocument doc;
    QDomElement smartB = m_smartCategory->xml();
    smartB.setAttribute( "product", "amaroK" );
    smartB.setAttribute( "version", APP_VERSION );
    smartB.setAttribute( "formatversion", "1.1" );
    QDomNode smartplaylistsNode = doc.importNode( smartB, true );
    doc.appendChild( smartplaylistsNode );

    QString temp( doc.toString() );

    // Only open the file after all data is ready. If it crashes, data is not lost!
    if ( !file.open( IO_WriteOnly ) ) return;

    QTextStream smart( &file );
    smart.setEncoding( QTextStream::UnicodeUTF8 );
    smart << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    smart << temp;
}

/**
 *************************************************************************
 *  PARTIES
 *************************************************************************
 **/

QString PlaylistBrowser::partyBrowserCache() const
{
    return amaroK::saveLocation() + "partybrowser_save.xml";
}

void PlaylistBrowser::addDynamic( QListViewItem *parent )
{
    Party *current = Party::instance();

    if( !parent ) parent = m_dynamicCategory;

    bool ok;
    QString name = KInputDialog::getText(i18n("Save Dynamic Playlist"), i18n("Enter playlist name:"), i18n("Untitled"), &ok, this);

    if( ok )
    {
        PartyEntry *saveMe = new PartyEntry( parent, 0, name );

        saveMe->setCycled( current->cycleTracks() );
        saveMe->setMarked( current->markHistory() );
        saveMe->setUpcoming( current->upcomingCount() );
        saveMe->setPrevious( current->previousCount() );
        saveMe->setAppendCount( current->appendCount() );
        saveMe->setAppendType( current->appendType() );

        QStringList list;
        if( current->appendType() == 2 )
        {
            debug() << "Saving custom list..." << endl;
            for( uint c = 0; c < m_dynamicEntries.count(); c++ )
            {
                debug() << "Appending: " << ( m_dynamicEntries.at(c) )->text(0) << endl;
                list.append( ( m_dynamicEntries.at(c) )->text(0) );
            }
        }
        saveMe->setItems( list );
        parent->sortChildItems( 0, true );
        parent->setOpen( true );

        saveDynamics();
    }
}

PlaylistCategory* PlaylistBrowser::loadDynamics()
{
    QFile file( partyBrowserCache() );

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;
    QDomElement e;

    PlaylistCategory *after = m_smartCategory;
    if( CollectionDB::instance()->isEmpty() || !m_smartCategory ) // incase of no collection
        after = m_playlistCategory;

    if( !file.open( IO_ReadOnly ) || !d.setContent( stream.read() ) )
    { /*Couldn't open the file or it had invalid content, so let's create some defaults*/
        PlaylistCategory *p = new PlaylistCategory( m_listview, after, i18n("Dynamic Playlists") );
        QListViewItem *random = new PartyEntry( p, 0, i18n("Random Mix") );
        //new PartyEntry has all the features we want for random mix
        PartyEntry *suggested = new PartyEntry( p, random, i18n("Suggested Songs" ) );
        suggested->setAppendType( PartyEntry::SUGGESTION );
        return p;
    }
    else {
        e = d.namedItem( "category" ).toElement();
        if ( e.attribute("formatversion") =="1.1" ) {
            PlaylistCategory* p = new PlaylistCategory( m_listview, after , e );
            p->setText( 0, i18n("Dynamic Playlists") );
            return p;
        }
        else { // Old unversioned format
            PlaylistCategory* p = new PlaylistCategory( m_listview, after, i18n("Dynamic Playlists") );
            QListViewItem *last = 0;
            QDomNode n = d.namedItem( "partybrowser" ).namedItem("party");
            for( ; !n.isNull();  n = n.nextSibling() ) {
                last = new PartyEntry( p, last, n.toElement() );
            }
            return p;
        }
    }
}

void PlaylistBrowser::saveDynamics()
{
    QFile file( partyBrowserCache() );
    QTextStream stream( &file );

    QDomDocument doc;
    QDomElement dynamicB = m_dynamicCategory->xml();
    dynamicB.setAttribute( "product", "amaroK" );
    dynamicB.setAttribute( "version", APP_VERSION );
    dynamicB.setAttribute( "formatversion", "1.1" );
    QDomNode dynamicsNode = doc.importNode( dynamicB, true );
    doc.appendChild( dynamicsNode );

    QString temp( doc.toString() );

    // Only open the file after all data is ready. If it crashes, data is not lost!
    if ( !file.open( IO_WriteOnly ) ) return;

    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << temp;
}

void PlaylistBrowser::loadDynamicItems()
{
    // Make sure all items are unmarked
    for( uint i=0; i < m_dynamicEntries.count(); i++ )
    {
        QListViewItem *it = m_dynamicEntries.at( i );

        if( it )
        {
            if( isPlaylist( it ) )
            {
                PlaylistEntry *item = static_cast<PlaylistEntry*>(it);
                item->setDynamic( false );
            }
            else if( isSmartPlaylist( it ) )
            {
                SmartPlaylist *item = static_cast<SmartPlaylist*>(it);
                item->setDynamic( false );
            }
        }
    }
    m_dynamicEntries.clear();  // Dont use remove(), since we do i++, which would cause skip overs!!!

    // Mark appropriate items as used
    if( AmarokConfig::dynamicType() == "Custom" )
    {
        QStringList playlists = AmarokConfig::dynamicCustomList();
        for( uint i=0; i < playlists.count(); i++ )
        {
            QListViewItem *it = findItem( playlists[i], 0 );

            if( it )
            {
                m_dynamicEntries.append( it );
                if( isPlaylist( it ) )
                {
                    PlaylistEntry *item = static_cast<PlaylistEntry*>(it);
                    item->setDynamic( true );
                }
                else if( isSmartPlaylist( it ) )
                {
                    SmartPlaylist *item = static_cast<SmartPlaylist*>(it);
                    item->setDynamic( true );
                }
            }
        }
    }
}

/**
 *************************************************************************
 *  PODCASTS
 *************************************************************************
 **/

QString PlaylistBrowser::podcastBrowserCache() const
{
    //returns the playlists stats cache file
    return amaroK::saveLocation() + "podcastbrowser_save.xml";
}

PlaylistCategory* PlaylistBrowser::loadPodcasts()
{
    QFile file( podcastBrowserCache() );
    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;
    QDomElement e;

    QListViewItem *after = m_streamsCategory;

    if( !file.open( IO_ReadOnly ) || !d.setContent( stream.read() ) )
    { /*Couldn't open the file or it had invalid content, so let's create an empty element*/
        return new PlaylistCategory( m_listview, after, i18n("Podcasts") );
    }
    else {
        m_podcastItemsToScan.clear();
        if( !m_podcastTimerInterval ) m_podcastTimerInterval = 14400000;  // 4 hours

        connect( m_podcastTimer, SIGNAL(timeout()), this, SLOT(scanPodcasts()) );

        e = d.namedItem( "category" ).toElement();
        PlaylistCategory *p = new PlaylistCategory( m_listview, after, e );
        p->setText( 0, i18n("Podcasts") );

        if( !m_podcastItemsToScan.isEmpty() )
            m_podcastTimer->start( m_podcastTimerInterval );

        return p;
    }
}

void PlaylistBrowser::savePodcasts()
{
    QFile file( podcastBrowserCache() );
    QTextStream stream( &file );

    QDomDocument doc;
    QDomElement podcastB = m_podcastCategory->xml();
    podcastB.setAttribute( "product", "amaroK" );
    podcastB.setAttribute( "version", APP_VERSION );
    podcastB.setAttribute( "formatversion", "1.1" );
    QDomNode podcastNode = doc.importNode( podcastB, true );
    doc.appendChild ( podcastNode );

    QString temp( doc.toString() );

    // Only open the file after all data is ready. If it crashes, data is not lost!
    if ( !file.open( IO_WriteOnly ) ) return;

    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << temp;
}

void PlaylistBrowser::scanPodcasts()
{
    //restart timer
    m_podcastTimer->start( m_podcastTimerInterval );

    for( uint i=0; i < m_podcastItemsToScan.count(); i++ )
    {
        QListViewItem  *item = m_podcastItemsToScan.at( i );
        PodcastChannel *pc   = static_cast<PodcastChannel*>(item);
        pc->rescan();
    }
}


void PlaylistBrowser::refreshPodcasts( QListViewItem *parent )
{
    for( QListViewItem *child = parent->firstChild();
            child;
            child = child->nextSibling() )
    {
        if( isPodcastChannel( child ) )
            static_cast<PodcastChannel*>( child )->rescan();
        else if( isCategory( child ) )
            refreshPodcasts( child );
    }
}

void PlaylistBrowser::addPodcast( QListViewItem *parent )
{
    bool ok;
    const QString name = KInputDialog::getText(i18n("Add Podcast"), i18n("Enter Podcast URL:"), QString::null, &ok, this);

    if( ok && !name.isEmpty() )
    {
        addPodcast( name, parent );
    }
}

void PlaylistBrowser::addPodcast( const QString &url, QListViewItem *parent )
{
    if( !parent ) parent = static_cast<QListViewItem*>(m_podcastCategory);

    PodcastChannel *pc = new PodcastChannel( parent, 0, KURL( url ) );

    if( m_podcastItemsToScan.isEmpty() )
    {
        m_podcastItemsToScan.append( pc );
        m_podcastTimer->start( m_podcastTimerInterval );
    }
    else
    {
        m_podcastItemsToScan.append( pc );
    }

    parent->sortChildItems( 0, true );
    parent->setOpen( true );

    savePodcasts();
}

void PlaylistBrowser::changePodcastInterval()
{
    double time = static_cast<double>(m_podcastTimerInterval / ( 60 * 60 * 1000 ));
    bool ok;
    double interval = KInputDialog::getDouble( i18n("Download Interval"),
                                            i18n("Scan interval (hours):"), time,
                                            0.5, 100.0, .5, 1, // min, max, step, base
                                            &ok, this);
    int milliseconds = static_cast<int>(interval*60.0*60.0*1000.0);
    if( ok )
    {
        if( milliseconds != m_podcastTimerInterval )
        {
            m_podcastTimerInterval = milliseconds;
            m_podcastTimer->changeInterval( m_podcastTimerInterval );
        }
    }
}

bool PlaylistBrowser::deletePodcasts( QPtrList<PodcastChannel> items )
{
    if ( items.isEmpty() ) return false;

    int button = KMessageBox::warningContinueCancel( this, i18n( "<p>You have selected 1 podcast to be <b>irreversibly</b> deleted. "
                                                                 "All downloaded episodes will also be deleted.",
                                                                 "<p>You have selected %n podcasts to be <b>irreversibly</b> deleted. "
                                                                 "All downloaded episodes will also be deleted.",
                                                                 items.count() ),
                                                     QString::null,
                                                     KStdGuiItem::del() );

    if ( button == KMessageBox::Continue )
    {
        KURL::List urls;
        foreachType( QPtrList<PodcastChannel>, items )
        {
            for( QListViewItem *ch = (*it)->firstChild(); ch; ch = ch->nextSibling() )
            {
                #define ch static_cast<PodcastItem*>(ch)
                if( ch->hasDownloaded() )
                {
                    //delete downloaded media
                    urls.append( ch->localUrl() );
                }
                #undef  ch
            }
            //delete downloaded xml
            urls.append( (*it)->xmlUrl() );
        }
        // TODO We need to check which files have been deleted successfully
        KIO::del( urls );
        return true;
    }
    return false;
}

void PlaylistBrowser::downloadSelectedPodcasts()
{
    QListViewItemIterator it( m_listview, QListViewItemIterator::Selected );

    for( ; it.current(); ++it )
    {
        if( isPodcastItem( *it ) )
        {
            #define item static_cast<PodcastItem*>(*it)
            if( !item->hasDownloaded() )
                m_podcastDownloadQueue.append( item );
            #undef  item
        }
    }
    downloadPodcastQueue();
}

void PlaylistBrowser::downloadPodcastQueue() //SLOT
{
    if( m_podcastDownloadQueue.isEmpty() ) return;

    PodcastItem *first = m_podcastDownloadQueue.first();
    first->downloadMedia();
    m_podcastDownloadQueue.removeFirst();

    connect( first, SIGNAL( downloadFinished() ), this, SLOT( downloadPodcastQueue() ) );
}

void PlaylistBrowser::setGlobalPodcastSettings( PodcastChannel *item )
{
    debug() << "Playlistbrowser is modifying global podcastsettings" << endl;
    const QString save   = item->saveLocation().path();
    const bool autoFetch = item->autoScan();
    const int mediaType  = item->mediaFetch();
    const bool purge     = item->hasPurge();
    const int purgeCount = item->purgeCount();

    QListViewItem *channel = m_podcastCategory->firstChild();

    for( ; channel; channel = channel->itemBelow() )
    {
        if( !isPodcastChannel( channel ) || channel == item )
            continue;
        #define channel static_cast<PodcastChannel*>(channel)
        debug() << "Settings are being saved for: " << channel->title() << endl;
        channel->setSettings( save, autoFetch, mediaType, purge, purgeCount );
        #undef  channel
    }
}

/**
 *************************************************************************
 *  PLAYLISTS
 *************************************************************************
 **/

QString PlaylistBrowser::playlistBrowserCache() const
{
    //returns the playlists stats cache file
    return amaroK::saveLocation() + "playlistbrowser_save.xml";
}

PlaylistCategory* PlaylistBrowser::loadPlaylists()
{
    QFile file( playlistBrowserCache() );

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;
    QDomElement e;

    if( !file.open( IO_ReadOnly ) || !d.setContent( stream.read() ) )
    { /*Couldn't open the file or it had invalid content, so let's create an empty element*/
        return new PlaylistCategory(m_listview, 0 , i18n("Playlists") );
    }
    else {
        e = d.namedItem( "category" ).toElement();
        if ( e.attribute("formatversion") =="1.1" ) {
            PlaylistCategory* p = new PlaylistCategory(m_listview, 0 , e );
            p->setText( 0, i18n("Playlists") );
            return p;
        }
        else { // Old unversioned format
            PlaylistCategory* p = new PlaylistCategory(m_listview, 0 , i18n("Playlists") );
            QListViewItem *last = 0;
            QDomNode n = d.namedItem( "playlistbrowser" ).namedItem("playlist");
            for( ; !n.isNull();  n = n.nextSibling() ) {
                last = new PlaylistEntry( p, last, n.toElement() );
            }
            return p;
        }
    }
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
                QListViewItem *last = 0;
                if( fi.lastModified() != lastModified )
                    addPlaylist( file ); //load the playlist
                else {
                    url.setPath(file);
                    last = new PlaylistEntry( m_playlistCategory, last, url, tracks, length );
                }
            }
        }
    }
    m_playlistCategory->setOpen( true );
}

void PlaylistBrowser::addPlaylist( const QString &path, QListViewItem *parent, bool force )
{
    // this function adds a playlist to the playlist browser

    if( !m_polished )
       polish();

    QFile file( path );
    if( !file.exists() ) return;

    PlaylistEntry *playlist = 0;
    for( QListViewItemIterator it( m_listview ); *it; ++it )
        if( isPlaylist( *it ) && path == ((PlaylistEntry *)*it)->url().path() ) {
            playlist = ((PlaylistEntry *)*it); //the playlist is already in the playlist browser
            parent = (*it)->parent();
            if( force )
                playlist->load(); //reload the playlist
            break;
        }

    if( !parent ) parent = static_cast<QListViewItem*>(m_playlistCategory);

    if( !playlist ) {
        if( !m_playlistCategory || !m_playlistCategory->childCount() ) {    //first child
            removeButton->setEnabled( true );
            renameButton->setEnabled( true );
        }

        KURL auxKURL;
        auxKURL.setPath(path);
        m_lastPlaylist = playlist = new PlaylistEntry( parent, 0, auxKURL );
    }

    parent->setOpen( true );
    parent->sortChildItems( 0, true );
    m_listview->clearSelection();
    playlist->setSelected( true );
}

bool PlaylistBrowser::savePlaylist( const QString &path, const QValueList<KURL> &in_urls,
                                    const QValueList<QString> &titles, const QValueList<QString> &seconds,
                                    bool relative )
{
    if( path.isEmpty() )
        return false;

    QFile file( path );

    if( !file.open( IO_WriteOnly ) )
    {
        KMessageBox::sorry( PlaylistWindow::self(), i18n( "Cannot write playlist (%1).").arg(path) );
        return false;
    }

    QTextStream stream( &file );
    stream << "#EXTM3U\n";

    KURL::List urls;
    for( int i = 0, n = in_urls.count(); i < n; ++i )
    {
        const KURL &url = in_urls[i];
        if( url.isLocalFile() && QFileInfo( url.path() ).isDir() )
            urls += recurse( url );
        else
            urls += url;
    }

    for( int i = 0, n = urls.count(); i < n; ++i )
    {
        const KURL &url = urls[i];

        if( !titles.isEmpty() && !seconds.isEmpty() )
        {
            stream << "#EXTINF:";
            stream << seconds[i];
            stream << ',';
            stream << titles[i];
            stream << '\n';
        }
        if (url.protocol() == "file" ) {
            if ( relative ) {
                const QFileInfo fi(file);
                stream << KURL::relativePath(fi.dirPath(), url.path());
            } else
                stream << url.path();
        } else {
            stream << url.url();
        }
        stream << "\n";
    }

    file.close(); // Flushes the file, before we read it
    PlaylistBrowser::instance()->addPlaylist( path, 0, true );

    return true;
}

void PlaylistBrowser::openPlaylist( QListViewItem *parent ) //SLOT
{
    // open a file selector to add playlists to the playlist browser
    QStringList files;
    files = KFileDialog::getOpenFileNames( QString::null, "*.m3u *.pls|" + i18n("Playlist Files"), this, i18n("Add Playlists") );

    const QStringList::ConstIterator end  = files.constEnd();
    for( QStringList::ConstIterator it = files.constBegin(); it != end; ++it )
        addPlaylist( *it, parent );

    savePlaylists();
}

void PlaylistBrowser::savePlaylists()
{
    QFile file( playlistBrowserCache() );

    QDomDocument doc;
    QDomElement playlistsB = m_playlistCategory->xml();
    playlistsB.setAttribute( "product", "amaroK" );
    playlistsB.setAttribute( "version", APP_VERSION );
    playlistsB.setAttribute( "formatversion", "1.1" );
    QDomNode playlistsNode = doc.importNode( playlistsB, true );
    doc.appendChild( playlistsNode );

    QString temp( doc.toString() );

    // Only open the file after all data is ready. If it crashes, data is not lost!
    if ( !file.open( IO_WriteOnly ) ) return;

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << temp;
}

bool PlaylistBrowser::deletePlaylists( QPtrList<PlaylistEntry> items )
{
    KURL::List urls;
    foreachType( QPtrList<PlaylistEntry>, items )
    {
        urls.append( (*it)->url() );
    }
    if( !urls.isEmpty() )
        return deletePlaylists( urls );

    return false;
}

bool PlaylistBrowser::deletePlaylists( KURL::List items )
{
    if ( items.isEmpty() ) return false;

    int button = KMessageBox::warningContinueCancel( this, i18n( "<p>You have selected 1 playlist to be <b>irreversibly</b> deleted.",
                                                                 "<p>You have selected %n playlists to be <b>irreversibly</b> deleted.",
                                                                 items.count() ),
                                                     QString::null,
                                                     KStdGuiItem::del() );

    if ( button == KMessageBox::Continue )
    {
        // TODO We need to check which files have been deleted successfully
        KIO::del( items );
        // Avoid deleting dirs. See bug #122480
        for ( KURL::List::iterator it = items.begin(), end = items.end(); it != end; ++it ) {
            if ( QFileInfo( (*it).path() ).isDir() ) {
                it = items.remove( it );
                continue;
            }
        }
        return true;
    }
    return false;
}

void PlaylistBrowser::savePlaylist( PlaylistEntry *item )
{
    bool append = false;

    if( item->trackList().count() == 0 ) //the playlist hasn't been loaded so we append the dropped tracks
        append = true;

    //save the modified playlist in m3u or pls format
    const QString ext = fileExtension( item->url().path() );
    if( ext.lower() == "m3u" )
        saveM3U( item, append );
    else
        savePLS( item, append );

    item->setModified( false );    //don't show the save icon
}

/**
 *************************************************************************
 *  General Methods
 *************************************************************************
 **/

PlaylistBrowserEntry *
PlaylistBrowser::findItem( QString &t, int c ) const
{
    return (PlaylistBrowserEntry *)m_listview->findItem( t, c, Qt::ExactMatch );
}

bool PlaylistBrowser::createPlaylist( QListViewItem *parent, bool current )
{
    const QString path = PlaylistDialog::getSaveFileName( i18n("Untitled") );
    debug() << "Creating playlist" << endl;
    if( path.isEmpty() )
        return false;

    if( !parent )
        parent = static_cast<QListViewItem *>( m_playlistCategory );

    debug() << "Saving Playlist to: " << path << endl;

    if( current )
    {
        if ( !Playlist::instance()->saveM3U( path ) ) {
            return false;
        }
    }
    else
    {
        debug() << "not current!" << endl;

	//Remove any items in Listview that have the same path as this one
        //  Should only happen when overwriting a playlist
        QListViewItem *item = parent->firstChild();
        while (item)
        {
            if (static_cast<PlaylistEntry*>(item)->url() == path)
            {
                QListViewItem *todelete=item;
                item = item->nextSibling();
                delete todelete;
            }
            else
                item = item->nextSibling();
        }

        //Remove existing playlist if it exists
        if (QFileInfo(path).exists())
            QFileInfo(path).dir().remove(path);

        m_lastPlaylist = new PlaylistEntry( parent, 0, path );
        parent->setOpen( true );
        parent->sortChildItems( 0, true );
    }

    savePlaylists();

    return true;
}

void PlaylistBrowser::slotDoubleClicked( QListViewItem *item ) //SLOT
{
    if( !item ) return;

    if( isPlaylist( item ) ) {
        // open the playlist
        #define item static_cast<PlaylistEntry *>(item)
        //don't replace, it generally makes people think amaroK behaves like JuK
        //and we don't so they then get really confused about things
        Playlist::instance()->insertMedia( item->url(), Playlist::Replace );
        #undef  item
    }
    else if( isPodcastChannel( item ) )
    {
        #define item static_cast<PodcastChannel *>(item)
        KURL::List list;
        QListViewItem *child = item->firstChild();
        while( child )
        {
            #define child static_cast<PodcastItem *>(child)

            child->hasDownloaded() ?
                list.append( child->localUrl() ):
                list.append( child->url()      );

            child->setNew( false );

            #undef child
            child = child->nextSibling();
        }

        Playlist::instance()->insertMedia( list, Playlist::Replace );
        item->setNew( false );

        #undef item
    }
    else if( isPodcastItem( item ) )
    {
        #define item static_cast<PodcastItem *>(item)
        KURL::List list;

        item->hasDownloaded() ?
            list.append( item->localUrl() ):
            list.append( item->url()      );

        Playlist::instance()->insertMedia( list, Playlist::DirectPlay );
        item->setNew( false );

        #undef item
    }
    else if( isStream( item ) )
    {
        Playlist::instance()->insertMedia( static_cast<StreamEntry *>(item)->url(), Playlist::Replace );
    }
    else if( isSmartPlaylist( item ) )
    {
        #define item static_cast<SmartPlaylist *>(item)
        if( !item->query().isEmpty() )
            Playlist::instance()->insertMediaSql( item->query(), Playlist::Clear );
        #undef  item
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
    else if( isDynamic( item ) )
    {
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "dynamic_mode" ))->setChecked( true );
        Party::instance()->loadConfig( static_cast<PartyEntry *>(item) );
        loadDynamicItems();
        Playlist::instance()->repopulate();
    }
    else
        debug() << "No functionality for item double click implemented" << endl;
}

void PlaylistBrowser::collectionScanDone()
{
    if( !m_polished || CollectionDB::instance()->isEmpty() )
    {
        return;
    }
    else if( !m_smartCategory )
    {
        m_smartCategory = loadSmartPlaylists();
        loadDefaultSmartPlaylists();
        m_smartCategory->setOpen( true );
    }
}

void PlaylistBrowser::addToDynamic()
{
    QListViewItemIterator it( m_listview, QListViewItemIterator::Selected );

    for( ; it.current(); ++it )
    {
        if( m_dynamicEntries.find( *it ) < 0 ) // check that it is not there
        {
            m_dynamicEntries.append( *it );
            if( isPlaylist( *it ) )
            {
                PlaylistEntry *item = static_cast<PlaylistEntry*>(*it);
                item->setDynamic( true );
            }
            else if( isSmartPlaylist( *it ) )
            {
                SmartPlaylist *item = static_cast<SmartPlaylist*>(*it);
                item->setDynamic( true );
            }
        }
    }
}

void PlaylistBrowser::subFromDynamic()
{
    QListViewItemIterator it( m_listview, QListViewItemIterator::Selected );

    for( ; it.current(); ++it )
    {
        if( m_dynamicEntries.find( *it ) >= 0 ) // check if it is already present
        {
            m_dynamicEntries.remove( *it );
            if( isPlaylist( *it ) )
            {
                PlaylistEntry *item = static_cast<PlaylistEntry*>(*it);
                item->setDynamic( false );
            }
            else if( isSmartPlaylist( *it ) )
            {
                SmartPlaylist *item = static_cast<SmartPlaylist*>(*it);
                item->setDynamic( false );
            }
        }
    }
}

void PlaylistBrowser::removeSelectedItems() //SLOT
{
    // this function remove selected playlists and tracks

    //remove currentItem, no matter if selected or not
    m_listview->setSelected( m_listview->currentItem(), true );

    QPtrList<QListViewItem> selected;
    QListViewItemIterator it( m_listview, QListViewItemIterator::Selected);
    for( ; it.current(); ++it )
    {
        if( (*it) == m_coolStreams || (*it) == m_smartDefaults )
            continue;
        // if the playlist containing this item is already selected the current item will be skipped
        // it will be deleted from the parent
        QListViewItem *parent = it.current()->parent();

        if( isCategory( *it ) && !static_cast<PlaylistCategory*>(*it)->isFolder() ) //its a base category
            continue;

        if( parent && parent->isSelected() ) //parent will remove children
            continue;

        while( parent->parent() && parent != m_coolStreams && parent != m_smartDefaults )
            parent = parent->parent();

        if( parent == m_coolStreams || parent == m_smartDefaults )
            continue;

        selected.append( it.current() );
    }

    QPtrList<PlaylistEntry> playlistsToDelete;
    QPtrList<PlaylistCategory> playlistFoldersToDelete;
    QPtrList<PodcastChannel> podcastsToDelete;
    QPtrList<PlaylistCategory> podcastFoldersToDelete;
    QPtrList<PlaylistTrackItem> tracksToDelete;

    bool playlistsChanged = false;
    bool streamsChanged = false;
    bool smartPlaylistsChanged = false;
    bool dynamicsChanged = false;
    bool podcastsChanged = false;

    /// @note the variable keepItem is used to tell us if there is a more sinister operation which is needed,
    /// being deleting from disk - this includes deleting playlists and deleting all downloaded podcast media.
    for( QListViewItem *item = selected.first(); item; item = selected.next() )
    {
        bool keepItem = false;

        if( isPlaylist( item ) )
        {
            keepItem = playlistsChanged = true;
            playlistsToDelete.append( static_cast<PlaylistEntry*>(item) );
        }

        /// @note when we delete a category, we have to check if it contains either playlists or podcasts which
        /// need deleting to occur.  We set keepItem = true to delete the folder after we have deleted the
        /// playlists/podcasts, and only then do we remove the folder.
        else if( isCategory( item ) )
        {
            if( isPlaylist( item->firstChild() ))
            {
                for( QListViewItem *ch = item->firstChild(); ch; ch = ch->nextSibling() )
                {
                    keepItem = playlistsChanged = true;
                    playlistsToDelete.append( static_cast<PlaylistEntry*>(ch) );
                    playlistFoldersToDelete.append( static_cast<PlaylistCategory*>(item) );
                }
            }
            if( isPodcastChannel( item->firstChild() ) )
            {
                for( QListViewItem *ch = item->firstChild(); ch; ch = ch->nextSibling() )
                {
                    keepItem = podcastsChanged = true;
                    podcastsToDelete.append( static_cast<PodcastChannel*>(ch) );
                    podcastFoldersToDelete.append( static_cast<PlaylistCategory*>(item) );
                }
            }
        }
        else if( isStream( item ) )        streamsChanged = true;
        else if( isSmartPlaylist( item ) ) smartPlaylistsChanged = true;
        else if( isDynamic( item ) )       dynamicsChanged = true;

        if( isPlaylistTrackItem( item ) )
        {
            //we are going to be deleting the parent playlist, dont bother removing it
            if( playlistsToDelete.find( static_cast<PlaylistEntry*>(item->parent()) ) != -1 )
                continue;

            playlistsChanged = true;
            //remove the track
            PlaylistEntry *playlist = static_cast<PlaylistEntry*>(item->parent());
            playlist->removeTrack( item );
        }
        else if( isPodcastChannel( item ) )
        {
            //we are going to be deleting the parent podcast, dont bother removing it
            if( podcastsToDelete.find( static_cast<PodcastChannel*>(item->parent()) ) != -1 )
                continue;

            podcastsChanged = true;
            m_podcastItemsToScan.remove( static_cast<PodcastChannel*>(item) );
            podcastsToDelete.append( static_cast<PodcastChannel*>(item) );
        }
        else if( !keepItem )
        {
            m_dynamicEntries.remove(item); // if it's not there, no problem, it just returns false.
            delete item;
        }
    }

    if( streamsChanged )        saveStreams();
    if( smartPlaylistsChanged ) saveSmartPlaylists();
    if( dynamicsChanged )       saveDynamics();
    if( podcastsChanged )       savePodcasts();

    // used for deleting playlists first, then folders.
    if( playlistsChanged )
    {
        if( deletePlaylists( playlistsToDelete ) )
        {
            foreachType( QPtrList<PlaylistEntry>, playlistsToDelete )
            {
                m_dynamicEntries.remove(*it);
                delete (*it);
            }

            foreachType( QPtrList<PlaylistCategory>, playlistFoldersToDelete )
            {
                delete (*it);
            }
            savePlaylists();
        }
    }

    if( podcastsChanged )
    {
        if( deletePodcasts( podcastsToDelete ) )
        {
            foreachType( QPtrList<PodcastChannel>, podcastsToDelete )
            {
                delete (*it);
            }
            foreachType( QPtrList<PlaylistCategory>, podcastFoldersToDelete )
            {
                delete (*it);
            }
            savePodcasts();
        }
    }
}

void PlaylistBrowser::renameSelectedItem() //SLOT
{
    QListViewItem *item = m_listview->currentItem();
    if( !item ) return;

    if( isCategory( item ) && static_cast<PlaylistCategory*>(item)->isFolder() )
    {
        if( item == m_coolStreams || item == m_smartDefaults )
            return;

        item->setRenameEnabled( 0, true );
        m_listview->rename( item, 0 );
    }

    else if( isPlaylist( item ) || isStream( item ) || isSmartPlaylist( item ) || isDynamic( item ) )
    {
        QListViewItem *parent = item->parent();

        while( parent )
        {
            if( parent == m_coolStreams || parent == m_smartDefaults )
                return;

            if( !parent->parent() )
                break;

            parent = parent->parent();
        }

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
        QString newPath = fileDirPath( oldPath ) + newName + "." + fileExtension( oldPath );

        if ( std::rename( QFile::encodeName( oldPath ), QFile::encodeName( newPath ) ) == -1 )
            KMessageBox::error( this, i18n("Error renaming the file.") );
        else
            item->setUrl( newPath );

        #undef item
    }

    item->setRenameEnabled( 0, false );
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
        stream << "NumberOfEntries=" << trackList.count() << endl;
        int c=1;
        for( TrackItemInfo *info = trackList.first(); info; info = trackList.next(), ++c )
        {
            stream << "File" << c << "=";
            stream << (info->url().protocol() == "file" ? info->url().path() : info->url().url());
            stream << "\nTitle" << c << "=";
            stream << info->title();
            stream << "\nLength" << c << "=";
            stream << info->length();
            stream << "\n";
         }

        stream << "Version=2\n";
        file.close();
    }
}

#include <kdirlister.h>
#include <qeventloop.h>
#include "playlistloader.h"
//this function (C) Copyright 2003-4 Max Howell, (C) Copyright 2004 Mark Kretschmann
KURL::List PlaylistBrowser::recurse( const KURL &url )
{
    typedef QMap<QString, KURL> FileMap;

    KDirLister lister( false );
    lister.setAutoUpdate( false );
    lister.setAutoErrorHandlingEnabled( false, 0 );
    lister.openURL( url );

    while( !lister.isFinished() )
        kapp->eventLoop()->processEvents( QEventLoop::ExcludeUserInput );

    KFileItemList items = lister.items(); //returns QPtrList, so we MUST only do it once!
    KURL::List urls;
    FileMap files;
    for( KFileItem *item = items.first(); item; item = items.next() ) {
        if( item->isFile() ) { files[item->name()] = item->url(); continue; }
        if( item->isDir() ) urls += recurse( item->url() );
    }

    foreachType( FileMap, files )
        // users often have playlist files that reflect directories
        // higher up, or stuff in this directory. Don't add them as
        // it produces double entries
        if( !PlaylistFile::isPlaylistFile( (*it).fileName() ) )
            urls += *it;

    return urls;
}


void PlaylistBrowser::currentItemChanged( QListViewItem *item )    //SLOT
{
    // rename remove and delete buttons are disabled if there are no playlists
    // rename and delete buttons are disabled for track items

    bool enable_remove = false;
    bool enable_rename = false;

    if( !item )
        goto enable_buttons;

    else if( isPlaylist( item ) )
    {
        enable_remove = true;
        enable_rename = true;
    }
    else if( isStream( item ) )
    {
        enable_remove = ( item->parent() != m_coolStreams );
        enable_rename = ( item->parent() != m_coolStreams );
    }
    else if( isSmartPlaylist( item ) )
    {
        QListViewItem *parent = item->parent();

        while( parent != m_smartCategory && parent != m_smartDefaults )
            parent = parent->parent();

        enable_remove = ( parent != m_smartDefaults );
        enable_rename = ( parent != m_smartDefaults );
    }
    else if( isDynamic( item ) )
    {
        enable_remove = true;
        enable_rename = true;
    }
    else if( isCategory( item ) )
    {
        if( static_cast<PlaylistCategory*>(item)->isFolder() )
        {
            if( item != m_coolStreams && item != m_smartDefaults ) {
                enable_remove = true;
                enable_rename = true;
            }
        }
    }
    else
        enable_remove = true;


    enable_buttons:

    removeButton->setEnabled( enable_remove );
    renameButton->setEnabled( enable_rename );
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

        case PODCAST:
            addPodcast();
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
            createPlaylist();
            break;

        case DYNAMIC:
            addDynamic();
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
        enum Id { LOAD, ADD, DYNADD, DYNSUB, SAVE, RESTORE, RENAME, DELETE };

        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );

        if( isDynamicEnabled() && AmarokConfig::dynamicType() == "Custom" )
        {
            if( static_cast<PlaylistEntry*>(item)->isDynamic() )
                menu.insertItem( SmallIconSet( "edit_remove" ), i18n( "Remove From Dynamic Mode" ), DYNSUB );
            else
                menu.insertItem( SmallIconSet( "edit_add" ), i18n( "Add to Dynamic Mode" ), DYNADD );
        }

        menu.insertSeparator();
        if( item->isModified() )
        {
            menu.insertItem( SmallIconSet("filesave"), i18n( "&Save" ), SAVE );
            menu.insertItem( i18n( "Res&tore" ), RESTORE );
            menu.insertSeparator();
        }
        menu.insertItem( SmallIconSet("editclear"), i18n( "&Rename" ), RENAME );
        menu.insertItem( SmallIconSet("editdelete"), i18n( "&Delete" ), DELETE );
        menu.setAccel( Key_Space, LOAD );
        menu.setAccel( Key_F2, RENAME );
        menu.setAccel( SHIFT+Key_Delete, DELETE );

        switch( menu.exec( p ) )
        {
            case LOAD:
                slotDoubleClicked( item );
                break;
            case ADD:
                Playlist::instance()->insertMedia( item->url() );
                break;
            case DYNADD:
                addToDynamic();
                break;
            case DYNSUB:
                subFromDynamic();
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
            case DELETE:
                removeSelectedItems();
                break;
        }
        #undef item
    }
    else if( isSmartPlaylist( item ) )
    {
        enum Actions { LOAD, ADD, DYNADD, DYNSUB, EDIT, REMOVE };

        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );

        if( isDynamicEnabled() && AmarokConfig::dynamicType() == "Custom" )
        {
            if( static_cast<SmartPlaylist*>(item)->isDynamic() )
                menu.insertItem( SmallIconSet( "edit_remove" ), i18n( "Remove From Dynamic Mode" ), DYNSUB );
            else
                menu.insertItem( SmallIconSet( "edit_add" ), i18n( "Add to Dynamic Mode" ), DYNADD );
        }

        menu.insertSeparator();
        // Forbid removal of Collection
        if( item->parent()->text(0) != i18n("Collection") )
        {
            if ( static_cast<SmartPlaylist *>(item)->isEditable() )
                menu.insertItem( SmallIconSet("editclear"), i18n( "E&dit..." ), EDIT );
            menu.insertItem( SmallIconSet("edittrash"), i18n( "R&emove" ), REMOVE );
        }

        switch( menu.exec( p ) )
        {
            case LOAD:
                slotDoubleClicked( item );
                break;
            case ADD:
                Playlist::instance()->insertMediaSql( static_cast<SmartPlaylist *>(item)->query(), Playlist::Append );
                break;
            case DYNADD:
                addToDynamic();
                break;
            case DYNSUB:
                subFromDynamic();
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
        if( item->parent() != m_coolStreams )
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
    else if( isDynamic( item ) ) {
        #define item static_cast<PartyEntry*>(item)
        enum Actions { LOAD, RENAME, REMOVE };
        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertSeparator();
        menu.insertItem( SmallIconSet("editclear"), i18n( "&Rename" ), RENAME );
        menu.insertItem( SmallIconSet("edittrash"), i18n( "R&emove" ), REMOVE );

        switch( menu.exec( p ) )
        {
            case LOAD:
                slotDoubleClicked( item );
                break;
            case RENAME:
                renameSelectedItem();
                break;
            case REMOVE:
                removeSelectedItems();
                break;
        }
        #undef item
    }
    else if( isPodcastChannel( item ) ) {
        #define item static_cast<PodcastChannel*>(item)
        enum Actions { LOAD, ADD, DELETE, INFO, RESCAN, CONFIG};
        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );
        menu.insertItem( SmallIconSet( "editdelete" ), i18n( "&Delete" ), DELETE );
        menu.insertSeparator();
        menu.insertItem( SmallIconSet( "info" ), i18n( "Show &Information" ), INFO );
        menu.insertItem( SmallIconSet( "reload" ), i18n( "&Check for Updates" ), RESCAN );
        menu.insertItem( SmallIconSet( "configure" ), i18n( "&Configure..." ), CONFIG );


        switch( menu.exec( p ) )
        {
            case LOAD:
                slotDoubleClicked( item );
                break;

            case ADD:
            {
                KURL::List list;
                QListViewItem *child = item->firstChild();
                while( child )
                {
                    list.append( static_cast<PodcastItem*>( child )->url() );
                    child = child->nextSibling();
                }
                Playlist::instance()->insertMedia( list );
                break;
            }

            case INFO:
                item->showAbout();
                break;

            case RESCAN:
                item->rescan();
                break;

            case DELETE:
                removeSelectedItems();
                break;

            case CONFIG:
                item->configure();

                if( item->autoScan() && m_podcastItemsToScan.find( item ) < 0 ) // check that it is not there
                {
                    m_podcastItemsToScan.append( item );
                }
                else if( !item->autoScan() && m_podcastItemsToScan.find( item ) >= 0 )
                {
                    m_podcastItemsToScan.remove( item );
                }

                if( m_podcastItemsToScan.isEmpty() )
                    m_podcastTimer->stop();
                else if( m_podcastItemsToScan.count() == 1 )
                    m_podcastTimer->start( m_podcastTimerInterval );
                // else timer is already running

                break;
        }
        #undef item
    }
    else if( isPodcastItem( item ) ) {
        #define item static_cast<PodcastItem*>(item)
        enum Actions { LOAD, QUEUE, INFO, GET, MEDIA_DEVICE };
        menu.insertItem( SmallIconSet( "player_play" ), i18n( "&Play" ), LOAD );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue" ), QUEUE );
        menu.insertSeparator();
        menu.insertItem( SmallIconSet( "info" ), i18n( "Show &Information" ), INFO );
        menu.insertItem( SmallIconSet( "down" ), i18n( "&Download Media" ), GET );

        menu.setItemEnabled( GET, !item->hasDownloaded() );

        if( MediaDevice::instance()->isConnected() )
        {
            menu.insertItem( SmallIconSet( "usbpendrive_unmount" ), i18n( "Add to Media Device &Transfer Queue" ), MEDIA_DEVICE );
            menu.setItemEnabled( MEDIA_DEVICE, item->hasDownloaded() );
        }

        switch( menu.exec( p ) )
        {
            case LOAD:
                slotDoubleClicked( item );
                break;

            case QUEUE:
                if( item->hasDownloaded() )
                    Playlist::instance()->insertMedia( item->localUrl(), Playlist::Queue );
                else
                    Playlist::instance()->insertMedia( item->url(), Playlist::Queue );
                break;

            case INFO:
                item->showAbout();
                break;

            case GET:
                downloadSelectedPodcasts();
                break;

            case MEDIA_DEVICE:
                MediaDevice::instance()->addURLs( item->localUrl() );
                break;
       }
        #undef item
    }
    else if( isCategory( item ) ) {
        #define item static_cast<PlaylistCategory*>(item)
        enum Actions { RENAME, REMOVE, CREATE, PLAYLIST, SMART, STREAM, DYNAMIC, PODCAST, REFRESH, INTERVAL };

        QListViewItem *parentCat = item;

        while( parentCat->parent() )
            parentCat = parentCat->parent();

        if( item == m_coolStreams || item == m_smartDefaults ) return;

        if( item->isFolder() ) {
            menu.insertItem( SmallIconSet("editclear"), i18n( "&Rename" ), RENAME );
            menu.insertItem( SmallIconSet("edittrash"), i18n( "R&emove" ), REMOVE );
            menu.insertSeparator();
        }

        if( parentCat == static_cast<QListViewItem*>(m_playlistCategory) )
            menu.insertItem( SmallIconSet("edit_add"), i18n("Add Playlist..."), PLAYLIST );

        else if( parentCat == static_cast<QListViewItem*>(m_smartCategory) )
            menu.insertItem( SmallIconSet("edit_add"), i18n("Add Smart-Playlist..."), SMART );

        else if( parentCat == static_cast<QListViewItem*>(m_streamsCategory) )
            menu.insertItem( SmallIconSet("edit_add"), i18n("Add Radio Stream..."), STREAM );

        else if( parentCat == static_cast<QListViewItem*>(m_dynamicCategory) )
            menu.insertItem( SmallIconSet("edit_add"), i18n("Save Dynamic Configuration..."), DYNAMIC );

        else if( parentCat == static_cast<QListViewItem*>(m_podcastCategory) )
        {
            menu.insertItem( SmallIconSet("reload"), i18n("Refresh All Podcasts"), REFRESH );
            menu.insertItem( SmallIconSet("edit_add"), i18n("Add Podcast..."), PODCAST );
            if( parentCat == item )
                menu.insertItem( SmallIconSet("tool_timer"), i18n("Scan Interval..."), INTERVAL );
        }

        menu.insertSeparator();
        menu.insertItem( SmallIconSet("folder"), i18n("Create Sub-Folder"), CREATE );

        QListViewItem *tracker = 0;
        int c;
        QString name;

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

            case DYNAMIC:
                addDynamic( item );
                break;

            case PODCAST:
                addPodcast( item );
                break;

            case REFRESH:
                refreshPodcasts(item);
                break;

            case CREATE:
                tracker = item->firstChild();

                for( c = 0 ; isCategory( tracker ); tracker = tracker->nextSibling() )
                {
                    if( tracker->text(0).startsWith( i18n("Folder") ) )
                        c++;
                    if( !isCategory( tracker->nextSibling() ) )
                        break;
                }
                name = i18n("Folder");
                if( c ) name = i18n("Folder %1").arg(c);
                if( tracker == item->firstChild() && !isCategory( tracker ) ) tracker = 0;

               (new PlaylistCategory( item, tracker, name, true ))->startRename( 0 );

                break;

            case INTERVAL:
                changePodcastInterval();
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
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue Track" ), QUEUE );
        menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );


        menu.insertSeparator();

        menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn to CD as Data"), BURN_DATACD );
        menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() && item->url().isLocalFile() );
        menu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("Burn to CD as Audio"), BURN_AUDIOCD );
        menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() && item->url().isLocalFile() );

        menu.insertSeparator();

        menu.insertItem( SmallIconSet("edittrash"), i18n( "&Remove" ), REMOVE );
        menu.insertItem( SmallIconSet("info"), i18n( "Edit Track &Information..." ), INFO );

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
                if( !item->url().isLocalFile() )
                    KMessageBox::sorry( this, i18n( "Track information is not available for remote media." ) );
                else if( QFile::exists( item->url().path() ) ) {
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
    , m_animationTimer( new QTimer() )
    , m_loading1( new QPixmap( locate("data", "amarok/images/loading1.png" ) ) )
    , m_loading2( new QPixmap( locate("data", "amarok/images/loading2.png" ) ) )
{
    addColumn( i18n("Playlists") );

    setSelectionMode( QListView::Extended );
    setResizeMode( QListView::AllColumns );
    setShowSortIndicator( true );
    setRootIsDecorated( true );

    setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    setDropHighlighter( true );   //and the highligther (a focus rect) is drawn when dragging over playlists
    setDropVisualizerWidth( 3 );
    setAcceptDrops( true );

    setTreeStepSize( 20 );

    connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );

    connect( this, SIGNAL( mouseButtonPressed ( int, QListViewItem *, const QPoint &, int ) ),
             this,   SLOT( mousePressed( int, QListViewItem *, const QPoint &, int ) ) );

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
    e->accept( e->source() == viewport() || KURLDrag::canDecode( e ) );
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

    if( e->source() == this )
    {
        moveSelectedItems( item ); // D&D sucks, do it ourselves
    }
    else {
        KURL::List list;
        QMap<QString, QString> map;
        if( KURLDrag::decode( e, list, map ) ) {
            if( parent && isPlaylist( parent ) ) {
                //insert the dropped tracks
                PlaylistEntry *playlist = (PlaylistEntry *)parent;
                playlist->insertTracks( after, list, map );
            }
            else //dropped on a playlist item
            {
                QListViewItem *parent = item;

                bool isPlaylistFolder = false;
                while( parent )
                {
                    if( parent == PlaylistBrowser::instance()->m_playlistCategory )
                    {
                        isPlaylistFolder = true;
                        break;
                    }
                    parent = parent->parent();
                }

                if( isPlaylist( item ) ) {
                    PlaylistEntry *playlist = (PlaylistEntry *)item;
                    //append the dropped tracks
                    playlist->insertTracks( 0, list, map );
                }
                else if( isCategory( item ) && isPlaylistFolder )
                {
                    PlaylistBrowser *pb = PlaylistBrowser::instance();
                    if ( pb->createPlaylist( item, false ) )
                        pb->m_lastPlaylist->insertTracks( 0, list, map );
                }
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

void PlaylistBrowserView::moveSelectedItems( QListViewItem *newParent )
{
    if( !newParent || isDynamic( newParent ) || isPodcastChannel( newParent ) ||
         isSmartPlaylist( newParent ) || isPodcastItem( newParent ) )
        return;

    if( newParent == PlaylistBrowser::instance()->m_coolStreams ||
        newParent == PlaylistBrowser::instance()->m_smartDefaults )
        return;

    QPtrList<QListViewItem> selected;
    QListViewItemIterator it( this, QListViewItemIterator::Selected );
    for( ; it.current(); ++it )
    {
        if( !(*it)->parent() ) //must be a base category we are draggin'
            continue;

        selected.append( *it );
    }

    for( QListViewItem *item = selected.first(); item; item = selected.next() )
    {
        QListViewItem *itemParent = item->parent();
        if( isPlaylistTrackItem( item ) )
        {
            if( !isPlaylist( newParent ) )
                continue;
            else
            {
                itemParent->takeItem( item );
                newParent->insertItem( item );
            }
        }
        else if( !isCategory( newParent ) )
            continue;

        QListViewItem *base = newParent;
        while( base->parent() )
            base = base->parent();

        if( base == PlaylistBrowser::instance()->m_playlistCategory && isPlaylist( item )   ||
            base == PlaylistBrowser::instance()->m_streamsCategory && isStream( item )      ||
            base == PlaylistBrowser::instance()->m_smartCategory && isSmartPlaylist( item ) ||
            base == PlaylistBrowser::instance()->m_dynamicCategory && isDynamic( item )     ||
            base == PlaylistBrowser::instance()->m_podcastCategory && isPodcastChannel( item ) )
        {
            itemParent->takeItem( item );
            newParent->insertItem( item );
            newParent->sortChildItems( 0, true );
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

        case SHIFT+Key_Delete:    //delete
        case Key_Delete:          //remove
            PlaylistBrowser::instance()->removeSelectedItems();
            break;

        case Key_F2:    //rename
            PlaylistBrowser::instance()->renameSelectedItem();
            break;

        default:
            KListView::keyPressEvent( e );
            break;
    }
}


void PlaylistBrowserView::startDrag()
{
    KURL::List urls;

    KMultipleDrag *drag = new KMultipleDrag( this );

    QListViewItemIterator it( this, QListViewItemIterator::Selected );

    for( ; it.current(); ++it )
    {
        if( isPlaylist( *it ) )
            urls += ((PlaylistEntry*)*it)->url();

        else if( isStream( *it ) )
            urls += ((StreamEntry*)*it)->url();

        else if( isPodcastItem( *it ) )
        {
            #define item static_cast<PodcastItem *>(*it)
            if( item->hasDownloaded() )
                urls += item->localUrl();
            else
                urls += item->url();

            item->setNew( false );

            #undef item
        }
        else if( isPodcastChannel( *it ) )
        {
            #define item static_cast<PodcastChannel *>(*it)
            KURL::List list;
            QListViewItem *child = item->firstChild();
            while( child )
            {
                list.append( static_cast<PodcastItem*>( child )->url() );
                static_cast<PodcastItem*>( child )->setNew( false );
                child = child->nextSibling();
            }

            item->setNew( false );

            #undef item
        }

        else if( isSmartPlaylist( *it ) )
        {
            SmartPlaylist *item = (SmartPlaylist*)*it;

            if( !item->query().isEmpty() )
            {
                QTextDrag *textdrag = new QTextDrag( item->query(), 0 );
                textdrag->setSubtype( "amarok-sql" );
                drag->addDragObject( textdrag );
            }

        }
        else if( isPlaylistTrackItem( *it ) )
            urls += ((PlaylistTrackItem*)*it)->url();
    }

    drag->addDragObject( new KURLDrag( urls, viewport() ) );
    drag->dragCopy();

}

/////////////////////////////////////////////////////////////////////////////
//    CLASS PlaylistDialog
////////////////////////////////////////////////////////////////////////////

QString PlaylistDialog::getSaveFileName( const QString &suggestion ) //static
{
    PlaylistDialog dialog;
    if( !suggestion.isEmpty() )
        dialog.edit->setText( suggestion );
    if( dialog.exec() == Accepted )
        return dialog.result;
    return QString::null;
}

PlaylistDialog::PlaylistDialog()
    : KDialogBase( PlaylistWindow::self(), "saveplaylist", true /*modal*/,
                   i18n( "Save Playlist" ), Ok | Cancel | User1, Ok, false /*seperator*/,
                   KGuiItem( i18n( "Save to location..." ), SmallIconSet( "folder" ) ) )
    , customChosen( false )
{
    QVBox *vbox = makeVBoxMainWidget();
    QLabel *label = new QLabel( i18n( "&Enter a name for the playlist:" ), vbox );
    edit = new KLineEdit( vbox );
    edit->setFocus();
    label->setBuddy( edit );
    enableButtonOK( false );
    connect( edit, SIGNAL( textChanged( const QString & ) ),
             this, SLOT( slotTextChanged( const QString& ) ) );
    connect( this, SIGNAL( user1Clicked() ), SLOT( slotCustomPath() ) );
}

void PlaylistDialog::slotOk()
{
    // TODO Remove this hack for 1.2. It's needed because playlists was a file once.
    QString folder = KGlobal::dirs()->saveLocation( "data", "amarok/playlists", false );
    QFileInfo info( folder );
    if ( !info.isDir() ) QFile::remove( folder );

    if( !customChosen && !edit->text().isEmpty() )
        result = KGlobal::dirs()->saveLocation( "data", "amarok/playlists/", true ) + edit->text() + ".m3u";

    if( !QFileInfo( result ).exists() ||
        KMessageBox::warningContinueCancel(
            PlaylistWindow::self(),
            i18n( "A playlist named \"%1\" already exists. Do you want to overwrite it?" ).arg( edit->text() ),
            i18n( "Overwrite Playlist?" ), i18n( "Overwrite" ) ) == KMessageBox::Continue )
    {
        KDialogBase::slotOk();
    }
}

void PlaylistDialog::slotTextChanged( const QString &s )
{
    enableButtonOK( !s.isEmpty() );
}

void PlaylistDialog::slotCustomPath()
{
   result = KFileDialog::getSaveFileName( ":saveplaylists", "*.m3u" );
   if( !result.isNull() )
   {
      edit->setText( result );
      edit->setReadOnly( true );
      enableButtonOK( true );
      customChosen = true;
   }
}

#include "playlistbrowser.moc"
