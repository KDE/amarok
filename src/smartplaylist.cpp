// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#include "collectionbrowser.h"
#include "collectiondb.h"
#include "playlist.h"
#include "smartplaylist.h"
#include "smartplaylisteditor.h"

#include <qheader.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>    //loadCustomPlaylists()

#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>    //customPlaylistsFile()
#include <kguiitem.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>    //KGlobal::dirs()
#include <kurldrag.h>         //dragObject()
#include <ktoolbar.h>


SmartPlaylistBox::SmartPlaylistBox( QWidget *parent, const char *name )
    : QVBox( parent, name )
{
    SmartPlaylistView *smartListView = new SmartPlaylistView( this );

    KActionCollection *ac = new KActionCollection( this );
    KAction *createSmartPlayist = new KAction( i18n("Create Smart-playlist"), "filenew", 0, smartListView, SLOT( createCustomPlaylist() ), ac, "Create Smart-playlist" );
    KAction *remove = new KAction( i18n("Remove"), "edittrash", 0, smartListView, SLOT( removeSelectedPlaylists() ), ac, "Remove" );

    KToolBar *toolbar = new KToolBar( this );
    toolbar->setMovingEnabled(false);
    toolbar->setFlat(true);
    toolbar->setIconSize( 16 );
    toolbar->setEnableContextMenu( false );

    toolbar->setIconText( KToolBar::IconTextRight, false ); //we want the "create smart-playlist" button to have text on right
    createSmartPlayist->plug( toolbar );
    toolbar->insertLineSeparator();
    toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance
    remove->plug( toolbar);
}



/////////////////////////////////////////////////////////////////////////////
//    CLASS SmartPlaylistView
////////////////////////////////////////////////////////////////////////////

SmartPlaylistView::SmartPlaylistView( QWidget *parent, const char *name )
   : KListView( parent, name )
   , m_loaded( 0 )
{
    addColumn( i18n("Smart-playlists") );
    setSelectionMode(QListView::Extended);
    setSorting( 0 ); //enable sorting (used for custom smart playlists)
    setFullWidth( true );
    setRootIsDecorated( true );
    setShowSortIndicator( true );

    if( !CollectionDB().isEmpty() ) {
        loadDefaultPlaylists();
        loadCustomPlaylists();
        m_loaded = true;
    }

    if ( CollectionView::instance() )
        connect( CollectionView::instance(), SIGNAL( sigScanDone() ), SLOT( collectionScanDone() ) );

    connect( this, SIGNAL( doubleClicked(QListViewItem*) ), SLOT( loadPlaylistSlot(QListViewItem*) ) );
    connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
                     SLOT( showContextMenu( QListViewItem *, const QPoint &, int ) ) );
}


SmartPlaylistView::~SmartPlaylistView()
{
    //save all custom smart playlists

    QFile file( customPlaylistsFile() );

    if( file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        QListViewItemIterator it( this );
        while( it.current() ) {
            SmartPlaylist *item = static_cast<SmartPlaylist*>(*it);
            if( item->isCustom() ) {  //save only custom playlists
                stream << "Name=" + item->text(0);
                stream << "\n";
                stream << item->query();
                stream << "\n";
            }
            ++it;
        }

        file.close();
    }
}


void SmartPlaylistView::createCustomPlaylist() //SLOT
{
    //open a dialog to create a custom smart playlist

    if( CollectionDB().isEmpty() )
        return;

    int counter = 1;
    QListViewItemIterator it( this );
    for( ; it.current(); ++it ) {
        if( (*it)->text(0).startsWith( i18n("Untitled") ) )
            counter++;
    }

    SmartPlaylistEditor *editor = new SmartPlaylistEditor( this, QString("Untitled %1").arg(counter) );
    SmartPlaylistEditor::Result r = editor->exec();
    if( r.result == QDialog::Accepted )
        new SmartPlaylist( this, 0, r.playlistName, r.query, QString::null, true );
}


void SmartPlaylistView::removeSelectedPlaylists()
{
    QPtrList<QListViewItem> selected = selectedItems();
    for( QListViewItem *item = selected.first(); item; item = selected.next() )
        if( static_cast<SmartPlaylist*>(item )->isCustom() )
            delete item;
}


