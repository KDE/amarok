// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#include "collectionbrowser.h"
#include "collectiondb.h"
#include "directorylist.h"
#include "metabundle.h"
#include "playlist.h"       //insertMedia(), showTrackInfo()
#include "statusbar.h"
#include "threadweaver.h"

#include <unistd.h>         //CollectionView ctor

#include <qapplication.h>
#include <qcstring.h>
#include <qdragobject.h>
#include <qptrlist.h>
#include <qtimer.h>
#include <qtooltip.h>       //QToolTip::add()

#include <kactioncollection.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kiconloader.h>    //renderView()
#include <klocale.h>
#include <kmenubar.h>
#include <kpopupmenu.h>
#include <kprogress.h>
#include <kpushbutton.h>
#include <ktoolbarbutton.h> //ctor
#include <kurldrag.h>       //dragObject()


namespace amaroK { extern KConfig *config( const QString& ); }
static const int MONITOR_INTERVAL = 1000 * 30; //ms


CollectionBrowser::CollectionBrowser( const char* name )
    : QVBox( 0, name )
{
    setSpacing( 4 );
    setMargin( 5 );

    m_actionsMenu = new KPopupMenu( this );
    m_cat1Menu = new KPopupMenu( this );
    m_cat2Menu = new KPopupMenu( this );

    KMenuBar* menu = new KMenuBar( this );
    menu->insertItem( i18n( "Actions" ), m_actionsMenu );
    menu->insertItem( i18n( "Primary" ), m_cat1Menu );
    menu->insertItem( i18n( "Secondary" ), m_cat2Menu );

    { //<Search LineEdit>
        QHBox *hbox; QToolButton *button;

        hbox         = new QHBox( this );
        button       = new QToolButton( hbox );
        m_searchEdit = new KLineEdit( hbox );

        hbox->setMargin( 4 );
        button->setIconSet( SmallIconSet( QApplication::reverseLayout() ? "clear_left" : "locationbar_erase" ) );
        m_searchEdit->setFrame( QFrame::Sunken );
        connect( button, SIGNAL(clicked()), m_searchEdit, SLOT(clear()) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_searchEdit, i18n( "Enter space-separated terms to filter collection" ) );
    } //</Search LineEdit>

    timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( slotSetFilter() ) );

    m_view = new CollectionView( this );
    //m_view->setMargin( 2 );

    m_actionsMenu->insertItem( i18n( "Configure Collection Folders..." ), m_view, SLOT( setupDirs() ) );
    m_actionsMenu->insertItem( i18n( "Configure Cover Download" ), m_view->m_db, SLOT( setupCoverFetcher() ) );

    m_actionsMenu->insertSeparator();
    m_actionsMenu->insertItem( i18n( "Start Scan" ), m_view, SLOT( scan() ), 0, IdScan );

    m_cat1Menu ->insertItem( i18n( "Album" ), m_view, SLOT( cat1Menu( int ) ), 0, IdAlbum );
    m_cat1Menu ->insertItem( i18n( "Artist"), m_view, SLOT( cat1Menu( int ) ), 0, IdArtist );
    m_cat1Menu ->insertItem( i18n( "Genre" ), m_view, SLOT( cat1Menu( int ) ), 0, IdGenre );
    m_cat1Menu ->insertItem( i18n( "Year" ), m_view, SLOT( cat1Menu( int ) ), 0, IdYear );

    m_cat2Menu ->insertItem( i18n( "None" ), m_view, SLOT( cat2Menu( int ) ), 0, IdNone );
    m_cat2Menu ->insertSeparator();
    m_cat2Menu ->insertItem( i18n( "Album" ), m_view, SLOT( cat2Menu( int ) ), 0, IdAlbum );
    m_cat2Menu ->insertItem( i18n( "Artist" ), m_view, SLOT( cat2Menu( int ) ), 0, IdArtist );
    m_cat2Menu ->insertItem( i18n( "Genre" ), m_view, SLOT( cat2Menu( int ) ), 0, IdGenre );
    m_cat2Menu ->insertItem( i18n( "Year" ), m_view, SLOT( cat2Menu( int ) ), 0, IdYear );

    m_view->cat1Menu( m_view->idForCat( m_view->m_category1 ), false );
    m_view->cat2Menu( m_view->idForCat( m_view->m_category2 ), true );

    connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ),
             this,           SLOT( slotSetFilterTimeout() ) );

    setFocusProxy( m_view ); //default object to get focus
    setMinimumWidth( menu->sizeHint().width() + 2 ); //set a reasonable minWidth
}


