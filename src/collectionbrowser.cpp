// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#include "app.h"            //makePlaylist()
#include "collectionbrowser.h"
#include "directorylist.h"
#include "metabundle.h"
#include "sqlite/sqlite.h"
#include "statusbar.h"

#include <unistd.h>         //CollectionView ctor

#include <qcstring.h>
#include <qdragobject.h>
#include <qmessagebox.h>
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
#include <ktoolbarbutton.h> //ctor
#include <kurldrag.h>       //dragObject()


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
//                        new QLabel( i18n( "Search for:" ), hbox );
        button       = new QToolButton( hbox );
        m_searchEdit = new KLineEdit( hbox );

        hbox->setMargin( 4 );
        button->setIconSet( SmallIconSet( "locationbar_erase.png" ) );
        m_searchEdit->setFrame( QFrame::Sunken );
        connect( button, SIGNAL(clicked()), m_searchEdit, SLOT(clear()) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_searchEdit, i18n( "Enter space-separated terms to filter collection" ) );
    } //</Search LineEdit>
    
    m_view = new CollectionView( this );
    //m_view->setMargin( 2 );

    m_actionsMenu->insertItem( i18n( "Configure Folders" ), m_view, SLOT( setupDirs() ) );
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

    m_view->cat1Menu( m_view->idForCat( m_view->m_category1 ) );
    m_view->cat2Menu( m_view->idForCat( m_view->m_category2 ) );

    timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( slotSetFilter() ) );

    connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ),
             this,           SLOT( slotSetFilterTimeout() ) );

    setFocusProxy( m_view ); //default object to get focus
    setMinimumWidth( menu->sizeHint().width() + 2 ); //set a reasonable minWidth
}


void
CollectionBrowser::slotSetFilterTimeout() //slot
{
    if ( timer->isActive() ) timer->stop();
    timer->start( 180, true );
}