void SmartPlaylistView::loadDefaultPlaylists()
{
    CollectionDB *db = new CollectionDB();
    QStringList artistList = db->artistList();

    /********** All Collection **************/
    QString query = "SELECT tags.url "
                    "FROM tags, artist "
                    "WHERE tags.artist = artist.id "
                    "ORDER BY artist.name, tags.title;";
    SmartPlaylist *item = new SmartPlaylist( this, 0, i18n("All Collection"), query, "kfm" );
    item->setKey( 1 );

    /********** Favorite Tracks **************/
    query = "SELECT tags.url "
            "FROM tags, statistics "
            "WHERE statistics.url = tags.url "
            "ORDER BY statistics.percentage DESC "
            "LIMIT 0,15;";
    item = new SmartPlaylist( this, 0, i18n("Favorite Tracks"), query );
    item->setKey( 2 );
    SmartPlaylist *childItem = 0;
    for( uint i=0; i < artistList.count(); i++ ) {
        query = QString( "SELECT tags.url "
                         "FROM tags, artist, statistics "
                         "WHERE statistics.url = tags.url AND tags.artist = artist.id AND artist.name = '%1' "
                         "ORDER BY statistics.percentage DESC "
                         "LIMIT 0,15;" ).arg( db->escapeString( artistList[i] ) );
        childItem = new SmartPlaylist( item, childItem, i18n("By ") + artistList[i], query );
    }

    /********** Most Played **************/
    query = "SELECT tags.url "
            "FROM tags, statistics "
            "WHERE statistics.url = tags.url "
            "ORDER BY statistics.playcounter DESC "
            "LIMIT 0,15;";
    item = new SmartPlaylist( this, 0, i18n("Most Played"), query );
    item->setKey( 3 );
    childItem = 0;
    for( uint i=0; i < artistList.count(); i++ ) {
        query = QString( "SELECT tags.url "
                         "FROM tags, artist, statistics "
                         "WHERE statistics.url = tags.url AND tags.artist = artist.id AND artist.name = '%1' "
                         "ORDER BY statistics.playcounter DESC "
                         "LIMIT 0,15;" ).arg( db->escapeString( artistList[i] ) );
        childItem = new SmartPlaylist( item, childItem, i18n("By ") + artistList[i], query );
    }

    /********** Newest Tracks **************/
    query = "SELECT url "
            "FROM tags "
            "ORDER BY tags.createdate DESC "
            "LIMIT 0,15;";
    item = new SmartPlaylist( this, 0, i18n("Newest Tracks"), query );
    item->setKey( 4 );
    childItem = 0;
    for( uint i=0; i < artistList.count(); i++ ) {
        query = QString( "SELECT tags.url "
                         "FROM tags, artist "
                         "WHERE tags.artist = artist.id AND artist.name = '%1' "
                         "ORDER BY tags.createdate DESC "
                         "LIMIT 0,15;" ).arg( db->escapeString( artistList[i] ) );
        childItem = new SmartPlaylist( item, childItem, i18n("By ") + artistList[i], query );
    }

    /********** Last Played **************/
    query = "SELECT url "
            "FROM statistics "
            "ORDER BY statistics.accessdate DESC "
            "LIMIT 0,15;";
    item = new SmartPlaylist( this, 0, i18n("Last Played"), query );
    item->setKey( 5 );

    /********** Never Played **************/
    query = "SELECT tags.url "
            "FROM tags, artist "
            "WHERE tags.url NOT IN(SELECT url FROM statistics) AND tags.artist = artist.id "
            "ORDER BY artist.name, tags.title;";
    item = new SmartPlaylist(this, 0, i18n("Never Played"), query );
    item->setKey( 6 );

    /********** Genres **************/
    item = new SmartPlaylist( this, 0, i18n("Genres") );
    item->setDragEnabled( false );
    item->setKey( 7 );

    QStringList values;
    QStringList names;

    db->execSql( "SELECT DISTINCT name "
                          "FROM genre;", &values, &names );

    childItem = 0;
    for( uint i=0; i < values.count(); i++ ) {
        query = QString("SELECT tags.url "
                        "FROM tags, artist "
                        "WHERE tags.genre = %1 AND tags.artist = artist.id "
                        "ORDER BY artist.name, tags.title;" ).arg( db->getValueID( "genre", values[i], false ) );
        childItem = new SmartPlaylist( item, childItem, values[i], query );
    }
    values.clear();
    names.clear();

    delete db;
}


