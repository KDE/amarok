// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#include "config.h"

#include "amarokconfig.h"
#include "clicklineedit.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "directorylist.h"
#include "k3bexporter.h"
#include "metabundle.h"
#include "playlist.h"       //insertMedia()
#include "statusbar.h"
#include "tagdialog.h"
#include "threadweaver.h"

#include <unistd.h>         //CollectionView ctor

#include <qapplication.h>
#include <qcstring.h>
#include <qdragobject.h>
#include <qptrlist.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qtooltip.h>       //QToolTip::add()

#include <kactioncollection.h>
#include <kapplication.h>   //kapp
#include <kconfig.h>
#include <kdebug.h>
#include <kiconloader.h>    //renderView()
#include <klocale.h>
#include <kpopupmenu.h>
#include <kprogress.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h> //ctor
#include <kurldrag.h>       //dragObject()


namespace amaroK { extern KConfig *config( const QString& ); }
static const int MONITOR_INTERVAL = 1000 * 30; //ms


CollectionBrowser::CollectionBrowser( const char* name )
    : QVBox( 0, name )
    , m_cat1Menu( new KPopupMenu( this ) )
    , m_cat2Menu( new KPopupMenu( this ) )
    , m_cat3Menu( new KPopupMenu( this ) )
    , m_timer( new QTimer( this ) )
{
    setSpacing( 4 );
    setMargin( 5 );

    KToolBar* toolbar = new KToolBar( this );
    toolbar->setMovingEnabled(false);
    toolbar->setFlat(true);
    toolbar->setIconSize( 16 );
    toolbar->setEnableContextMenu( false );

    { //<Search LineEdit>
        QHBox *hbox; KToolBarButton *button;

        hbox         = new QHBox( this );
        button       = new KToolBarButton( "locationbar_erase", 0, hbox );
        m_searchEdit = new ClickLineEdit( hbox, i18n( "Filter here..." ), "filter_edit" );

        hbox->setMargin( 1 );
        m_searchEdit->setFrame( QFrame::Sunken );
        connect( button, SIGNAL( clicked() ), m_searchEdit, SLOT( clear() ) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_searchEdit, i18n( "Enter space-separated terms to filter collection" ) );
    } //</Search LineEdit>

    KActionCollection* ac = new KActionCollection( this );
    m_scanAction = new KAction( i18n("Start Scan"), "reload", 0, this, SLOT( scan() ), ac, "Start Scan" );
    m_configureAction = new KAction( i18n("Configure Folders"), "configure", 0, this, SLOT( setupDirs() ), ac, "Configure" );

    KActionMenu* tagfilterMenuButton = new KActionMenu( i18n("Tag Filter"), "filter", ac );
    tagfilterMenuButton->setDelayed( false );
    m_categoryMenu = tagfilterMenuButton->popupMenu();

    m_view = new CollectionView( this );

    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );

    toolbar->setIconText( KToolBar::IconTextRight, false ); //we want the open button to have text on right
    m_scanAction->plug( toolbar );

    toolbar->insertLineSeparator();

    tagfilterMenuButton->plug( toolbar );

    toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance
    m_configureAction->plug( toolbar );

    m_categoryMenu->insertItem( i18n( "&First Level" ), m_cat1Menu );
    m_categoryMenu->insertItem( i18n( "&Second Level"), m_cat2Menu );
    m_categoryMenu->insertItem( i18n( "&Third Level" ), m_cat3Menu );

    m_cat1Menu ->insertItem( i18n( "&Album" ), m_view, SLOT( cat1Menu( int ) ), 0, IdAlbum );
    m_cat1Menu ->insertItem( i18n( "A&rtist"), m_view, SLOT( cat1Menu( int ) ), 0, IdArtist );
    m_cat1Menu ->insertItem( i18n( "&Genre" ), m_view, SLOT( cat1Menu( int ) ), 0, IdGenre );
    m_cat1Menu ->insertItem( i18n( "&Year" ), m_view, SLOT( cat1Menu( int ) ), 0, IdYear );

    m_cat2Menu ->insertItem( i18n( "&None" ), m_view, SLOT( cat2Menu( int ) ), 0, IdNone );
    m_cat2Menu ->insertSeparator();
    m_cat2Menu ->insertItem( i18n( "&Album" ), m_view, SLOT( cat2Menu( int ) ), 0, IdAlbum );
    m_cat2Menu ->insertItem( i18n( "A&rtist" ), m_view, SLOT( cat2Menu( int ) ), 0, IdArtist );
    m_cat2Menu ->insertItem( i18n( "&Genre" ), m_view, SLOT( cat2Menu( int ) ), 0, IdGenre );
    m_cat2Menu ->insertItem( i18n( "&Year" ), m_view, SLOT( cat2Menu( int ) ), 0, IdYear );

    m_cat3Menu ->insertItem( i18n( "&None" ), m_view, SLOT( cat3Menu( int ) ), 0, IdNone );
    m_cat3Menu ->insertSeparator();
    m_cat3Menu ->insertItem( i18n( "A&lbum" ), m_view, SLOT( cat3Menu( int ) ), 0, IdAlbum );
    m_cat3Menu ->insertItem( i18n( "A&rtist" ), m_view, SLOT( cat3Menu( int ) ), 0, IdArtist );
    m_cat3Menu ->insertItem( i18n( "&Genre" ), m_view, SLOT( cat3Menu( int ) ), 0, IdGenre );
    m_cat3Menu ->insertItem( i18n( "&Year" ), m_view, SLOT( cat3Menu( int ) ), 0, IdYear );

    m_view->cat1Menu( m_view->m_cat1, false );
    m_view->cat2Menu( m_view->m_cat2, false );
    m_view->cat3Menu( m_view->m_cat3, true );

    connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
    connect( m_searchEdit, SIGNAL( returnPressed() ), SLOT( slotSetFilter() ) );

    // This is used when the collection folders were changed in the first-run wizard
    connect( kapp, SIGNAL( sigScanCollection() ), m_view, SLOT( scan() ) );

    setFocusProxy( m_view ); //default object to get focus
    setMinimumWidth( toolbar->sizeHint().width() + 2 ); //set a reasonable minWidth
}

