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
#include "tagdialog.h"         //showContextMenu()
#include "threadweaver.h"

#include <qevent.h>            //customEvent()
#include <qheader.h>           //mousePressed()
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
#include <kpopupmenu.h>
#include <kstandarddirs.h>     //KGlobal::dirs()
#include <kurldrag.h>          //dragObject()

#include <stdio.h>             //rename() in renamePlaylist()

PlaylistBrowser *PlaylistBrowser::s_instance = 0;

static inline bool isDynamicEnabled() { return AmarokConfig::dynamicMode(); }

PlaylistBrowser::PlaylistBrowser( const char *name )
        : QVBox( 0, name )
{
    s_instance = this;

    QVBox *browserBox = new QVBox( this );
    browserBox->setSpacing( 3 );

    //<Toolbar>
    m_ac = new KActionCollection( this );

    addMenuButton  = new KActionMenu( i18n("Add"), "fileopen", m_ac );
    addMenuButton->setDelayed( false );

    KPopupMenu *addMenu  = addMenuButton->popupMenu();
    addMenu->insertItem( i18n("Playlist..."), PLAYLIST );
    addMenu->insertItem( i18n("Radio Stream..."), STREAM );
    addMenu->insertItem( i18n("Smart Playlist..."), SMARTPLAYLIST );
    connect( addMenu, SIGNAL( activated(int) ), SLOT( slotAddMenu(int) ) );

    saveMenuButton = new KActionMenu( i18n("Save"), "filesave", m_ac );
    saveMenuButton->setDelayed( false );

    KPopupMenu *saveMenu = saveMenuButton->popupMenu();
    saveMenu->insertItem( i18n("Current Playlist..."), CURRENT );
    saveMenu->insertItem( i18n("Dynamic Playlist..."), DYNAMIC );
    connect( saveMenu, SIGNAL( activated(int) ), SLOT( slotSaveMenu(int) ) );

    renameButton   = new KAction( i18n("Rename"), "editclear", 0, this, SLOT( renameSelectedItem() ), m_ac, "Rename" );
    removeButton   = new KAction( i18n("Remove"), "edittrash", 0, this, SLOT( removeSelectedItems() ), m_ac, "Remove" );

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

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistBrowser" );
    m_viewMode = (ViewMode)config->readNumEntry( "View", LISTVIEW );  //restore the view mode
    viewMenu->setItemChecked( m_viewMode, true );
    m_sortMode = config->readNumEntry( "Sorting", ASCENDING );
    slotViewMenu( m_sortMode );

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

    setMinimumWidth( m_toolbar->sizeHint().width() );

    m_smartDefaults = 0;
    m_coolStreams   = 0;

    m_playlistCategory = loadPlaylists();
    m_streamsCategory  = loadStreams();
    loadCoolStreams();

    if( !CollectionDB::instance()->isEmpty() ) {
        m_smartCategory = loadSmartPlaylists();
        loadDefaultSmartPlaylists();
        m_smartCategory->setOpen( true );
    }
    // must be loaded after streams
    m_dynamicCategory  = loadDynamics();

    m_playlistCategory->setOpen( true );
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

}