void
CollectionBrowser::slotSetFilterTimeout() //SLOT
{
    if ( timer->isActive() ) timer->stop();
    timer->start( 180, true );
}


void
CollectionBrowser::slotSetFilter() //SLOT
{
    m_view->setFilter( m_searchEdit->text() );
    m_view->renderView();
}


void
CollectionBrowser::setupDirs()  //SLOT
{
    m_view->setupDirs();
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionView
//////////////////////////////////////////////////////////////////////////////////////////

CollectionView* CollectionView::m_instance;
CollectionDB* CollectionView::m_db;
CollectionDB* CollectionView::m_insertdb;


CollectionView::CollectionView( CollectionBrowser* parent )
        : KListView( parent )
        , m_parent( parent )
        , m_isScanning( false )
        , m_progressBox( 0 )
{
    kdDebug() << k_funcinfo << endl;
    m_instance = this;
    
    setSelectionMode( QListView::Extended );
    setItemsMovable( false );
    setRootIsDecorated( true );
    setShowSortIndicator( true );
    setFullWidth( true );
    setAcceptDrops( false );
    setSorting( -1 );

    m_db = new CollectionDB();
    if ( !m_db )
        kdWarning() << k_funcinfo << "Could not open SQLite database\n";
    
    //<read config>
        KConfig* config = amaroK::config( "Collection Browser" );

        m_dirs = config->readListEntry( "Folders" );
        m_category1 = config->readEntry( "Category1", i18n( "Artist" ) );
        m_category2 = config->readEntry( "Category2", i18n( "None" ) );
        addColumn( m_category1 );
        m_recursively = config->readBoolEntry( "Scan Recursively", true );
        m_monitor = config->readBoolEntry( "Monitor Changes", false );
    //</read config>

    //<open database>
        //optimization for speeding up SQLite
        m_db->execSql( "PRAGMA default_synchronous = OFF;" );
        // m_db->execSql( "PRAGMA default_cache_size = 4000;" ); default is 2000, that should be enough.

        //remove database file if version is incompatible
        if ( ( config->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION ) || ( !m_db->isDbValid() ) )
        {
            kdDebug() << "Rebuilding database!" << endl;
            m_db->dropTables();
            m_db->createTables();
        }
        if ( ( config->readNumEntry( "Database Stats Version", 0 ) != DATABASE_STATS_VERSION ) || ( !m_db->isDbValid() ) )
        {
            m_db->dropStatsTable();
            m_db->createStatsTable();
        }

        m_insertdb = new CollectionDB();
        connect( m_insertdb,     SIGNAL( scanDone( bool ) ),
                 this,             SLOT( scanDone( bool ) ) );

        if ( config->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION )
            scan();
        else
            scanMonitor();
    //</open database>

    connect( this,           SIGNAL( expanded( QListViewItem* ) ),
             this,             SLOT( slotExpand( QListViewItem* ) ) );
    connect( this,           SIGNAL( collapsed( QListViewItem* ) ),
             this,             SLOT( slotCollapse( QListViewItem* ) ) );
    connect( this,           SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( doubleClicked( QListViewItem*, const QPoint&, int ) ) );
    connect( this,           SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );

    startTimer( MONITOR_INTERVAL );
}


CollectionView::~CollectionView() {
    kdDebug() << k_funcinfo << endl;

    KConfig* const config = amaroK::config( "Collection Browser" );
    config->writeEntry( "Folders", m_dirs );
    config->writeEntry( "Category1", m_category1 );
    config->writeEntry( "Category2", m_category2 );
    config->writeEntry( "Scan Recursively", m_recursively );
    config->writeEntry( "Monitor Changes", m_monitor );
    config->writeEntry( "Database Version", DATABASE_VERSION );
    config->writeEntry( "Database Stats Version", DATABASE_STATS_VERSION );

    delete m_db;
}

//////////////////////////////////////////////////////////////////////////////////////////
// private slots
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::setupDirs()  //SLOT
{
    DirectoryList list( m_dirs, m_recursively, m_monitor, parentWidget() );
    DirectoryList::Result result = list.exec();

    // Check to see if Cancel was pressed
    if ( result.status == QDialog::Rejected )
        return;

    m_dirs = result.dirs;
    m_recursively = result.scanRecursively;
    m_monitor = result.monitorChanges;

    scan();
}


void
CollectionView::scan()  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    if ( !m_isScanning )
    {
        m_isScanning = true;
        m_parent->m_actionsMenu->setItemEnabled( CollectionBrowser::IdScan, false );
        m_insertdb->scan( m_dirs, m_recursively );
    
        m_progressBox = new QHBox( m_parent  );
        KPushButton* button = new KPushButton( i18n( "Abort Scan" ), m_progressBox );
        connect( button, SIGNAL( clicked() ), m_insertdb, SLOT( stopScan() ) );
        m_progress = new KProgress( m_progressBox );
        m_progressBox->show();
    }
}