void
CollectionBrowser::slotSetFilterTimeout() //SLOT
{
    if ( m_timer->isActive() ) m_timer->stop();
    m_timer->start( 280, true );
}

void
CollectionBrowser::slotSetFilter() //SLOT
{
    if ( m_timer->isActive() ) m_timer->stop();

    m_view->setFilter( m_searchEdit->text() );
    m_view->renderView();
}

void
CollectionBrowser::scan()  //SLOT
{
    m_view->scan();
}

void
CollectionBrowser::setupDirs()  //SLOT
{
    m_view->setupDirs();
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionView
//////////////////////////////////////////////////////////////////////////////////////////

CollectionView* CollectionView::m_instance = 0;
CollectionDB* CollectionView::m_db = 0;
CollectionDB* CollectionView::m_insertdb = 0;


CollectionView::CollectionView( CollectionBrowser* parent )
        : KListView( parent )
        , m_parent( parent )
        , m_isScanning( false )
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

    //<READ CONFIG>
        KConfig* config = amaroK::config( "Collection Browser" );
        m_cat1 = config->readNumEntry( "Category1", CollectionBrowser::IdArtist );
        m_cat2 = config->readNumEntry( "Category2", CollectionBrowser::IdAlbum );
        m_cat3 = config->readNumEntry( "Category3", CollectionBrowser::IdNone );

        addColumn( captionForCategory( m_cat1 ) );
    //</READ CONFIG>

    //<OPEN DATABASE>
        //optimization for speeding up SQLite
#ifdef USE_MYSQL
//TODO?
#else
        m_db->query( "PRAGMA default_synchronous = OFF;" );
#endif

        //remove database file if version is incompatible
        if ( ( config->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION ) || ( !m_db->isValid() ) )
        {
            kdDebug() << "Rebuilding database!" << endl;
            m_db->dropTables();
            m_db->createTables();
        }
        if ( ( config->readNumEntry( "Database Stats Version", 0 ) != DATABASE_STATS_VERSION ) || ( !m_db->isValid() ) )
        {
            kdDebug() << "Rebuilding stats-database!" << endl;
            m_db->dropStatsTable();
            m_db->createStatsTable();
        }

        m_insertdb = new CollectionDB();
        connect( CollectionDB::emitter(), SIGNAL( scanDone( bool ) ),
                 this,                      SLOT( scanDone( bool ) ) );

        if ( config->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION )
            scan();
        else
            scanMonitor();
    //</OPEN DATABASE>

    //<PROGRESS BAR>
        m_progressBox = new QHBox( m_parent );
        QPushButton* button = new QPushButton( SmallIconSet( "button_cancel" ), i18n( "Abort" ), m_progressBox );
        connect( button, SIGNAL( clicked() ), m_insertdb, SLOT( stopScan() ) );
        m_progress = new KProgress( m_progressBox );
        m_progress->setFixedHeight( button->sizeHint().height() );
        m_progressBox->hide();
    //<PROGRESS BAR>

    connect( this,           SIGNAL( currentChanged( QListViewItem* ) ),
             this,             SLOT( cacheItem( QListViewItem* ) ) );
    connect( this,           SIGNAL( expanded( QListViewItem* ) ),
             this,             SLOT( slotExpand( QListViewItem* ) ) );
    connect( this,           SIGNAL( collapsed( QListViewItem* ) ),
             this,             SLOT( slotCollapse( QListViewItem* ) ) );
    connect( this,           SIGNAL( returnPressed( QListViewItem* ) ),
             this,             SLOT( invokeItem( QListViewItem* ) ) );
    connect( this,           SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( invokeItem( QListViewItem* ) ) );
    connect( this,           SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );

    startTimer( MONITOR_INTERVAL );
}


