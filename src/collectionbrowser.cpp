// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#include "config.h"

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
        m_searchEdit = new KLineEdit( hbox, "filter_edit" );
        m_searchEdit->installEventFilter( this );

        hbox->setMargin( 1 );
        m_searchEdit->setFrame( QFrame::Sunken );
        connect( button, SIGNAL(clicked()), this, SLOT(clearFilter()) );

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

    {
        //set the lineEdit to initial state
        QEvent e( QEvent::FocusOut );
        eventFilter( m_searchEdit, &e );
    }

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

    m_view->cat1Menu( m_view->idForCat( m_view->m_category1 ), false );
    m_view->cat2Menu( m_view->idForCat( m_view->m_category2 ), false );
    m_view->cat3Menu( m_view->idForCat( m_view->m_category3 ), true );

    connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );

    // This is used when the collection folders were changed in the first-run wizard
    connect( kapp, SIGNAL( sigScanCollection() ), m_view, SLOT( scan() ) );

    setFocusProxy( m_view ); //default object to get focus
    setMinimumWidth( toolbar->sizeHint().width() + 2 ); //set a reasonable minWidth
}

void
CollectionBrowser::slotSetFilterTimeout() //SLOT
{
    if ( m_timer->isActive() ) m_timer->stop();
    m_timer->start( 180, true );
}

void
CollectionBrowser::slotSetFilter() //SLOT
{
    m_view->setFilter( m_searchEdit->text() );
    m_view->renderView();
}