void
CollectionBrowser::slotSetFilter() //slot
{
    m_view->setFilter( m_searchEdit->text() );
    m_view->renderView();
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionView
//////////////////////////////////////////////////////////////////////////////////////////

CollectionDB* CollectionView::m_db;
CollectionDB* CollectionView::m_insertdb;

CollectionView::CollectionView( CollectionBrowser* parent )
        : KListView( parent )
        , m_parent( parent )
{
    kdDebug() << k_funcinfo << endl;

    setSelectionMode( QListView::Extended );
    setItemsMovable( false );
    setRootIsDecorated( true );
    setShowSortIndicator( true );
    setFullWidth( true );
    setAcceptDrops( false );
    setSorting( -1 );

    //<read config>
        KConfig* config = KGlobal::config();
        config->setGroup( "Collection Browser" );

        m_dirs = config->readListEntry( "Folders" );
        m_category1 = config->readEntry( "Category1", i18n( "Artist" ) );
        m_category2 = config->readEntry( "Category2", i18n( "None" ) );
        addColumn( m_category1 );
        m_recursively = config->readBoolEntry( "Scan Recursively", true );
        m_monitor = config->readBoolEntry( "Monitor Changes", false );
    //</read config>

    //<open database>
        m_db = new CollectionDB();
        if ( !m_db )
            kdWarning() << k_funcinfo << "Could not open SQLite database\n";

        //optimization for speeding up SQLite
        m_db->execSql( "PRAGMA default_synchronous = OFF;" );
        m_db->execSql( "PRAGMA default_cache_size = 4000;" );

        //remove database file if version is incompatible
        if ( config->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION )
        {
            m_db->dropTables();
            m_db->createTables();
            m_insertdb = new CollectionDB();
            scan();
        } else
        {
            m_insertdb = new CollectionDB();
            scanModifiedDirs();
        }

        if ( m_monitor )
            m_insertdb->addCollectionToWatcher();

    //</open database>

    connect( m_insertdb, SIGNAL( scanDone() ),
             this,         SLOT( scanDone() ) );
    connect( this,       SIGNAL( expanded( QListViewItem* ) ),
             this,         SLOT( slotExpand( QListViewItem* ) ) );
    connect( this,       SIGNAL( collapsed( QListViewItem* ) ),
             this,         SLOT( slotCollapse( QListViewItem* ) ) );
    connect( this,       SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
             this,         SLOT( doubleClicked( QListViewItem*, const QPoint&, int ) ) );
    connect( this,       SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,         SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );

    renderView();
}


CollectionView::~CollectionView() {
    kdDebug() << k_funcinfo << endl;

    KConfig* config = KGlobal::config();
    config->setGroup( "Collection Browser" );
    config->writeEntry( "Folders", m_dirs );
    config->writeEntry( "Category1", m_category1 );
    config->writeEntry( "Category2", m_category2 );
    config->writeEntry( "Scan Recursively", m_recursively );
    config->writeEntry( "Monitor Changes", m_monitor );
    config->writeEntry( "Database Version", DATABASE_VERSION );

    delete m_db;
}

//////////////////////////////////////////////////////////////////////////////////////////
// private slots
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::setupDirs()  //SLOT
{
    DirectoryList list( m_dirs, m_recursively, m_monitor );
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
    m_parent->m_actionsMenu->setItemEnabled( m_parent->IdScan, false );
    m_insertdb->scan( m_dirs, m_recursively );
}


void
CollectionView::scanModifiedDirs()  //SLOT
{
    m_parent->m_actionsMenu->setItemEnabled( m_parent->IdScan, false );
    m_insertdb->scanModifiedDirs( m_recursively );
}


void
CollectionView::renderView( )  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    clear();

    //query database for all records with the specified category
    QString filterToken;
    if ( m_filter != "" )
        filterToken = "AND ( " + m_category1.lower() + ".name LIKE '%" + m_db->escapeString( m_filter ) + "%' OR tags.title LIKE '%" + m_db->escapeString( m_filter ) + "%' )";

    QString command = "SELECT DISTINCT " + m_category1.lower() + ".name FROM tags, " + m_category1.lower()
                    + " WHERE tags." + m_category1.lower() + "=" + m_category1.lower() + ".id " + filterToken
                    + " ORDER BY " + m_category1.lower() + ".name DESC;";

    QStringList values;
    QStringList names;
    m_db->execSql( command, &values, &names );

    QPixmap pixmap = iconForCat( m_category1 );

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
CollectionView::scanDone() //slot
{
    kdDebug() << k_funcinfo << endl;

    // we need to reconnect to the db after every scan, since sqlite is not able to keep
    // the tables synced for multiple threads.
    delete m_db;
    m_db = new CollectionDB();

    renderView();
    
    if ( m_monitor )
        m_insertdb->addCollectionToWatcher();

    m_parent->m_actionsMenu->setItemEnabled( m_parent->IdScan, true );
}


void
CollectionView::slotExpand( QListViewItem* item )  //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !item ) return ;

    QString filterToken = QString( "" );
    if ( m_filter != "" )
        filterToken = "AND ( " + m_category1.lower() + ".name LIKE '%" + m_db->escapeString( m_filter ) + "%' OR tags.title LIKE '%" + m_db->escapeString( m_filter ) + "%' )";

    if  ( item->depth() == 0 ) {
        QString command;
        if ( m_category2 == i18n( "None" ) ) {
            QString id = QString::number( m_db->getValueID( m_category1.lower(), item->text( 0 ), false ) );

            command = "SELECT DISTINCT tags.title, tags.url FROM tags, " + m_category1.lower() + " WHERE tags."
                    + m_category1.lower() + "=" + id + " " + filterToken + " ORDER BY tags.track DESC, tags.url DESC;";
        }
        else {
            filterToken = "AND ( " + m_category1.lower() + ".id = tags." + m_category1.lower() + " AND " + m_category1.lower() + ".name LIKE '%" + m_db->escapeString( m_filter ) + "%' OR tags.title LIKE '%" + m_db->escapeString( m_filter ) + "%' )";
            QString id = QString::number( m_db->getValueID( m_category1.lower(), item->text( 0 ), false ) );

            command = "SELECT DISTINCT " + m_category2.lower() + ".name, '0' FROM tags, " + m_category2.lower()
                    + ", " + m_category1.lower() + " WHERE tags." + m_category2.lower() + "=" + m_category2.lower()
                    + ".id AND tags." + m_category1.lower() + "=" + id + " " + filterToken + " ORDER BY "
                    + m_category2.lower() + ".name DESC;";
        }

        QStringList values;
        QStringList names;
        m_db->execSql( command, &values, &names );

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
        QString id = QString::number( m_db->getValueID( m_category1.lower(), item->parent()->text( 0 ), false ) );
        QString id_sub = QString::number( m_db->getValueID( m_category2.lower(), item->text( 0 ), false ) );

        QString command = "SELECT tags.title, tags.url FROM tags, " + m_category1.lower() + " WHERE tags."
                        + m_category1.lower() + " = " + id + " AND tags." + m_category2.lower() + " = " + id_sub
                        + " AND tags." + m_category1.lower() + " = " + m_category1.lower() + ".id " + filterToken
                        + " ORDER BY tags.track DESC, tags.url DESC;";

        QStringList values;
        QStringList names;
        m_db->execSql( command, &values, &names );

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
CollectionView::cat1Menu( int id )  //SLOT
{
    m_parent->m_cat1Menu->setItemChecked( idForCat( m_category1 ), false ); //uncheck old item
    m_parent->m_cat2Menu->setItemEnabled( idForCat( m_category1 ), true );  //enable old item
    m_category1 = catForId( id );
    setColumnText( 0, m_category1 );
    m_parent->m_cat1Menu->setItemChecked( idForCat( m_category1 ), true );

    //prevent choosing the same category in both menus
    m_parent->m_cat2Menu->setItemEnabled( id , false );

    renderView();
}


void
CollectionView::cat2Menu( int id )  //SLOT
{
    m_parent->m_cat2Menu->setItemChecked( idForCat( m_category2 ), false ); //uncheck old item
    m_parent->m_cat1Menu->setItemEnabled( idForCat( m_category2 ), true );  //enable old item
    m_category2 = catForId( id );
    m_parent->m_cat2Menu->setItemChecked( idForCat( m_category2 ), true );

    //prevent choosing the same category in both menus
    m_parent->m_cat1Menu->setItemEnabled( id , false );

    renderView();
}


void
CollectionView::doubleClicked( QListViewItem* item, const QPoint& point, int ) //SLOT
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

        menu.insertItem( i18n( "Make Playlist" ), this, SLOT( makePlaylist() ) );
        menu.insertItem( i18n( "Add to Playlist" ), this, SLOT( addToPlaylist() ) );

        if ( ( item->depth() && m_category2 == i18n( "None" ) ) || item->depth() == 2 )
            menu.insertItem( i18n( "Track Information" ), this, SLOT( showTrackInfo() ) );

        menu.exec( point );
    }
}