CollectionView::~CollectionView() {
    kdDebug() << k_funcinfo << endl;

    KConfig* const config = amaroK::config( "Collection Browser" );
    config->writeEntry( "Category1", m_cat1 );
    config->writeEntry( "Category2", m_cat2 );
    config->writeEntry( "Category3", m_cat3 );
    config->writeEntry( "Database Version", DATABASE_VERSION );
    config->writeEntry( "Database Stats Version", DATABASE_STATS_VERSION );
}

//////////////////////////////////////////////////////////////////////////////////////////
// private slots
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::setupDirs()  //SLOT
{
    KDialogBase dialog( this, 0, false, i18n("Configure Collection") );
    CollectionSetup *setup = new CollectionSetup( &dialog );
    dialog.setMainWidget( setup );
    dialog.showButtonApply( false );
    dialog.adjustSize();

    if ( dialog.exec() != QDialog::Rejected )
    {
        const bool rescan = ( AmarokConfig::collectionFolders() != setup->dirs() );
        setup->writeConfig();

        if ( rescan )
            scan();
    }
}


void
CollectionView::scan()  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    if ( AmarokConfig::collectionFolders().isEmpty() )
    {
        //FIXME: this should be done by CollectionDB, too
        m_insertdb->dropTables();
        this->clear();
//        emit CollectionDB::emitter()->scanDone( false );
    }
    else if ( !m_isScanning )
    {
        m_isScanning = true;
        m_parent->m_scanAction->setEnabled( false );

        m_insertdb->scan( AmarokConfig::collectionFolders(), AmarokConfig::scanRecursively(),
                          AmarokConfig::importPlaylists() );

        amaroK::StatusBar::instance()->message( i18n( "Building Collection..." ) );
    }
}


void
CollectionView::scanMonitor()  //SLOT
{
    if ( !m_isScanning && AmarokConfig::monitorChanges() )
    {
        m_parent->m_scanAction->setEnabled( false );
        m_insertdb->scanModifiedDirs( AmarokConfig::scanRecursively(), AmarokConfig::importPlaylists() );
    }
}