PlaylistBrowser::~PlaylistBrowser()
{
    savePlaylists();
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

    KConfig *config = kapp->config();

    config->setGroup( "PlaylistBrowser" );
    config->writeEntry( "View", m_viewMode );
    config->writeEntry( "Sorting", m_sortMode );
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


PlaylistCategory* PlaylistBrowser::loadStreams()
{
    QFile file( streamBrowserCache() );

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;
    QDomElement e;

    if( !file.open( IO_ReadOnly ) || !d.setContent( stream.read() ) )
    { /*Couldn't open the file or it had invalid content, so let's create an empty element*/
        return new PlaylistCategory(m_listview, m_playlistCategory , i18n("Radio Streams") );
    }
    else {
        e = d.namedItem( "category" ).toElement();
        if ( e.attribute("formatversion") =="1.1" ) {
            return new PlaylistCategory(m_listview, m_playlistCategory, e );
        }
        else { // Old unversioned format
            PlaylistCategory* p = new PlaylistCategory(m_listview, m_playlistCategory, i18n("Radio Streams") );
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
    QDomElement streamB = m_streamsCategory->xml();
    streamB.setAttribute( "product", "amaroK" );
    streamB.setAttribute( "version", APP_VERSION );
    streamB.setAttribute( "formatversion", "1.1" );
    doc.appendChild( streamB );

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
    if( CollectionDB::instance()->isEmpty() || !m_smartCategory )
        return;

    if( !parent ) parent = static_cast<QListViewItem*>(m_smartCategory);

    SmartPlaylistEditor dialog( i18n("Untitled"), this );
    if( dialog.exec() == QDialog::Accepted ) {
        new SmartPlaylist( parent, 0, dialog.result() );
        parent->setOpen( true );
    }
}

PlaylistCategory* PlaylistBrowser::loadSmartPlaylists()
{

    QFile file( smartplaylistBrowserCache() );
    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;
    QDomElement e;

    if( !file.open( IO_ReadOnly ) || !d.setContent( stream.read() ) )
    { /*Couldn't open the file or it had invalid content, so let's create an empty element*/
        return new PlaylistCategory(m_listview, m_streamsCategory , i18n("Smart Playlists") );
    }
   else {
        e = d.namedItem( "category" ).toElement();
        if ( e.attribute("formatversion") =="1.1" ) {
            return new PlaylistCategory(m_listview, m_streamsCategory, e );
        }
        else { // Old unversioned format
            PlaylistCategory* p = new PlaylistCategory(m_listview, m_streamsCategory , i18n("Smart Playlists") );
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
    item->setPixmap( 0, SmallIcon("kfm") );

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
        item->sqlForTags = dialog.query();
        item->setText(0, dialog.name());
    }
}

void PlaylistBrowser::saveSmartPlaylists()
{
    QFile file( smartplaylistBrowserCache() );

    // If the user hadn't set a collection, we didn't create the Smart Playlist Item
    if( !m_smartCategory || !file.open( IO_WriteOnly ) ) return;

    QDomDocument doc;
    QDomElement smartB = m_smartCategory->xml();
    smartB.setAttribute( "product", "amaroK" );
    smartB.setAttribute( "version", APP_VERSION );
    smartB.setAttribute( "formatversion", "1.1" );
    doc.appendChild( smartB );

    QTextStream smart( &file );
    smart.setEncoding( QTextStream::UnicodeUTF8 );
    smart << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    smart << doc.toString();
}

/**
 *************************************************************************
 *  PARTIES
 *************************************************************************
 **/

QString PlaylistBrowser::partyBrowserCache()
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
        after = m_streamsCategory;

    if( !file.open( IO_ReadOnly ) || !d.setContent( stream.read() ) )
    { /*Couldn't open the file or it had invalid content, so let's create an empty element*/
        return new PlaylistCategory( m_listview, after, i18n("Dynamic Playlists") );
    }
    else {
        e = d.namedItem( "category" ).toElement();
        if ( e.attribute("formatversion") =="1.1" ) {
            return new PlaylistCategory( m_listview, after , e );
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

    if( !file.open( IO_WriteOnly ) ) return;

    QDomDocument doc;
    QDomElement partyB = doc.createElement( "partybrowser" );
    partyB.setAttribute( "product", "amaroK" );
    partyB.setAttribute( "version", APP_VERSION );
    doc.appendChild( partyB );

    PlaylistCategory *currentCat=0;

    #define m_dynamicCategory static_cast<QListViewItem *>(m_dynamicCategory)

    QListViewItem *it = m_dynamicCategory->firstChild();
    for( int count = 0; count < m_dynamicCategory->childCount(); count++ )
    {
        QDomElement i;

        if( !isCategory( it ) )
            currentCat = static_cast<PlaylistCategory*>(it->parent() );

        if( isDynamic( it ) )
        {
            PartyEntry *item = (PartyEntry*)it;

            i = doc.createElement("party");
            i.setAttribute( "name", item->text(0) );

            QDomElement attr = doc.createElement( "cycleTracks" );
            QDomText t = doc.createTextNode( item->isCycled() ? "true" : "false" );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = doc.createElement( "markHistory" );
            t = doc.createTextNode( item->isMarked() ? "true" : "false" );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = doc.createElement( "upcoming" );
            t = doc.createTextNode( QString::number( item->upcoming() ) );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = doc.createElement( "previous" );
            t = doc.createTextNode( QString::number( item->previous() ) );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = doc.createElement( "appendCount" );
            t = doc.createTextNode( QString::number( item->appendCount() ) );
            attr.appendChild( t );
            i.appendChild( attr );

            attr = doc.createElement( "appendType" );
            t = doc.createTextNode( QString::number( item->appendType() ) );
            attr.appendChild( t );
            i.appendChild( attr );

            QString list;
            if( item->appendType() == 2 )
            {
                QStringList items = item->items();
                for( uint c = 0; c < items.count(); c++ )
                {
                    PlaylistBrowserEntry *saveMe = findItem( *(items.at(c)), 0 );
                    debug() << "Saving item (" << item->text(0) << ") with " << saveMe->text(0) << endl;
                    list.append( saveMe->text(0) );
                    if ( c < items.count()-1 )
                        list.append( ',' );
                }
            }

            attr = doc.createElement( "items" );
            t = doc.createTextNode( list );
            attr.appendChild( t );
            i.appendChild( attr );
        }

        partyB.appendChild( i );

        it = it->nextSibling();
    }

    #undef m_dynamicCategory

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << doc.toString();
}

void PlaylistBrowser::loadDynamicItems()
{
    debug() << "Removing dynamic entries" << endl;
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
        debug() << "Loading entries" << endl;
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
 *  PLAYLISTS
 *************************************************************************
 **/

QString PlaylistBrowser::playlistBrowserCache()
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
        return new PlaylistCategory(m_listview, 0 , "Playlists" );
    }
    else {
        e = d.namedItem( "category" ).toElement();
        if ( e.attribute("formatversion") =="1.1" ) {
            return new PlaylistCategory(m_listview, 0 , e );
        }
        else { // Old unversioned format
            PlaylistCategory* p = new PlaylistCategory(m_listview, 0 , "Playlists" );
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
        if( !m_playlistCategory || !m_playlistCategory->childCount() ) {    //first child
            removeButton->setEnabled( true );
            renameButton->setEnabled( true );
        }

        if( !parent ) parent = static_cast<QListViewItem*>(m_playlistCategory);

        KURL auxKURL;
        auxKURL.setPath(path);
        m_lastPlaylist = new PlaylistEntry( parent, 0, auxKURL );
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
    QDomElement playlistsB = m_playlistCategory->xml();
    playlistsB.setAttribute( "product", "amaroK" );
    playlistsB.setAttribute( "version", APP_VERSION );
    playlistsB.setAttribute( "formatversion", "1.1" );
    doc.appendChild( playlistsB );

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << doc.toString();
}

/**
 *************************************************************************
 *  General Methods
 *************************************************************************
 **/

PlaylistBrowserEntry *
PlaylistBrowser::findItem( QString &t, int c )
{
    return (PlaylistBrowserEntry *)m_listview->findItem( t, c, Qt::ExactMatch );
}

void PlaylistBrowser::createPlaylist( bool current )
{
    bool ok;
    QString name = KInputDialog::getText(i18n("Save Playlist"), i18n("Enter playlist name:"), i18n("Untitled"), &ok, this);

    if( ok )
    {
        // TODO Remove this hack for 1.2. It's needed because playlists was a file once.
        QString folder = KGlobal::dirs()->saveLocation( "data", "amarok/playlists", false );
        QFileInfo info( folder );
        if ( !info.isDir() ) QFile::remove( folder );

        QString path = KGlobal::dirs()->saveLocation( "data", "amarok/playlists/", true ) + name + ".m3u";
        debug() << "Saving Playlist to: " << path << endl;

        if( current )
        {
            if ( !Playlist::instance()->saveM3U( path ) ) {
                KMessageBox::sorry( this, i18n( "Cannot write playlist (%1).").arg(path) );
                return;
            }
            addPlaylist( path );
        }
        else
        {
            m_lastPlaylist = new PlaylistEntry( static_cast<QListViewItem*>(m_playlistCategory), 0, path );
        }
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
        #undef  item
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
        Party::instance()->loadConfig( static_cast<PartyEntry *>(item) );
    }
    else
        debug() << "No functionality for item double click implemented" << endl;
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
        if( isPlaylistTrackItem( item ) ) {
                //remove the track
                PlaylistEntry *playlist = (PlaylistEntry *)item->parent();
                playlist->removeTrack( item );
        }
        else
            delete item;
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
    else if( isPlaylist( item ) || isStream( item ) || isSmartPlaylist( item ) || isDynamic( item ) ) {
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
        enum Id { LOAD, ADD, DYNADD, DYNSUB, SAVE, RESTORE, RENAME, REMOVE, DELETE };

        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );

        if( isDynamicEnabled() )
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
        enum Actions { LOAD, ADD, DYNADD, DYNSUB, EDIT, REMOVE };

        menu.insertItem( SmallIconSet( "fileopen" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), ADD );

        if( isDynamicEnabled() )
        {
            if( static_cast<SmartPlaylist*>(item)->isDynamic() )
                menu.insertItem( SmallIconSet( "edit_remove" ), i18n( "Remove From Dynamic Mode" ), DYNSUB );
            else
                menu.insertItem( SmallIconSet( "edit_add" ), i18n( "Add to From Dynamic Mode" ), DYNADD );
        }

        menu.insertSeparator();
        // Forbid removal of Collection
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
                Playlist::instance()->insertMediaSql( static_cast<SmartPlaylist *>(item)->sqlForTags, Playlist::Append );
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
        #undef  item
    }
    else if( isCategory( item ) ) {
        #define item static_cast<PlaylistCategory*>(item)
        enum Actions { RENAME, REMOVE, CREATE, PLAYLIST, SMART, STREAM, FOLDER };

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
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue Track" ), QUEUE );
        menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );


        menu.insertSeparator();

        menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn to CD as Data"), BURN_DATACD );
        menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() && item->url().isLocalFile() );
        menu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("Burn to CD as Audio"), BURN_AUDIOCD );
        menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() && item->url().isLocalFile() );

        menu.insertSeparator();

        menu.insertItem( SmallIconSet("edittrash"), i18n( "&Remove" ), REMOVE );
        menu.insertItem( SmallIconSet("info"), i18n( "&View/Edit Meta Information..." ), INFO );

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
    setResizeMode( QListView::AllColumns );
    setShowSortIndicator( true );
    setRootIsDecorated( true );

    setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    setDropHighlighter( true );    //and the highligther (a focus rect) is drawn when dragging over playlists
    setDropVisualizerWidth( 3 );
    setAcceptDrops( true );

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
            if( parent && isPlaylist( parent ) ) {
                //insert the dropped tracks
                PlaylistEntry *playlist = (PlaylistEntry *)parent;
                playlist->insertTracks( after, list, map );
            }
            else //dropped on a playlist item
            {
                if( isPlaylist( item ) ) {
                    PlaylistEntry *playlist = (PlaylistEntry *)item;
                    //append the dropped tracks
                    playlist->insertTracks( 0, list, map );
                }
                else if( isCategory( item ) &&
                         item == PlaylistBrowser::instance()->m_playlistCategory )
                {
                    PlaylistBrowser *pb = PlaylistBrowser::instance();
                    pb->createPlaylist( false );
                    pb->m_lastPlaylist->insertTracks( 0, list, map );
//                     pb->savePlaylist( pb->m_lastPlaylist );
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
            SmartPlaylist *item = (SmartPlaylist*)*it;
            QString query = item->sqlForTags;
            if( !query.isEmpty() )
            {
                QStringList list = CollectionDB::instance()->query( query );

                for( uint c=10; c < list.count(); c += 11 )
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