void
CollectionView::scanMonitor()  //SLOT
{
    if ( !m_isScanning && m_monitor )
    {
        m_isScanning = true;
        m_parent->m_actionsMenu->setItemEnabled( CollectionBrowser::IdScan, false );
        m_insertdb->scanModifiedDirs( m_recursively );
    }
}


void
CollectionView::renderView( )  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    clear();
    QPixmap pixmap = iconForCat( m_category1 );

    //query database for all records with the specified category
    QStringList values;
    QStringList names;
    m_db->retrieveFirstLevel( tableForCat( m_category1 ), tableForCat( m_category2 ), m_filter, &values, &names );

    for ( uint i = 0; i < values.count(); i++ ) {
        if ( values[i].isEmpty() ) continue;

        KListViewItem* item = new KListViewItem( this );
        item->setExpandable( true );
        item->setDragEnabled( true );
        item->setDropEnabled( false );
        item->setText( 0, values[ i ] );
        item->setPixmap( 0, pixmap );
    }
}


void
CollectionView::scanDone( bool changed ) //SLOT
{
    // we need to reconnect to the db after every scan, since sqlite is not able to keep
    // the tables synced for multiple threads.
    delete m_db;
    m_db = new CollectionDB();

    if ( changed )
        renderView();

    m_parent->m_actionsMenu->setItemEnabled( CollectionBrowser::IdScan, true );
    m_isScanning = false;
    
    delete m_progressBox;
    m_progressBox = 0;
}


void
CollectionView::slotExpand( QListViewItem* item )  //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !item ) return ;

    if  ( item->depth() == 0 ) {
        QStringList values;
        QStringList names;
        m_db->retrieveSecondLevel( item->text( 0 ), tableForCat( m_category1 ), tableForCat( m_category2 ), m_filter, &values, &names );

        QPixmap pixmap = iconForCat( m_category2 );

        for ( uint i = 0; i < values.count(); i += 2 ) {
            Item* child = new Item( item );
            child->setDragEnabled( true );
            child->setDropEnabled( false );
            child->setText( 0, values[ i + 0 ] );
            if ( m_category2 != i18n( "None" ) )
                child->setPixmap( 0, pixmap );
            child->setUrl( values[ i + 1 ] );
            child->setExpandable( m_category2 != i18n( "None" ) );
        }
    }
    else {
        QStringList values;
        QStringList names;
        m_db->retrieveThirdLevel( item->parent()->text( 0 ), item->text( 0 ), tableForCat( m_category1 ), tableForCat( m_category2 ), m_filter, &values, &names );

        for ( uint i = 0; i < values.count(); i += 2 ) {
            Item* child = new Item( item );
            child->setDragEnabled( true );
            child->setDropEnabled( false );
            child->setText( 0, values[ i + 0 ] );
            child->setUrl( values[ i + 1 ] );
        }
    }
}


void
CollectionView::slotCollapse( QListViewItem* item )  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    QListViewItem* child = item->firstChild();
    QListViewItem* childTmp;
    //delete all children
    while ( child ) {
        childTmp = child;
        child = child->nextSibling();
        delete childTmp;
    }
}