void
CollectionView::renderView( )  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    clear();
    QPixmap pixmap = iconForCategory( m_cat1 );

    //query database for all records with the specified category
    QStringList values;
    QueryBuilder qb;

    qb.addReturnValue( m_cat1, QueryBuilder::valName );
    qb.addFilter( m_cat1 | m_cat2 | m_cat3 | QueryBuilder::tabSong, m_filter );
    qb.sortBy( m_cat1, QueryBuilder::valName );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );

    if ( m_cat1 == QueryBuilder::tabArtist )
        qb.setOptions( QueryBuilder::optNoCompilations );

    values = qb.run();

    for ( int i = values.count() - 1; i >= 0; --i )
    {
        if ( values[i].stripWhiteSpace().isEmpty() )
            values[i] = i18n( "Unknown" );

        KListViewItem* item = new KListViewItem( this );
        item->setExpandable( true );
        item->setDragEnabled( true );
        item->setDropEnabled( false );
        item->setText( 0, values[ i ] );
        item->setPixmap( 0, pixmap );
    }

    if ( m_cat1 == QueryBuilder::tabArtist )
    {
        qb.clear();
        qb.addReturnValue( m_cat1, QueryBuilder::valName );
        qb.addFilter( m_cat1 | m_cat2 | m_cat3 | QueryBuilder::tabSong , m_filter );
        qb.setOptions( QueryBuilder::optOnlyCompilations | QueryBuilder::optRemoveDuplicates );
        qb.setLimit( 0, 1 );
        values = qb.run();

        if ( values.count() )
        {
            KListViewItem* item = new KListViewItem( this );
            item->setExpandable( true );
            item->setDragEnabled( true );
            item->setDropEnabled( false );
            item->setText( 0, i18n( "Various Artists" ) );
            item->setPixmap( 0, pixmap );
        }
    }
}


void
CollectionView::scanDone( bool changed ) //SLOT
{
    if ( changed )
    {
        // take care of sql updates (schema changed errors)
        delete m_db;
        m_db = new CollectionDB();

        renderView();

        //restore cached item
        if ( !m_cacheItem.isEmpty() )
        {
            QListViewItem* item = this->findItem( m_cacheItem[ 0 ], 0 );
            if ( item )
            {
                item->setOpen( true );
                for ( uint i = 1; i < m_cacheItem.count() && item; i++ )
                {
                    item = item->firstChild();
                    while ( item )
                    {
                        if ( item->text( 0 ) == m_cacheItem[ i ] )
                        {
                            item->setOpen( true );
                            break;
                        }
                        item = item->nextSibling();
                    }
                }

                if ( item )
                {
                    item->setSelected( true );
                    this->ensureItemVisible( item );
                }
            }
        }
    }
    m_parent->m_scanAction->setEnabled( true );
    m_isScanning = false;

    amaroK::StatusBar::instance()->clear();
}


