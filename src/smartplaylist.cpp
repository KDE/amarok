// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#include "amarok.h"
#include "collectiondb.h"
#include "k3bexporter.h"
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
    KToolBar *toolbar = new KToolBar( this );
    toolbar->setMovingEnabled(false);
    toolbar->setFlat(true);
    toolbar->setIconSize( 16 );
    toolbar->setEnableContextMenu( false );

    SmartPlaylistView *smartListView = new SmartPlaylistView( this );

    KActionCollection *ac = new KActionCollection( this );
    KAction *createSmartPlayist = new KAction( i18n("Create Smart-Playlist"), "filenew", 0, smartListView, SLOT( createCustomPlaylist() ), ac, "Create Smart-playlist" );
    KAction *remove = new KAction( i18n("Remove"), "edittrash", 0, smartListView, SLOT( removeSelectedPlaylists() ), ac, "Remove" );

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
    addColumn( i18n("Smart-Playlists") );
    setSelectionMode(QListView::Extended);
    setSorting( 0 ); //enable sorting (used for custom smart playlists)
    setFullWidth( true );
    setRootIsDecorated( true );
    setShowSortIndicator( true );

    if( !CollectionDB::instance()->isEmpty() ) {
        loadDefaultPlaylists();
        loadCustomPlaylists();
        m_loaded = true;
    }

    connect( CollectionDB::instance(), SIGNAL( scanDone( bool ) ), SLOT( collectionScanDone() ) );

    connect( this, SIGNAL( doubleClicked(QListViewItem*) ), SLOT( loadPlaylistSlot(QListViewItem*) ) );
    connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
                     SLOT( showContextMenu( QListViewItem *, const QPoint &, int ) ) );
}


SmartPlaylistView::~SmartPlaylistView()
{
    QFile file( customPlaylistsFile() );

    if( file.open( IO_WriteOnly ) )
    {
        //save all custom smart playlists

        QTextStream stream( &file );
        for( QListViewItemIterator it( this ); it.current(); ++it ) {
            SmartPlaylist *item = static_cast<SmartPlaylist*>(*it);
            if( item->isCustom() ) {  //save only custom playlists
                stream << "Name=" + item->text(0);
                stream << "\n";
                stream << item->query();
                stream << "\n";
            }
        }
    }
}