void
CollectionView::cat1Menu( int id, bool rerender )  //SLOT
{
    m_parent->m_cat1Menu->setItemChecked( idForCat( m_category1 ), false ); //uncheck old item
    m_parent->m_cat2Menu->setItemEnabled( idForCat( m_category1 ), true );  //enable old item
    m_category1 = catForId( id );
    setColumnText( 0, m_category1 );
    m_parent->m_cat1Menu->setItemChecked( idForCat( m_category1 ), true );

    //prevent choosing the same category in both menus
    m_parent->m_cat2Menu->setItemEnabled( id , false );

    if ( rerender )
        renderView();
}


void
CollectionView::cat2Menu( int id, bool rerender )  //SLOT
{
    m_parent->m_cat2Menu->setItemChecked( idForCat( m_category2 ), false ); //uncheck old item
    m_parent->m_cat1Menu->setItemEnabled( idForCat( m_category2 ), true );  //enable old item
    m_category2 = catForId( id );
    m_parent->m_cat2Menu->setItemChecked( idForCat( m_category2 ), true );

    //prevent choosing the same category in both menus
    m_parent->m_cat1Menu->setItemEnabled( id , false );

    if ( rerender )
        renderView();
}


void
CollectionView::doubleClicked( QListViewItem* item, const QPoint&, int ) //SLOT
{
    if ( !item )
        return;

    item->setOpen( !item->isOpen() );
}


void
CollectionView::rmbPressed( QListViewItem* item, const QPoint& point, int ) //SLOT
{
    if ( item ) {
        KPopupMenu menu( this );

        enum Actions { MAKE, APPEND, QUEUE, COVER, INFO };

        menu.insertItem( i18n( "&Make Playlist" ), MAKE );
        menu.insertItem( i18n( "&Add to Playlist" ), APPEND ); //TODO say Append to Playlist
        menu.insertItem( i18n( "&Queue After Current Track" ), QUEUE );
        
        menu.insertSeparator();
        
        menu.insertItem( i18n( "&Fetch Cover Images" ), this, SLOT( fetchCover() ), 0, COVER );
        menu.insertItem( i18n( "Track Information" ), this, SLOT( showTrackInfo() ), 0, INFO );
        
        menu.setItemEnabled( INFO, ( item->depth() && m_category2 == i18n( "None" ) ) || item->depth() == 2 );

        switch( menu.exec( point ) ) {
            case MAKE:
                Playlist::instance()->clear(); //FALL THROUGH
            case APPEND:
                Playlist::instance()->appendMedia( listSelected() );
                break;
            case QUEUE:
                Playlist::instance()->queueMedia( listSelected() );
                break;
        }
    }
}


void
CollectionView::fetchCover() //SLOT
{
    Item* item = static_cast<Item*>( currentItem() );
    if ( !item ) return;
    if ( m_category2 != i18n( "None" ) && item->depth() != 2 ) return;

    KURL::List urls( listSelected() );
    
    for ( uint i = 0; i < urls.count(); i++ ) {
        QString command = QString
                            ( "SELECT DISTINCT artist.name, album.name FROM artist, tags, album "
                            "WHERE artist.id = tags.artist AND album.id = tags.album AND tags.url = '%1';" )
                            .arg( m_db->escapeString( urls[i].path() ) );
        QStringList values;
        QStringList names;
        m_db->execSql( command, &values, &names );
        if ( values.isEmpty() ) continue;
        QString key = values[0] + " - " + values[1];
        
        m_db->fetchCover( this, key );
    }
}

                        
void
CollectionView::showTrackInfo() //SLOT
{
    Item* item = static_cast<Item*>( currentItem() );
    if ( !item ) return;

    if ( m_category2 == i18n( "None" ) || item->depth() == 2 )
        Playlist::showTrackInfo( item->url() );
}


//////////////////////////////////////////////////////////////////////////////////////////
// protected
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::timerEvent( QTimerEvent* )
{
    if ( m_monitor )
        scanMonitor();
}