void
CollectionView::slotExpand( QListViewItem* item )  //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !item ) return;

    int category;
    QStringList values;
    QueryBuilder qb;

    switch ( item->depth() )
    {
        case 0:
            // check for compilations
            if ( item->text( 0 ) != i18n( "Various Artists" ) )
                qb.addMatch( m_cat1, item->text( 0 ) );
            else
                qb.setOptions( QueryBuilder::optOnlyCompilations );

            if ( m_cat2 == QueryBuilder::tabSong )
            {
                qb.addReturnValue( m_cat2, QueryBuilder::valTitle );
                qb.addReturnValue( m_cat2, QueryBuilder::valURL );
                qb.sortBy( m_cat2, QueryBuilder::valTrack );
                qb.sortBy( m_cat2, QueryBuilder::valTitle );
            }
            else
            {
                qb.addReturnValue( m_cat2, QueryBuilder::valName );
                qb.sortBy( m_cat2, QueryBuilder::valName );
            }

            category = m_cat2;
            break;

        case 1:
            // check for compilations
            if ( item->parent()->text( 0 ) != i18n( "Various Artists" ) )
                qb.addMatch( m_cat1, item->parent()->text( 0 ) );
            else
                qb.setOptions( QueryBuilder::optOnlyCompilations );

            qb.addMatch( m_cat2, item->text( 0 ) );

            if ( m_cat3 == QueryBuilder::tabSong )
            {
                qb.addReturnValue( m_cat3, QueryBuilder::valTitle );
                qb.addReturnValue( m_cat3, QueryBuilder::valURL );
                qb.sortBy( m_cat3, QueryBuilder::valTrack );
                qb.sortBy( m_cat3, QueryBuilder::valTitle );
            }
            else
            {
                qb.addReturnValue( m_cat3, QueryBuilder::valName );
                qb.sortBy( m_cat3, QueryBuilder::valName );
            }

            category = m_cat3;
            break;

        case 2:
            // check for compilations
            if ( item->parent()->parent()->text( 0 ) != i18n( "Various Artists" ) )
                qb.addMatch( m_cat1, item->parent()->parent()->text( 0 ) );
            else
                qb.setOptions( QueryBuilder::optOnlyCompilations );

            qb.addMatch( m_cat2, item->parent()->text( 0 ) );
            qb.addMatch( m_cat3, item->text( 0 ) );

            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );

            category = CollectionBrowser::IdNone;
            break;
    }

    qb.addFilter( m_cat1 | m_cat2 | m_cat3 | QueryBuilder::tabSong, m_filter );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    values = qb.run();

    QPixmap pixmap;
    bool expandable = category != CollectionBrowser::IdNone;
    if ( expandable )
        pixmap = iconForCategory( category );

    for ( int i = values.count() - qb.countReturnValues(); i >= 0; i -= qb.countReturnValues() )
    {
        Item* child = new Item( item );
        child->setDragEnabled( true );
        child->setDropEnabled( false );
        child->setText( 0, values[ i ].stripWhiteSpace().isEmpty() ? i18n( "Unknown" ) : values[ i ] );
        if ( expandable )
            child->setPixmap( 0, pixmap );
        else
            child->setUrl( values[ i + 1 ] );
        child->setExpandable( expandable );
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
    m_parent->m_cat1Menu->setItemChecked( m_cat1, false ); //uncheck old item
    m_parent->m_cat2Menu->setItemEnabled( m_cat1, true );  //enable old item
    m_cat1 = id;
    setColumnText( 0, captionForCategory( m_cat1 ) );
    m_parent->m_cat1Menu->setItemChecked( m_cat1, true );

    //prevent choosing the same category in both menus
    m_parent->m_cat2Menu->setItemEnabled( id , false );
    m_parent->m_cat3Menu->setItemEnabled( id , false );

    //if this item is checked in second menu, uncheck it
    if ( m_parent->m_cat2Menu->isItemChecked( id ) ) {
        m_parent->m_cat2Menu->setItemChecked( id, false );
        m_parent->m_cat2Menu->setItemChecked( CollectionBrowser::IdNone, true );
        m_cat2 = CollectionBrowser::IdNone;
        enableCat3Menu( false );
    }
    //if this item is checked in third menu, uncheck it
    if ( m_parent->m_cat3Menu->isItemChecked( id ) ) {
        m_parent->m_cat3Menu->setItemChecked( id, false );
        m_parent->m_cat3Menu->setItemChecked( CollectionBrowser::IdNone, true );
        m_cat3 = CollectionBrowser::IdNone;
    }

    if ( rerender )
        renderView();
}


void
CollectionView::cat2Menu( int id, bool rerender )  //SLOT
{
    m_parent->m_cat2Menu->setItemChecked( m_cat2, false ); //uncheck old item
    m_parent->m_cat3Menu->setItemEnabled( m_cat3, true );  //enable old item
    m_cat2 = id;
    m_parent->m_cat2Menu->setItemChecked( m_cat2, true );

    enableCat3Menu( id != CollectionBrowser::IdNone );

    //prevent choosing the same category in both menus
    m_parent->m_cat3Menu->setItemEnabled( m_cat1 , false );
    if( id != CollectionBrowser::IdNone )
        m_parent->m_cat3Menu->setItemEnabled( id , false );

    //if this item is checked in third menu, uncheck it
    if ( m_parent->m_cat3Menu->isItemChecked( id ) ) {
        m_parent->m_cat3Menu->setItemChecked( id, false );
        enableCat3Menu( false );
    }

    if ( rerender )
        renderView();
}


void
CollectionView::cat3Menu( int id, bool rerender )  //SLOT
{
    m_parent->m_cat3Menu->setItemChecked( m_cat3, false ); //uncheck old item
    m_cat3 = id;
    m_parent->m_cat3Menu->setItemChecked( m_cat3, true );

    if ( rerender )
        renderView();
}


