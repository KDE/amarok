// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#include "collectionbrowser.h"
#include "directorylist.h"
#include "metabundle.h"
#include "app.h"      //makePlaylist()
#include "sqlite/sqlite.h"
#include "statusbar.h"
#include "threadweaver.h"

#include <unistd.h>         //CollectionView ctor

#include <qcstring.h>
#include <qdragobject.h>
#include <qmessagebox.h>
#include <qptrlist.h>

#include <kactioncollection.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <kglobal.h>
#include <kiconloader.h>    //renderView()
#include <klocale.h>
#include <kmenubar.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>
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

    QHBox * hbox2 = new QHBox( this );
    hbox2->setSpacing( 4 );
    new QLabel( "Search for:", hbox2 );
    m_searchEdit = new KLineEdit( hbox2 );

    m_view = new CollectionView( this );
    //m_view->setMargin( 2 );

    m_actionsMenu->insertItem( i18n( "Configure Folders" ), m_view, SLOT( setupDirs() ) );
    m_actionsMenu->insertItem( i18n( "Start Scan" ), m_view, SLOT( scan() ) );

    m_cat1Menu ->insertItem( "Album", m_view, SLOT( cat1Menu( int ) ), 0, IdAlbum );
    m_cat1Menu ->insertItem( "Artist", m_view, SLOT( cat1Menu( int ) ), 0, IdArtist );
    m_cat1Menu ->insertItem( "Genre", m_view, SLOT( cat1Menu( int ) ), 0, IdGenre );
    m_cat1Menu ->insertItem( "Year", m_view, SLOT( cat1Menu( int ) ), 0, IdYear );

    m_cat2Menu ->insertItem( "None", m_view, SLOT( cat2Menu( int ) ), 0, IdNone );
    m_cat2Menu ->insertSeparator();
    m_cat2Menu ->insertItem( "Album", m_view, SLOT( cat2Menu( int ) ), 0, IdAlbum );
    m_cat2Menu ->insertItem( "Artist", m_view, SLOT( cat2Menu( int ) ), 0, IdArtist );
    m_cat2Menu ->insertItem( "Genre", m_view, SLOT( cat2Menu( int ) ), 0, IdGenre );
    m_cat2Menu ->insertItem( "Year", m_view, SLOT( cat2Menu( int ) ), 0, IdYear );

    m_cat1Menu->setItemChecked( m_view->idForCat( m_view->m_category1 ), true );
    m_cat2Menu->setItemChecked( m_view->idForCat( m_view->m_category2 ), true );

    connect( m_searchEdit, SIGNAL( returnPressed() ),
             this,           SLOT( slotSetFilter() ) );

    setFocusProxy( m_view ); //default object to get focus
    setMinimumWidth( menu->sizeHint().width() + 2 ); //set a reasonable minWidth
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
        , m_weaver( new ThreadWeaver( this ) )
        , m_dirWatch( new KDirWatch( this ) )
{
    kdDebug() << k_funcinfo << endl;

    setSelectionMode( QListView::Extended );
    setItemsMovable( false );
    setRootIsDecorated( true );
    setShowSortIndicator( true );
    setFullWidth( true );
    setAcceptDrops( false );

    //<read config>
        KConfig* config = KGlobal::config();
        config->setGroup( "Collection Browser" );

        m_dirs = config->readListEntry( "Folders" );
        m_category1 = config->readEntry( "Category1", "Artist" );
        m_category2 = config->readEntry( "Category2", "None" );
        addColumn( m_category1 );
        m_recursively = config->readBoolEntry( "Scan Recursively", true );
        m_monitor = config->readBoolEntry( "Monitor Changes", false );
    //</read config>

    //<open database>
        m_databasePath = ( KGlobal::dirs() ->saveLocation( "data", kapp->instanceName() + "/" )
                       + "collection.db" ).local8Bit();
        //remove database file if version is incompatible
        if ( config->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION )
            ::unlink( m_databasePath );

        m_db = new CollectionDB( m_databasePath );
        if ( !m_db )
            kdWarning() << k_funcinfo << "Could not open SQLite database\n";
        //optimization for speeding up SQLite
        m_db->execSql( "PRAGMA default_synchronous = OFF;" );
        m_db->execSql( "PRAGMA default_cache_size = 4000;" );

        m_db->createTables();
    //</open database>

    connect( this,       SIGNAL( tagsReady() ),
             this,         SLOT( renderView() ) );
    connect( this,       SIGNAL( expanded( QListViewItem* ) ),
             this,         SLOT( slotExpand( QListViewItem* ) ) );
    connect( this,       SIGNAL( collapsed( QListViewItem* ) ),
             this,         SLOT( slotCollapse( QListViewItem* ) ) );
    connect( this,       SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
             this,         SLOT( rmbPressed( QListViewItem*, const QPoint&, int ) ) );
    connect( m_dirWatch, SIGNAL( dirty( const QString& ) ),
             this,         SLOT( dirDirty( const QString& ) ) );

    renderView();

    if ( m_monitor ) {
        m_dirWatch->startScan();
        scan();
    }
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
// private
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

    //we must re-scan everything, when a change was made to the folder list
    if ( result.addedDirs.count() || result.removedDirs.count() ) {
        //destroy KDirWatch and create new one, to make it forget all directories
        delete m_dirWatch;
        m_dirWatch = new KDirWatch( this );
        scan();
    }

    if ( m_monitor )
        m_dirWatch->startScan();
    else
        m_dirWatch->stopScan();
}