void
CollectionView::customEvent( QCustomEvent *e )
{
    CollectionReader::ProgressEvent* p = (CollectionReader::ProgressEvent*)e;

    switch ( p->state() ) {
    case CollectionReader::ProgressEvent::Start:
        m_progress->setProgress( 0 );
        m_progress->show();
        break;

    case CollectionReader::ProgressEvent::Stop:
        break;

    case CollectionReader::ProgressEvent::Total:
        m_progress->setTotalSteps( p->value() );
        break;

    case CollectionReader::ProgressEvent::Progress:
        m_progress->setProgress( p->value() );
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// private
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::startDrag() {
    KURLDrag* d = new KURLDrag( listSelected(), this );
    d->dragCopy();
}


KURL::List
CollectionView::listSelected() {
    //Here we determine the URLs of all selected items. We use two passes, one for the parent items,
    //and another one for the children.

    KURL::List list;
    QListViewItem* item;
    QStringList values;
    QStringList names;

    //first pass: parents
    for ( item = firstChild(); item; item = item->nextSibling() )
        if ( item->isSelected() )
        {
            values.clear();
            names.clear();

            m_db->retrieveFirstLevelURLs( item->text( 0 ), tableForCat( m_category1 ), tableForCat( m_category2 ), m_filter, &values, &names );
            for ( uint i = 0; i < values.count(); i++ )
            {
                KURL tmp;
                tmp.setPath( values[i] );
                list << tmp;
            }
        }

    //second pass: category 1
    if ( m_category2 == i18n( "None" ) )
    {
        for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                if ( child->isSelected() && !child->parent()->isSelected() )
                    list << static_cast<Item*>( child ) ->url();
    }
    else {
        for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                if ( child->isSelected() && !child->parent()->isSelected() )
                {
                    values.clear();
                    names.clear();

                    m_db->retrieveSecondLevelURLs( item->text( 0 ), child->text( 0 ), tableForCat( m_category1 ), tableForCat( m_category2 ), m_filter, &values, &names );
                    for ( uint i = 0; i < values.count(); i++ )
                    {
                        KURL tmp;
                        tmp.setPath( values[i] );
                        list << tmp;
                    }
                }
    }

    //third pass: category 2
    for ( item = firstChild(); item; item = item->nextSibling() )
        for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
            for ( QListViewItem* grandChild = child->firstChild(); grandChild; grandChild = grandChild->nextSibling() )
                if ( grandChild->isSelected() && !child->parent()->isSelected() && !child->isSelected() )
                    list << static_cast<Item*>( grandChild ) ->url();

    return list;
}


QString
CollectionView::catForId( int id ) const
{
    switch ( id ) {
        case CollectionBrowser::IdAlbum:
            return i18n( "Album" );
        case CollectionBrowser::IdArtist:
            return i18n( "Artist" );
        case CollectionBrowser::IdGenre:
            return i18n( "Genre" );
        case CollectionBrowser::IdYear:
            return i18n( "Year" );
        default:
            break;
    }

    return i18n( "None" );
}


int
CollectionView::idForCat( const QString& cat ) const
{
    if ( cat == i18n( "Album" ) ) return CollectionBrowser::IdAlbum;
    if ( cat == i18n( "Artist" ) ) return CollectionBrowser::IdArtist;
    if ( cat == i18n( "Genre" ) ) return CollectionBrowser::IdGenre;
    if ( cat == i18n( "Year" ) ) return CollectionBrowser::IdYear;

    //falltrough:
    return CollectionBrowser::IdNone;
}


QPixmap
CollectionView::iconForCat( const QString& cat ) const
{
    QString icon;
    if ( cat == i18n( "Album" ) ) icon = "cdrom_unmount";
    if ( cat == i18n( "Artist" ) ) icon = "personal";
    if ( cat == i18n( "Genre" ) ) icon = "kfm";
    if ( cat == i18n( "Year" ) ) icon = "history";

    KIconLoader iconLoader;
    return iconLoader.loadIcon( icon, KIcon::Toolbar, KIcon::SizeSmall );
}


QString
CollectionView::tableForCat( const QString& cat ) const
{
    if ( cat == i18n( "Album" ) ) return "album";
    if ( cat == i18n( "Artist" ) ) return "artist";
    if ( cat == i18n( "Genre" ) ) return "genre";
    if ( cat == i18n( "Year" ) ) return "year";

    //falltrough:
    return 0;
}
  

#include "collectionbrowser.moc"