void
CollectionView::makePlaylist() //slot
{
    pApp->actionCollection()->action( "playlist_clear" )->activate();
    pApp->insertMedia( listSelected() );
}


void
CollectionView::addToPlaylist() //slot
{
    pApp->insertMedia( listSelected() );
}


void
CollectionView::showTrackInfo() //slot
{
    Item* item = static_cast<Item*>( currentItem() );
    if ( !item ) return;

    if ( m_category2 == i18n( "None" ) || item->depth() == 2 ) {
        QString command = QString
                          ( "SELECT DISTINCT artist.name, album.name, genre.name, year.name, comment FROM tags, artist, album, genre, year "
                            "WHERE artist.id = tags.artist AND album.id = tags.album AND genre.id = tags.genre AND year.id = tags.year AND tags.url = '%1';" )
                            .arg( m_db->escapeString( item->url().path() ) );

        QStringList values;
        QStringList names;
        m_db->execSql( command, &values, &names );
        if ( values.isEmpty() ) return;

        QString str  = "<html><body><table width=\"100%\" border=\"1\">";
        QString body = "<tr><td>%1</td><td>%2</td></tr>";

        str += body.arg( i18n( "Title" ),  item->text( 0 ) );
        str += body.arg( i18n( "Artist" ), values[0] );
        str += body.arg( i18n( "Album" ),  values[1] );
        str += body.arg( i18n( "Genre" ),  values[2] );
        str += body.arg( i18n( "Year" ),  values[3] );
        str += body.arg( i18n( "Comment" ),values[4] );
    //     str += body.arg( i18n( "Length" ), mb.prettyLength() );
    //     str += body.arg( i18n( "Bitrate" ),mb.prettyBitrate() );
    //     str += body.arg( i18n( "Samplerate" ), mb.prettySampleRate() );

        str.append( "</table></body></html>" );

        QMessageBox box( i18n( "Meta Information" ), str, QMessageBox::Information,
                        QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton,
                        0, 0, true, Qt::WStyle_DialogBorder );
        box.setTextFormat( Qt::RichText );
        box.exec();
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

    //first pass: parents
    for ( item = firstChild(); item; item = item->nextSibling() )
        if ( item->isSelected() ) {
            QString filterToken;
            if ( m_filter != "" )
                filterToken = "AND ( " + m_category1.lower() + ".name LIKE '%" + m_db->escapeString( m_filter ) + "%' OR tags.title LIKE '%" + m_db->escapeString( m_filter ) + "%' )";

            //query database for all tracks in our sub-category
            QString id = QString::number( m_db->getValueID( m_category1.lower(), item->text( 0 ), false ) );
            QString command = "SELECT DISTINCT tags.url FROM tags, " + m_category1.lower() + " WHERE tags."
                            + m_category1.lower() + "=" + id + " " + filterToken + " ORDER BY tags.dir, tags.track, tags.url;";

            QStringList values;
            QStringList names;
            m_db->execSql( command, &values, &names );

            for ( uint i = 0; i < values.count(); i++ ) {
                KURL tmp;
                tmp.setPath( values[i] );
                list << tmp;
            }
        }

    //second pass: category 1
    if ( m_category2 == i18n( "None" ) ) {
        for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                if ( child->isSelected() )
                    list << static_cast<Item*>( child ) ->url();
    }
    else {
        for ( item = firstChild(); item; item = item->nextSibling() )
            for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
                if ( child->isSelected() ) {
                    QString filterToken;
                    filterToken = "AND ( " + m_category1.lower() + ".id = tags." + m_category1.lower() + " AND " + m_category1.lower() + ".name LIKE '%" + m_db->escapeString( m_filter ) + "%' OR tags.title LIKE '%" + m_db->escapeString( m_filter ) + "%' )";
                    QString id = QString::number( m_db->getValueID( m_category1.lower(), item->text( 0 ), false ) );
                    QString id_sub = QString::number( m_db->getValueID( m_category2.lower(), child->text( 0 ), false ) );

                    QString command = "SELECT DISTINCT tags.url FROM tags, " + m_category1.lower() + " WHERE tags."
                                    + m_category2.lower() + "=" + id_sub + " AND tags." + m_category1.lower() + "="
                                    + id + " " + filterToken + " ORDER BY tags.track, tags.url;";

                    QStringList values;
                    QStringList names;
                    m_db->execSql( command, &values, &names );

                    for ( uint i = 0; i < values.count(); i++ ) {
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
                if ( grandChild->isSelected() )
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


#include "collectionbrowser.moc"