void
CollectionBrowser::clearFilter()
{
    m_searchEdit->clear();
    m_timer->stop();
    slotSetFilter();
    if( !m_searchEdit->hasFocus() ) {
        //set the lineEdit to initial state
        QEvent e( QEvent::FocusOut );
        eventFilter( m_searchEdit, &e);
    }
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

bool CollectionBrowser::eventFilter( QObject *o, QEvent *e )
{
    if( o == m_searchEdit ) {
        switch( e->type() ) {
           case QEvent::FocusIn:
               if( m_view->filter().isEmpty() ) {
                   m_searchEdit->clear();
                   m_timer->stop();
                   m_searchEdit->setPaletteForegroundColor( colorGroup().text() );
                   return FALSE;
               }

            case QEvent::FocusOut:
                if( m_view->filter().isEmpty() ) {
                    m_searchEdit->setPalette( palette() );
                    m_searchEdit->setPaletteForegroundColor( palette().color( QPalette::Disabled, QColorGroup::Text ) );
                    m_searchEdit->setText( i18n("Filter here...") );
                    m_timer->stop();
                    return FALSE;
                }

            default:
                return FALSE;
        };
    }

    return FALSE;
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
        m_category1 = config->readEntry( "Category1", i18n( "Artist" ) );
        m_category2 = config->readEntry( "Category2", i18n( "None" ) );
        m_category3 = config->readEntry( "Category3", i18n( "None" ) );

        addColumn( m_category1 );
    //</READ CONFIG>

    //<OPEN DATABASE>
        //optimization for speeding up SQLite
        m_db->query( "PRAGMA default_synchronous = OFF;" );

        //remove database file if version is incompatible
        if ( ( config->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION ) || ( !m_db->isDbValid() ) )
        {
            kdDebug() << "Rebuilding database!" << endl;
            m_db->dropTables();
            m_db->createTables();
        }
        if ( ( config->readNumEntry( "Database Stats Version", 0 ) != DATABASE_STATS_VERSION ) || ( !m_db->isDbValid() ) )
        {
            kdDebug() << "Rebuilding stats-database!" << endl;
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
    //</OPEN DATABASE>

    //<PROGRESS BAR>
        m_progressBox = new QHBox( m_parent );
        QPushButton* button = new QPushButton( SmallIcon( "button_cancel" ), i18n( "Abort" ), m_progressBox );
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
    connect( this,           SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( doubleClicked( QListViewItem*, const QPoint&, int ) ) );
    connect( this,           SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,             SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );

    startTimer( MONITOR_INTERVAL );
}


CollectionView::~CollectionView() {
    kdDebug() << k_funcinfo << endl;

    KConfig* const config = amaroK::config( "Collection Browser" );
    config->writeEntry( "Category1", m_category1 );
    config->writeEntry( "Category2", m_category2 );
    config->writeEntry( "Category3", m_category3 );
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
        m_insertdb->dropTables();
        this->clear();
        emit sigScanDone();
    }
    else if ( !m_isScanning )
    {
        m_isScanning = true;
        m_parent->m_scanAction->setEnabled( false );

        m_insertdb->scan( AmarokConfig::collectionFolders(), AmarokConfig::scanRecursively(),
                          AmarokConfig::importPlaylists() );

        emit sigScanStarted();
        amaroK::StatusBar::instance()->message( i18n("Building Collection...") );
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
    QPixmap pixmap = iconForCat( m_category1 );

    //query database for all records with the specified category
    QStringList values;
    QStringList names;
    bool addedVA = false;
    m_db->retrieveFirstLevel( tableForCat( m_category1 ), tableForCat( m_category2 ), tableForCat( m_category3 ), m_filter, &values, &names );

    for ( uint i = 0; i < values.count(); i += 2 )
    {
        if ( values[i].isEmpty() )
          values[i] = i18n( "Unknown" );

        if ( m_category1 == i18n( "Artist" ) && ( values[i + 1] == "1" || values[i] == i18n( "Various Artists" ) ) )
        {
            addedVA = true;
            continue;
        }

        KListViewItem* item = new KListViewItem( this );
        item->setExpandable( true );
        item->setDragEnabled( true );
        item->setDropEnabled( false );
        item->setText( 0, values[ i ] );
        item->setPixmap( 0, pixmap );
    }

    if ( addedVA )
    {
        KListViewItem* item = new KListViewItem( this );
        item->setExpandable( true );
        item->setDragEnabled( true );
        item->setDropEnabled( false );
        item->setText( 0, i18n( "Various Artists" ) );
        item->setPixmap( 0, pixmap );
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

    emit sigScanDone();
}


void
CollectionView::slotExpand( QListViewItem* item )  //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !item ) return;

    QStringList values;
    QStringList names;

    QString category;

    if( item->depth() == 0 ) {
        m_db->retrieveSecondLevel( item->text( 0 ), tableForCat( m_category1 ), tableForCat( m_category2 ), tableForCat( m_category3 ), m_filter, &values, &names );
        category = m_category2;
    } else if( item->depth() == 1 ) {
        m_db->retrieveThirdLevel( item->parent()->text( 0 ), item->text( 0 ), tableForCat( m_category1 ), tableForCat( m_category2 ), tableForCat( m_category3 ), m_filter, &values, &names );
        category = m_category3;
    } else if( item->depth() == 2 ) {
        m_db->retrieveFourthLevel( item->parent()->parent()->text( 0 ), item->parent()->text( 0 ), item->text( 0 ),  tableForCat( m_category1 ), tableForCat( m_category2 ), tableForCat( m_category3 ), m_filter, &values, &names );
        category = i18n("None");
    }

    QPixmap pixmap;
    bool expandable = category != i18n( "None" );
    if ( expandable )
        pixmap = iconForCat( category );

    for ( uint i = 0; i < values.count(); i += 2 )
    {
        Item* child = new Item( item );
        child->setDragEnabled( true );
        child->setDropEnabled( false );
        child->setText( 0, values[ i + 0 ] );
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
    m_parent->m_cat1Menu->setItemChecked( idForCat( m_category1 ), false ); //uncheck old item
    m_parent->m_cat2Menu->setItemEnabled( idForCat( m_category1 ), true );  //enable old item
    m_category1 = catForId( id );
    setColumnText( 0, m_category1 );
    m_parent->m_cat1Menu->setItemChecked( idForCat( m_category1 ), true );

    //prevent choosing the same category in both menus
    m_parent->m_cat2Menu->setItemEnabled( id , false );
    m_parent->m_cat3Menu->setItemEnabled( id , false );
    //m_parent->m_cat3Menu->setItemEnabled( idForCat( m_category2 ) , false );

    //if this item is checked in second menu, uncheck it
    if ( m_parent->m_cat2Menu->isItemChecked( id ) ) {
        m_parent->m_cat2Menu->setItemChecked( id, false );
        m_parent->m_cat2Menu->setItemChecked( CollectionBrowser::IdNone, true );
        m_category2 = catForId( CollectionBrowser::IdNone );
        enableCat3Menu( false );
    }
    //if this item is checked in third menu, uncheck it
    if ( m_parent->m_cat3Menu->isItemChecked( id ) ) {
        m_parent->m_cat3Menu->setItemChecked( id, false );
        m_parent->m_cat3Menu->setItemChecked( CollectionBrowser::IdNone, true );
        m_category3 = catForId( CollectionBrowser::IdNone );
    }

    if ( rerender )
        renderView();
}


void
CollectionView::cat2Menu( int id, bool rerender )  //SLOT
{
    m_parent->m_cat2Menu->setItemChecked( idForCat( m_category2 ), false ); //uncheck old item
    m_parent->m_cat3Menu->setItemEnabled( idForCat( m_category3 ), true );  //enable old item
    m_category2 = catForId( id );
    m_parent->m_cat2Menu->setItemChecked( idForCat( m_category2 ), true );

    enableCat3Menu(  id != CollectionBrowser::IdNone );

    //prevent choosing the same category in both menus
    m_parent->m_cat3Menu->setItemEnabled( idForCat( m_category1 ) , false );
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
    m_parent->m_cat3Menu->setItemChecked( idForCat( m_category3 ), false ); //uncheck old item
    m_category3 = catForId( id );
    m_parent->m_cat3Menu->setItemChecked( idForCat( m_category3 ), true );

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
        m_parent->m_cat3Menu->setItemChecked( idForCat( m_category3 ), false );
        m_parent->m_cat3Menu->setItemChecked( CollectionBrowser::IdNone, true );
        m_category3 = catForId( CollectionBrowser::IdNone );
    }
}


void
CollectionView::doubleClicked( QListViewItem* item, const QPoint&, int ) //SLOT
{
    if ( !item )
        return;

    if( item->isExpandable() )
        item->setOpen( !item->isOpen() );
    else
        //direct play & prevent doubles in playlist
        Playlist::instance()->appendMedia( static_cast<Item*>( item )->url(), true, true );

}


void
CollectionView::rmbPressed( QListViewItem* item, const QPoint& point, int ) //SLOT
{
    if ( item ) {
        KPopupMenu menu( this );

        QString category;
        if( item->depth() == 0 )
            category = m_category1;
        else if( item->depth() == 1 )
            category = m_category2;
        else if( item->depth() == 2 )
            category = m_category3;

        #ifdef AMAZON_SUPPORT
        enum Actions { APPEND, MAKE, QUEUE, BURN_ARTIST, BURN_ALBUM,
                       BURN_DATACD, BURN_AUDIOCD, COVER, INFO };
        #else
        enum Actions { APPEND, MAKE, QUEUE, BURN_ARTIST, BURN_ALBUM,
                       BURN_DATACD, BURN_AUDIOCD, INFO };
        #endif


        menu.insertItem( i18n( "&Append to the Playlist" ), APPEND );
        menu.insertItem( i18n( "&Make Playlist" ), MAKE );
        menu.insertItem( i18n( "&Queue After Current Track" ), QUEUE );

        if( category == i18n("Artist") ) {
            menu.insertItem( i18n("Burn All Tracks by This Artist"), BURN_ARTIST );
            menu.setItemEnabled( BURN_ARTIST, K3bExporter::isAvailable() );
        }
        else if( category == i18n("Album") ) {
            menu.insertItem( i18n("Burn This Album"), BURN_ALBUM );
            menu.setItemEnabled( BURN_ALBUM, K3bExporter::isAvailable() );
        }
        else if( !item->isExpandable() ) {
            menu.insertSeparator();
            menu.insertItem( i18n("Burn to CD as data"), BURN_DATACD );
            menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
            menu.insertItem( i18n("Burn to CD as audio"), BURN_AUDIOCD );
            menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
        }

        menu.insertSeparator();

        #ifdef AMAZON_SUPPORT
        menu.insertItem( i18n( "&Fetch Cover Images" ), this, SLOT( fetchCover() ), 0, COVER );
        menu.setItemEnabled(COVER, category == i18n("Album") );
        #endif
        menu.insertItem( i18n( "Edit Meta Information..." ), this, SLOT( showTrackInfo() ), 0, INFO );

        menu.setItemEnabled( INFO, !item->isExpandable()  );

        switch( menu.exec( point ) ) {

            case APPEND:
                Playlist::instance()->appendMedia( listSelected() );
                break;
            case MAKE:
                Playlist::instance()->clear(); //FALL THROUGH
            case QUEUE:
                Playlist::instance()->queueMedia( listSelected() );
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
    QStringList names;

    //first pass: parents
    for ( item = firstChild(); item; item = item->nextSibling() )
        if ( item->isSelected() )
        {
            values.clear();
            names.clear();

            m_db->retrieveFirstLevelURLs( item->text( 0 ), tableForCat( m_category1 ), tableForCat( m_category2 ),
                                                            tableForCat( m_category3 ), m_filter, &values, &names );
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

                    m_db->retrieveSecondLevelURLs( item->text( 0 ), child->text( 0 ), tableForCat( m_category1 ), tableForCat( m_category2 ), tableForCat( m_category3 ), m_filter, &values, &names );
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
    if ( m_category3 == i18n( "None" ) )
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
                        values.clear();
                        names.clear();

                        m_db->retrieveThirdLevelURLs( item->text( 0 ), child->text( 0 ), grandChild->text(0), tableForCat( m_category1 ), tableForCat( m_category2 ), tableForCat( m_category3 ),  m_filter, &values, &names );
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