void
CollectionView::enableCat3Menu( bool enable )
{
    m_parent->m_cat3Menu->setItemEnabled( CollectionBrowser::IdAlbum, enable );
    m_parent->m_cat3Menu->setItemEnabled( CollectionBrowser::IdArtist, enable );
    m_parent->m_cat3Menu->setItemEnabled( CollectionBrowser::IdGenre, enable );
    m_parent->m_cat3Menu->setItemEnabled( CollectionBrowser::IdYear, enable );

    if( !enable ) {
        m_parent->m_cat3Menu->setItemChecked( m_cat3, false );
        m_parent->m_cat3Menu->setItemChecked( CollectionBrowser::IdNone, true );
        m_cat3 = CollectionBrowser::IdNone;
    }
}


void
CollectionView::invokeItem( QListViewItem* item ) //SLOT
{
    if ( !item )
        return;

    if( item->isExpandable() )
        item->setOpen( !item->isOpen() );
    else
        //direct play & prevent doubles in playlist
        Playlist::instance()->insertMedia( static_cast<Item*>( item )->url(), Playlist::Unique | Playlist::DirectPlay );

}


void
CollectionView::rmbPressed( QListViewItem* item, const QPoint& point, int ) //SLOT
{
    if ( item ) {
        KPopupMenu menu( this );

        int cat;
        if( item->depth() == 0 )
            cat = m_cat1;
        else if( item->depth() == 1 )
            cat = m_cat2;
        else if( item->depth() == 2 )
            cat = m_cat3;

        #ifdef AMAZON_SUPPORT
        enum Actions { APPEND, MAKE, QUEUE, BURN_ARTIST, BURN_ALBUM,
                       BURN_DATACD, BURN_AUDIOCD, COVER, INFO };
        #else
        enum Actions { APPEND, MAKE, QUEUE, BURN_ARTIST, BURN_ALBUM,
                       BURN_DATACD, BURN_AUDIOCD, INFO };
        #endif


        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Make Playlist" ), MAKE );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue After Current Track" ), QUEUE );

        menu.insertSeparator();

        if( cat == CollectionBrowser::IdArtist ) {
            menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn All Tracks by This Artist"), BURN_ARTIST );
            menu.setItemEnabled( BURN_ARTIST, K3bExporter::isAvailable() );
        }
        else if( cat == CollectionBrowser::IdAlbum ) {
            menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn This Album"), BURN_ALBUM );
            menu.setItemEnabled( BURN_ALBUM, K3bExporter::isAvailable() );
        }
        else if( !item->isExpandable() ) {
            menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Burn to CD as Data"), BURN_DATACD );
            menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
            menu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("Burn to CD as Audio"), BURN_AUDIOCD );
            menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
        }

        menu.insertSeparator();

        #ifdef AMAZON_SUPPORT
        menu.insertItem( SmallIconSet( "www" ), i18n( "&Fetch Cover Images" ), this, SLOT( fetchCover() ), 0, COVER );
        menu.setItemEnabled(COVER, cat == CollectionBrowser::IdAlbum );
        #endif
        menu.insertItem( SmallIconSet( "info" ), i18n( "View/Edit Meta Information..." ), this, SLOT( showTrackInfo() ), 0, INFO );

        menu.setItemEnabled( INFO, !item->isExpandable()  );

        switch( menu.exec( point ) ) {

            case APPEND:
                Playlist::instance()->insertMedia( listSelected(), Playlist::Append );
                break;
            case MAKE:
                Playlist::instance()->insertMedia( listSelected(), Playlist::Replace );
                break;
            case QUEUE:
                Playlist::instance()->insertMedia( listSelected(), Playlist::Queue );
                break;
            case BURN_ARTIST:
                K3bExporter::instance()->exportArtist( item->text(0) );
                break;
            case BURN_ALBUM:
                K3bExporter::instance()->exportAlbum( item->text(0) );
                break;
            case BURN_DATACD:
                K3bExporter::instance()->exportTracks( listSelected(), K3bExporter::DataCD );
                break;
            case BURN_AUDIOCD:
                K3bExporter::instance()->exportTracks( listSelected(), K3bExporter::AudioCD );
                break;
        }
    }
}