void SmartPlaylistView::createCustomPlaylist() //SLOT
{
    //open a dialog to create a custom smart playlist

    if( CollectionDB::instance()->isEmpty() )
        return;

    int counter = 1;
    QListViewItemIterator it( this );
    for( ; it.current(); ++it ) {
        if( (*it)->text(0).startsWith( i18n("Untitled") ) )
            counter++;
    }

    SmartPlaylistEditor *editor = new SmartPlaylistEditor( this, i18n("Untitled %1").arg(counter) );
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
    const QStringList artists = CollectionDB::instance()->artistList();
    QString sql;
    SmartPlaylist *item;
    QueryBuilder qb;

    // album / artist / genre / title / year / comment / track / bitrate / length / samplerate / path

    /********** All Collection **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );

    item = new SmartPlaylist( this, 0, i18n( "All Collection" ), qb.query(), "kfm" );
    item->setKey( 1 );

    /********** Favorite Tracks **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( this, 0, i18n("Favorite Tracks"), qb.query() );
    item->setKey( 2 );
    SmartPlaylist *childItem = 0;
    foreach( artists ) {
        sql = QString( "SELECT tags.url "
                       "FROM tags, artist, statistics "
                       "WHERE statistics.url = tags.url AND tags.artist = artist.id AND artist.name = '%1' "
                       "ORDER BY statistics.percentage DESC "
                       "LIMIT 0,15;" )
                       .arg( CollectionDB::instance()->escapeString( *it ) );

        childItem = new SmartPlaylist( item, childItem, i18n("By %1").arg( *it ), sql );
    }

    /********** Most Played **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( this, 0, i18n("Most Played"), qb.query() );
    item->setKey( 3 );
    childItem = 0;
    foreach( artists ) {
        sql = QString( "SELECT tags.url "
                       "FROM tags, artist, statistics "
                       "WHERE statistics.url = tags.url AND tags.artist = artist.id AND artist.name = '%1' "
                       "ORDER BY statistics.playcounter DESC "
                       "LIMIT 0,15;" ).arg( CollectionDB::instance()->escapeString( *it ) );
        childItem = new SmartPlaylist( item, childItem, i18n("By %1").arg( *it ), sql );
    }

    /********** Newest Tracks **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( this, 0, i18n("Newest Tracks"), qb.query() );
    item->setKey( 4 );
    childItem = 0;
    foreach( artists ) {
        sql = QString( "SELECT tags.url "
                       "FROM tags, artist "
                       "WHERE tags.artist = artist.id AND artist.name = '%1' "
                       "ORDER BY tags.createdate DESC "
                       "LIMIT 0,15;" ).arg( CollectionDB::instance()->escapeString( *it ) );
        childItem = new SmartPlaylist( item, childItem, i18n("By %1").arg( *it ), sql );
    }

    /********** Last Played **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valAccessDate, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( this, 0, i18n("Last Played"), qb.query() );
    item->setKey( 5 );

    /********** Never Played **************/
    sql = "SELECT tags.url "
          "FROM tags, artist "
          "WHERE tags.url NOT IN(SELECT url FROM statistics) AND tags.artist = artist.id "
          "ORDER BY artist.name, tags.title;";
    item = new SmartPlaylist(this, 0, i18n("Never Played"), sql );
    item->setKey( 6 );

    /********** Genres **************/
    item = new SmartPlaylist( this, 0, i18n("Genres") );
    item->setDragEnabled( false );
    item->setKey( 7 );

    const QStringList genres = CollectionDB::instance()->query( "SELECT DISTINCT name FROM genre;" );
    childItem = 0;
    foreach( genres ) {
        sql = QString( "SELECT tags.url "
                       "FROM tags, artist "
                       "WHERE tags.genre = %1 AND tags.artist = artist.id "
                       "ORDER BY artist.name, tags.title;" )
                       .arg( CollectionDB::instance()->genreID( *it, false ) );

        childItem = new SmartPlaylist( item, childItem, *it, sql );
    }

    /********** 100 Random Tracks **************/
    qb.initSQLDrag();
    qb.setOptions( QueryBuilder::optRandomize );
    qb.setLimit( 0, 100 );

    item = new SmartPlaylist( this, 0, i18n("100 Random Tracks"), qb.query() );
    item->setKey( 8 );
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


QString SmartPlaylistView::customPlaylistsFile()
{
    //returns the file used to store custom smart playlists
    return amaroK::saveLocation() + "smartplaylists";
}

#include "debug.h"
#include <qdragobject.h>
#include <kmultipledrag.h>
QDragObject *SmartPlaylistView::dragObject()
{
    DEBUG_FUNC_INFO

    //TODO KURL drag is also required, but preferably only
    //     determine the KURLs when pasting, otherwise only
    //     provide the mimetype

    KURL::List urls;
    QPtrList<QListViewItem> items = selectedItems();

    KMultipleDrag *drag = new KMultipleDrag( this );

    for( SmartPlaylist *item = (SmartPlaylist*)items.first(); item; item = (SmartPlaylist*)items.next() )
    {
        urls += item->urls();

        QTextDrag *textdrag = new QTextDrag( item->query(), 0 );
        textdrag->setSubtype( "amarok-sql" );
        drag->addDragObject( textdrag );
    }

    drag->addDragObject( new KURLDrag( urls, 0 ) );
    return drag;
}


void SmartPlaylistView::paintEmptyArea( QPainter *p, const QRect &r )
{
    if( !childCount() ) {
        p->fillRect( r, colorGroup().base() );
        p->drawText( 10, 10, width()-20, height()-20, WordBreak,
                i18n("You need to build a collection to use \"Smart Playlists\"") );
    } else
        QListView::paintEmptyArea( p, r );
}


void SmartPlaylistView::loadPlaylistSlot( QListViewItem *item ) //SLOT
{
    if( !item )
        return;

    #define item static_cast<SmartPlaylist*>(item)
    if( !item->query().isEmpty() ) {
        // open the smart playlist
        Playlist::instance()->clear();
        Playlist::instance()->insertMedia( item->urls() );
    }
    #undef item
}


void SmartPlaylistView::showContextMenu( QListViewItem *item, const QPoint &p, int ) //SLOT
{
    #define item static_cast<SmartPlaylist*>(item)

    if( !item ) return;

    enum Id { BURN_DATACD, BURN_AUDIOCD, EDIT, REMOVE };

    KPopupMenu menu( this );
    //TODO menu.insertItem( i18n("Edit"), EDIT );
    menu.insertItem( i18n("Burn to CD as Data"), BURN_DATACD );
    menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
    menu.insertItem( i18n("Burn to CD as Audio"), BURN_AUDIOCD );
    menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
    if( item->isCustom() ) {
        menu.insertSeparator();
        menu.insertItem( i18n("Remove"), REMOVE );
    }

    switch( menu.exec( p ) ) {
        case BURN_DATACD:
            K3bExporter::instance()->exportTracks( item->urls(), K3bExporter::DataCD );
            break;

        case BURN_AUDIOCD:
            K3bExporter::instance()->exportTracks( item->urls(), K3bExporter::AudioCD );
            break;

        case EDIT:
            break;

        case REMOVE:
            removeSelectedPlaylists();
            break;
    };

    #undef item
}


void SmartPlaylistView::collectionScanDone() //SLOT
{
    if( CollectionDB::instance()->isEmpty() ) {
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

SmartPlaylist::SmartPlaylist( KListView *parent, KListViewItem *after, const QString &name,
                              const QString &query, const QString &icon, bool custom )
    : KListViewItem( parent, after, name )
    , m_query( query )
    , m_custom( custom )
{
    setPixmap( 0, SmallIcon( icon.isEmpty() ? "player_playlist_2" : icon ) );
    setDragEnabled(true);
}

SmartPlaylist::SmartPlaylist( SmartPlaylist *item, KListViewItem *after, const QString &name,
                             const QString &query, const QString &icon, bool custom )
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

KURL::List
SmartPlaylist::urls() const
{
    KURL::List list;
    KURL url;

    const QStringList values = CollectionDB::instance()->query( m_query );
    foreach( values ) {
        url.setPath( *it );
        list += url;
    }

    return list;
}

#include "smartplaylist.moc"