void
CollectionView::scan()  //SLOT
{
    m_insertdb = new CollectionDB( m_databasePath );

    m_weaver->append( new CollectionReader( this, amaroK::StatusBar::self(), m_dirs, m_recursively ) );

    if ( m_monitor )
        m_dirWatch->startScan();
}


void
CollectionView::dirDirty( const QString& path )
{
    kdDebug() << k_funcinfo << "Dirty: " << path << endl;

    //remove old records with the same dir as our dirty dir, to prevent dupes
    QString command = QString
                      ( "DELETE FROM tags WHERE dir = '%1';" )
                      .arg( m_db->escapeString( path ) );
    m_db->execSql( command );

    m_weaver->append( new CollectionReader( this, amaroK::StatusBar::self(), path, false ) );
}


void
CollectionView::renderView( )  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    clear();

    //query database for all records with the specified category
    QString filterToken = QString( "" );
    if ( m_filter != "" )
        filterToken = QString
                      ( "AND ( %1.name LIKE '\%%2\%' OR tags.title LIKE '\%%3\%' )" )
                      .arg( m_category1.lower() )
                      .arg( m_db->escapeString( m_filter ) )
                      .arg( m_db->escapeString( m_filter ) );

    QString command = QString
                      ( "SELECT DISTINCT %1.name FROM tags, %2 WHERE tags.%3=%4.id %5;" )
                      .arg( m_category1.lower() )
                      .arg( m_category1.lower() )
                      .arg( m_category1.lower() )
                      .arg( m_category1.lower() )
                      .arg( filterToken );

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
CollectionView::slotExpand( QListViewItem* item )  //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !item ) return ;

    kdDebug() << "item depth: " << item->depth() << endl;
    QString filterToken = QString( "" );
    if ( m_filter != "" )
        filterToken = QString
                      ( "AND ( %1.name LIKE '\%%2\%' OR tags.title LIKE '\%%3\%' )" )
                      .arg( m_category1.lower() )
                      .arg( m_db->escapeString( m_filter ) )
                      .arg( m_db->escapeString( m_filter ) );

    if  ( item->depth() == 0 ) {
        QString command;
        if ( m_category2 == "None" ) {
            QString id = QString::number( m_db->getValueID( m_category1.lower(), item->text( 0 ), false ) );

            command = QString
                      ( "SELECT DISTINCT tags.title, tags.url FROM tags, %1 WHERE tags.%2=%3 %4 ORDER BY track;" )
                      .arg( m_category1.lower() )
                      .arg( m_category1.lower() )
                      .arg( id )
                      .arg( filterToken );
        }
        else {
            filterToken = QString
                          ( "AND ( %1.id = tags.%2 AND %3.name LIKE '\%%4\%' OR tags.title LIKE '\%%5\%' )" )
                          .arg( m_category1.lower() )
                          .arg( m_category1.lower() )
                          .arg( m_category1.lower() )
                          .arg( m_db->escapeString( m_filter ) )
                          .arg( m_db->escapeString( m_filter ) );
            QString id = QString::number( m_db->getValueID( m_category1.lower(), item->text( 0 ), false ) );

            command = QString
                      ( "SELECT DISTINCT %1.name, '0' FROM tags, %2, %3 WHERE tags.%4=%5.id AND tags.%6=%7 %8 ORDER BY track;" )
                      .arg( m_category2.lower() )
                      .arg( m_category2.lower() )
                      .arg( m_category1.lower() )
                      .arg( m_category2.lower() )
                      .arg( m_category2.lower() )
                      .arg( m_category1.lower() )
                      .arg( id )
                      .arg( filterToken );
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
            if ( m_category2 != "None" )
                child->setPixmap( 0, pixmap );
            child->setUrl( values[ i + 1 ] );
            child->setExpandable( m_category2 != "None" );
    //         kdDebug() << "url: " << values[ i + 1 ] << endl;
        }
    }
    else {
        QString id = QString::number( m_db->getValueID( m_category1.lower(), item->parent()->text( 0 ), false ) );
        QString id_sub = QString::number( m_db->getValueID( m_category2.lower(), item->text( 0 ), false ) );

        QString command = QString
                          ( "SELECT tags.title, tags.url FROM tags, %1 WHERE tags.%2 = %3 AND tags.%4 = %5 AND tags.%6 = %7.id %8 ORDER BY track;" )
                          .arg( m_category1.lower() )
                          .arg( m_category1.lower() )
                          .arg( id )
                          .arg( m_category2.lower() )
                          .arg( id_sub )
                          .arg( m_category1.lower() )
                          .arg( m_category1.lower() )
                          .arg( filterToken );

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


QPixmap
CollectionView::iconForCat( const QString& cat ) const
{
    QString icon;
    if ( cat == "Album"  ) icon = "cdrom_unmount";
    if ( cat == "Artist" ) icon = "personal";
    if ( cat == "Genre"  ) icon = "kfm";
    if ( cat == "Year"   ) icon = "history";

    KIconLoader iconLoader;
    return iconLoader.loadIcon( icon, KIcon::Toolbar, KIcon::SizeSmall );
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
    m_category1 = catForId( id );
    setColumnText( 0, m_category1 );
    m_parent->m_cat1Menu->setItemChecked( idForCat( m_category1 ), true );
    
    enableAllItems( m_parent->m_cat2Menu );
    //prevent choosing the same category in both menus
    m_parent->m_cat2Menu->setItemEnabled( id , false );
    
    renderView();
}


void
CollectionView::cat2Menu( int id )  //SLOT
{
    m_parent->m_cat2Menu->setItemChecked( idForCat( m_category2 ), false ); //uncheck old item
    m_category2 = catForId( id );
    m_parent->m_cat2Menu->setItemChecked( idForCat( m_category2 ), true );
    
    enableAllItems( m_parent->m_cat1Menu );
    //prevent choosing the same category in both menus
    m_parent->m_cat1Menu->setItemEnabled( id , false );

    renderView();
}


void
CollectionView::enableAllItems( KPopupMenu* const menu )
{
    for ( uint index = 0; index < menu->count(); index++ )
        menu->setItemEnabled( menu->idAt( index ), true );
}


QString
CollectionView::catForId( int id ) const
{
    switch ( id ) {
        case CollectionBrowser::IdAlbum:
            return "Album";
        case CollectionBrowser::IdArtist:
            return "Artist";
        case CollectionBrowser::IdGenre:
            return "Genre";
        case CollectionBrowser::IdYear:
            return "Year";
        default:
            break;
    }

    return "None";
}


int
CollectionView::idForCat( const QString& cat ) const
{
    if ( cat == "Album"  ) return CollectionBrowser::IdAlbum;
    if ( cat == "Artist" ) return CollectionBrowser::IdArtist;
    if ( cat == "Genre"  ) return CollectionBrowser::IdGenre;
    if ( cat == "Year"   ) return CollectionBrowser::IdYear;

    //falltrough:
    return CollectionBrowser::IdNone;
}


void
CollectionView::customEvent( QCustomEvent *e )
{
    if ( e->type() == (QEvent::Type) ThreadWeaver::Job::CollectionReader ) {
        kdDebug() << k_funcinfo << endl;

        CollectionReader * c = static_cast<CollectionReader*>( e );
        //CollectionReader provides a list of all subdirs, which we feed into KDirWatch
        if ( m_monitor )
            for ( uint i = 0; i < c->dirList().count(); i++ )
                if ( !m_dirWatch->contains( c->dirList()[i] ) ) {
                    m_dirWatch->addDir( c->dirList()[i], true );
                        //kdDebug() << "Adding to dirWatch: " << c->dirList()[i] << endl;
                }

        // we need to reconnect to the db after every scan, since sqlite is not able to keep
        // the tables synced for multiple threads.
        delete m_db;
        delete m_insertdb;
        m_db = new CollectionDB( m_databasePath );

        emit tagsReady();
    }
}


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
                filterToken = QString
                      ( "AND ( %1.name LIKE '\%%2\%' OR tags.title LIKE '\%%3\%' )" )
                      .arg( m_category1.lower() )
                      .arg( m_db->escapeString( m_filter ) )
                      .arg( m_db->escapeString( m_filter ) );


            //query database for all tracks in our sub-category
            QString id = QString::number( m_db->getValueID( m_category1.lower(), item->text( 0 ), false ) );
            QString command = QString
                              ( "SELECT DISTINCT tags.url FROM tags, %1 WHERE tags.%2=%3 %4" )
                              .arg( m_category1.lower() )
                              .arg( m_category1.lower() )
                              .arg( id )
                              .arg( filterToken );

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
    if ( m_category2 == "None" ) {
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
                    filterToken = QString
                                  ( "AND ( %1.id = tags.%2 AND %3.name LIKE '\%%4\%' OR tags.title LIKE '\%%5\%' )" )
                                  .arg( m_category1.lower() )
                                  .arg( m_category1.lower() )
                                  .arg( m_category1.lower() )
                                  .arg( m_db->escapeString( m_filter ) )
                                  .arg( m_db->escapeString( m_filter ) );
                    QString id = QString::number( m_db->getValueID( m_category1.lower(), item->text( 0 ), false ) );
                    QString id_sub = QString::number( m_db->getValueID( m_category2.lower(), child->text( 0 ), false ) );

                    QString command = QString
                              ( "SELECT DISTINCT tags.url FROM tags, %1 WHERE tags.%2=%3 AND tags.%4=%5 %6 ORDER BY track;" )
                              .arg( m_category1.lower() )
                              .arg( m_category2.lower() )
                              .arg( id_sub )
                              .arg( m_category1.lower() )
                              .arg( id )
                              .arg( filterToken );

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


void
CollectionView::rmbPressed( QListViewItem* item, const QPoint& point, int ) //SLOT
{
    if ( item ) {
        KPopupMenu menu( this );
        
        menu.insertItem( i18n( "Make Playlist" ), this, SLOT( makePlaylist() ) );
        
        if ( ( item->depth() && m_category2 == "None" ) || item->depth() == 2 )
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
CollectionView::showTrackInfo() //slot
{
    Item* item = static_cast<Item*>( currentItem() );
    if ( !item ) return;

    if ( m_category2 == "None" || item->depth() == 2 ) {
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
        str += body.arg( i18n( "Year" ),   values[3] );
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


int
CollectionView::Item::compare( QListViewItem* item, int col, bool ascending ) const
{
    //We overload compare() just to prevent the listView from sorting our items.

    if ( item->depth() == 1 && ( (CollectionView*) listView() )->m_category2 == "None" )
        return 0;
    if ( item->depth() == 2 )
        return 0;

    return KListViewItem::compare( item, col, ascending );
}


#include "collectionbrowser.moc"