void SmartPlaylistView::loadCustomPlaylists()
{
    QFile file( customPlaylistsFile() );

    if( file.open( IO_ReadOnly ) )
    {
        QTextStream stream( &file );
        QString str;
        QString name, query;

        while ( !( str = stream.readLine() ).isNull() ) {
            if ( str.startsWith( "Name=" ) )
                name = str.mid( 5 );
            else {
                query = str;
                new SmartPlaylist( this, 0, name, query, QString::null, true );
            }
        }
    }
}


KURL::List SmartPlaylistView::loadSmartPlaylist( QListViewItem *item )
{
    // this function load the smart playlist querying the collection database
    // and returns the list of tracks

    #define item static_cast<SmartPlaylist*>(item)

    QStringList values;
    QStringList names;
    KURL::List list;

    CollectionDB *db = new CollectionDB();
    db->execSql( item->query(), &values, &names );

    if ( !values.isEmpty() )
    {
        for ( uint i = 0; i < values.count(); ++i )
            list += KURL( values[i] );
    }

    values.clear();
    names.clear();
    delete db;

    #undef item

    return list;
}


QString SmartPlaylistView::customPlaylistsFile()
{
    //returns the file used to store custom smart playlists
    return KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "smartplaylists";
}


QDragObject *SmartPlaylistView::dragObject()
{
    return new KURLDrag( loadSmartPlaylist( currentItem() ), this );
}


void SmartPlaylistView::paintEmptyArea( QPainter *p, const QRect &r )
{
    if( !childCount() ) {
        p->fillRect( r, colorGroup().base() );
        p->drawText(10, 10, width()-20, height()-20, WordBreak,
                    i18n("You need to build a collection to use \"Smart Playlists\"") );
    } else
        QListView::paintEmptyArea( p, r );
}


void SmartPlaylistView::loadPlaylistSlot( QListViewItem *item ) //SLOT
{
    if( !item )
        return;

    if( !((SmartPlaylist*)item)->query().isEmpty() ) {
        // open the smart playlist
        Playlist::instance()->clear();
        Playlist::instance()->appendMedia( loadSmartPlaylist( item ) );
    }
}


void SmartPlaylistView::showContextMenu( QListViewItem *item, const QPoint &p, int ) //SLOT
{
    #define item static_cast<SmartPlaylist*>(item)

    if( !item ) return;

    if( item->isCustom() ) {
        enum Id { EDIT, REMOVE };

        KPopupMenu menu;
        //menu.insertItem( i18n("Edit"), EDIT );
        menu.insertItem( i18n("Remove"), REMOVE );

        switch( menu.exec( p ) ) {
            case EDIT:
                break;

            case REMOVE:
                removeSelectedPlaylists();
                break;
        };
    }

    #undef item
}


void SmartPlaylistView::collectionScanDone() //SLOT
{
    if( CollectionDB().isEmpty() ) {
        clear();
        m_loaded = false;
    }
    else if( !m_loaded ) {
        loadDefaultPlaylists();
        loadCustomPlaylists();
        m_loaded = true;
    }
}


/////////////////////////////////////////////////////////////////////////////
//    CLASS SmartPlaylist
////////////////////////////////////////////////////////////////////////////

SmartPlaylist::SmartPlaylist( KListView *parent, KListViewItem *after, QString name,
                              QString query, QString icon, bool custom )
    : KListViewItem( parent, after, name )
    , m_query( query )
    , m_custom( custom )
{
    setPixmap( 0, SmallIcon( icon.isEmpty() ? "player_playlist_2" : icon ) );
    setDragEnabled(true);
}

SmartPlaylist::SmartPlaylist( SmartPlaylist *item, KListViewItem *after, QString name,
                              QString query, QString icon, bool custom )
    : KListViewItem( item, after, name )
    , m_query( query )
    , m_custom( custom )
{
    setPixmap( 0, SmallIcon( icon.isEmpty() ? "player_playlist_2" : icon ) );
    setDragEnabled(true);
}

QString SmartPlaylist::key( int column, bool ) const
{
    //we want to show default playlists above the custom playlists
    return ( m_custom || parent() ? text(column) : QString( "000000" + QString::number(m_key) ) );
}

#include "smartplaylist.moc"