void
CollectionView::fetchCover() //SLOT
{
    #ifdef AMAZON_SUPPORT
    Item* item = static_cast<Item*>( currentItem() );
    if ( !item ) return;

    QString album = item->text(0);

    // find the first artist's name
    m_db->query( QString ( "SELECT DISTINCT artist.name FROM artist, album, tags "
                           "WHERE artist.id = tags.artist AND tags.album = album.id "
                           "AND album.name = '%1';" )
                           .arg( album ) );

    if ( !m_db->m_values.isEmpty() )
        m_db->fetchCover( this, m_db->m_values[0], album, false );
    #endif
}

void
CollectionView::showTrackInfo() //SLOT
{
    Item* item = static_cast<Item*>( currentItem() );
    if ( !item ) return;

    if ( !item->isExpandable() ) {
        TagDialog* dialog = new TagDialog( item->url() );
        dialog->show();
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// protected
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::timerEvent( QTimerEvent* )
{
    if ( AmarokConfig::monitorChanges() )
        scanMonitor();
}


void
CollectionView::customEvent( QCustomEvent *e )
{
    CollectionReader::ProgressEvent* p = dynamic_cast<CollectionReader::ProgressEvent*>( e );

    if ( p ) {
        switch ( p->state() ) {
        case CollectionReader::ProgressEvent::Start:
            m_progress->setProgress( 0 );
            m_progressBox->show();
            m_isScanning = true;
            break;

        case CollectionReader::ProgressEvent::Stop:
            m_progressBox->hide();
            break;

        case CollectionReader::ProgressEvent::Total:
            m_progress->setTotalSteps( p->value() );
            break;

        case CollectionReader::ProgressEvent::Progress:
            m_progress->setProgress( p->value() );
        }
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


void
CollectionView::cacheItem( QListViewItem* item )
{
    m_cacheItem.clear();

    if ( item )
    {
        switch ( item->depth() )
        {
            case 0: m_cacheItem << item->text( 0 );
                    break;

            case 1: m_cacheItem << item->parent()->text( 0 );
                    m_cacheItem << item->text( 0 );
                    break;

            case 2: m_cacheItem << item->parent()->parent()->text( 0 );
                    m_cacheItem << item->parent()->text( 0 );
                    m_cacheItem << item->text( 0 );
                    break;

            case 3: m_cacheItem << item->parent()->parent()->parent()->text( 0 );
                    m_cacheItem << item->parent()->parent()->text( 0 );
                    m_cacheItem << item->parent()->text( 0 );
                    m_cacheItem << item->text( 0 );
                    break;
            }
        }
}


KURL::List
CollectionView::listSelected() {
    //Here we determine the URLs of all selected items. We use two passes, one for the parent items,
    //and another one for the children.

    KURL::List list;
    QListViewItem* item;
    QStringList values;
    QueryBuilder qb;

    //first pass: parents
    for ( item = firstChild(); item; item = item->nextSibling() )
        if ( item->isSelected() )
        {
            bool sampler = item->text( 0 ) == i18n( "Various Artists" );
            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

            if ( !sampler )
                qb.addMatch( m_cat1, item->text( 0 ) );
            else
                qb.setOptions( QueryBuilder::optOnlyCompilations );

            qb.addFilter( m_cat1 | m_cat2 | m_cat3 | QueryBuilder::tabSong, m_filter );

            if ( !sampler ) qb.sortBy( m_cat1, QueryBuilder::valName );
            if ( m_cat2 != QueryBuilder::tabSong ) qb.sortBy( m_cat2, QueryBuilder::valName );
            if ( m_cat3 != QueryBuilder::tabSong ) qb.sortBy( m_cat3, QueryBuilder::valName );
            qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

            qb.setOptions( QueryBuilder::optRemoveDuplicates );
            values = qb.run();

            for ( uint i = 0; i < values.count(); i++ )
            {
                KURL tmp;
                tmp.setPath( values[i] );
                list << tmp;
            }
        }

    //second pass: category 1
    if ( m_cat2 == CollectionBrowser::IdNone )
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
                    bool sampler = item->text( 0 ) == i18n( "Various Artists" );
                    qb.clear();
                    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

                    if ( !sampler )
                        qb.addMatch( m_cat1, item->text( 0 ) );
                    else
                        qb.setOptions( QueryBuilder::optOnlyCompilations );

                    qb.addMatch( m_cat2, child->text( 0 ) );
                    qb.addFilter( m_cat1 | m_cat2 | m_cat3 | QueryBuilder::tabSong, m_filter );

                    if ( !sampler ) qb.sortBy( m_cat1, QueryBuilder::valName );
                    qb.sortBy( m_cat2, QueryBuilder::valName );
                    if ( m_cat3 != QueryBuilder::tabSong ) qb.sortBy( m_cat3, QueryBuilder::valName );
                    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

                    qb.setOptions( QueryBuilder::optRemoveDuplicates );
                    values = qb.run();

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
                if ( grandChild->isSelected() && !grandChild->isExpandable() && !child->parent()->isSelected() && !child->isSelected() )
                    list << static_cast<Item*>( grandChild ) ->url();

    //category 3
    if ( m_cat3 == CollectionBrowser::IdNone )
    {
        for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                for ( QListViewItem* grandChild = child->firstChild(); grandChild; grandChild = grandChild->nextSibling() )
                    for ( QListViewItem* grandChild2 = grandChild->firstChild(); grandChild2; grandChild2 = grandChild2->nextSibling() )
                        if ( grandChild2->isSelected() && !child->parent()->isSelected() && !child->isSelected() && !grandChild->isSelected() )
                            list << static_cast<Item*>( grandChild2 ) ->url();
    }
    else {

        for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                for ( QListViewItem* grandChild = child->firstChild(); grandChild; grandChild = grandChild->nextSibling() )
                    if ( grandChild->isSelected() && !grandChild->parent()->isSelected() )
                    {
                        bool sampler = item->text( 0 ) == i18n( "Various Artists" );
                        qb.clear();
                        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

                        if ( !sampler )
                            qb.addMatch( m_cat1, item->text( 0 ) );
                        else
                            qb.setOptions( QueryBuilder::optOnlyCompilations );

                        qb.addMatch( m_cat2, child->text( 0 ) );
                        qb.addMatch( m_cat3, grandChild->text( 0 ) );
                        qb.addFilter( m_cat1 | m_cat2 | m_cat3 | QueryBuilder::tabSong, m_filter );

                        if ( !sampler ) qb.sortBy( m_cat1, QueryBuilder::valName );
                        qb.sortBy( m_cat2, QueryBuilder::valName );
                        qb.sortBy( m_cat3, QueryBuilder::valName );
                        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

                        qb.setOptions( QueryBuilder::optRemoveDuplicates );
                        values = qb.run();

                        for ( uint i = 0; i < values.count(); i++ )
                        {
                            KURL tmp;
                            tmp.setPath( values[i] );
                            list << tmp;
                        }
                    }
    }

    //category 3
    for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                for ( QListViewItem* grandChild = child->firstChild(); grandChild; grandChild = grandChild->nextSibling() )
                    for ( QListViewItem* grandChild2 = grandChild->firstChild(); grandChild2; grandChild2 = grandChild2->nextSibling() )
                        if ( grandChild2->isSelected() && !child->parent()->isSelected() && !child->isSelected() && !grandChild->isSelected() )
                            list << static_cast<Item*>( grandChild2 ) ->url();

    return list;
}


QPixmap
CollectionView::iconForCategory( const int cat ) const
{
    QString icon;
    switch( cat )
    {
        case CollectionBrowser::IdAlbum:
            icon = "cdrom_unmount";
            break;

        case CollectionBrowser::IdArtist:
            icon = "personal";
            break;

        case CollectionBrowser::IdGenre:
            icon = "kfm";
            break;

        case CollectionBrowser::IdYear:
            icon = "history";
            break;
    }

    KIconLoader iconLoader;
    return iconLoader.loadIcon( icon, KIcon::Toolbar, KIcon::SizeSmall );
}


QString
CollectionView::captionForCategory( const int cat ) const
{
    switch( cat )
    {
        case CollectionBrowser::IdAlbum:
            return i18n( "Album" );
            break;

        case CollectionBrowser::IdArtist:
            return i18n( "Artist" );
            break;

        case CollectionBrowser::IdGenre:
            return i18n( "Genre" );
            break;

        case CollectionBrowser::IdYear:
            return i18n( "Year" );
            break;
    }

    return QString::null;
}


#include "collectionbrowser.moc"


