// (c) Mark Kretschmann 2004
// See COPYING file for licensing information


#include "config.h"

#ifdef HAVE_SQLITE

#include "collectionbrowser.h"
#include "directorylist.h"
#include "metabundle.h"
#include "statusbar.h"
#include "threadweaver.h"

#include <sqlite.h>
#include <unistd.h>         //CollectionView ctor

#include <qcstring.h>
#include <qdragobject.h>
#include <qptrlist.h>

#include <kapplication.h>
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
    QHBox * hbox = new QHBox( this );

    m_actionsMenu = new KPopupMenu( hbox );
    m_cat1Menu = new KPopupMenu( hbox );
    m_cat2Menu = new KPopupMenu( hbox );

    KMenuBar* menu = new KMenuBar( this );
    menu->insertItem( i18n( "Actions" ), m_actionsMenu );
    menu->insertItem( i18n( "Primary" ), m_cat1Menu );
    menu->insertItem( i18n( "Secondary" ), m_cat2Menu );
    
    CollectionView* view = new CollectionView( this );
    
    m_actionsMenu->insertItem( i18n( "Configure Folders" ), view, SLOT( setupDirs() ) );
    m_actionsMenu->insertItem( i18n( "Start Scan" ), view, SLOT( scan() ) );
    
    m_cat1Menu ->insertItem( "Album", view, SLOT( cat1Menu( int ) ), 0, IdAlbum );
    m_cat1Menu ->insertItem( "Artist", view, SLOT( cat1Menu( int ) ), 0, IdArtist );
    m_cat1Menu ->insertItem( "Genre", view, SLOT( cat1Menu( int ) ), 0, IdGenre );
    m_cat1Menu ->insertItem( "Year", view, SLOT( cat1Menu( int ) ), 0, IdYear );

    m_cat2Menu ->insertItem( "None", view, SLOT( cat2Menu( int ) ), 0, IdNone );
    m_cat2Menu ->insertSeparator();
    m_cat2Menu ->insertItem( "Album", view, SLOT( cat2Menu( int ) ), 0, IdAlbum );
    m_cat2Menu ->insertItem( "Artist", view, SLOT( cat2Menu( int ) ), 0, IdArtist );
    m_cat2Menu ->insertItem( "Genre", view, SLOT( cat2Menu( int ) ), 0, IdGenre );
    m_cat2Menu ->insertItem( "Year", view, SLOT( cat2Menu( int ) ), 0, IdYear );

    m_cat1Menu->setItemChecked( view->idForCat( view->m_category1 ), true );
    m_cat2Menu->setItemChecked( view->idForCat( view->m_category2 ), true );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionView
//////////////////////////////////////////////////////////////////////////////////////////

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
        m_monitor = config->readBoolEntry( "Monitor Changes", true );
    //</read config>
    
    //<open database>
        QCString path = ( KGlobal::dirs() ->saveLocation( "data", kapp->instanceName() + "/" )
                        + "collection.db" ).local8Bit();
        //remove database file if version is incompatible
        if ( config->readNumEntry( "Database Version", 0 ) != DATABASE_VERSION )
            ::unlink( path );
                            
        m_db = sqlite_open( path, 0, 0 );
    
        if ( !m_db )
            kdWarning() << k_funcinfo << "Could not open SQLite database\n";
        //optimization for speeding up SQLite
        execSql( "PRAGMA default_synchronous = OFF;" );        
        execSql( "PRAGMA default_cache_size = 4000;" );        
            
        QCString command = "CREATE TABLE tags ("
                        "url varchar(100),"
                        "dir varchar(100),"
                        "album varchar(100),"
                        "artist varchar(100),"
                        "genre varchar(100),"
                        "title varchar(100),"
                        "year varchar(4) );";
    
        execSql( command );
    //</open database>
    
    connect( this,       SIGNAL( tagsReady() ),
             this,         SLOT( renderView() ) );
    connect( this,       SIGNAL( expanded( QListViewItem* ) ),
             this,         SLOT( slotExpand( QListViewItem* ) ) );
    connect( this,       SIGNAL( collapsed( QListViewItem* ) ),
             this,         SLOT( slotCollapse( QListViewItem* ) ) );
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

    sqlite_close( m_db );
}


//////////////////////////////////////////////////////////////////////////////////////////
// private
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::setupDirs()  //SLOT
{
    DirectoryList list( m_dirs, m_recursively, m_monitor );
    DirectoryList::Result result = list.exec();

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
    //remove all records
    QCString command = "DELETE FROM tags;";
    execSql( command );

    m_weaver->append( new CollectionReader( this, amaroK::StatusBar::self(), m_dirs, m_recursively ) );

    if ( m_monitor )
        m_dirWatch->startScan();
}


void
CollectionView::dirDirty( const QString& path )
{
    kdDebug() << k_funcinfo << "Dirty: " << path << endl;

    //remove old records with the same dir as our dirty dir, to prevent dupes
    QString command = "DELETE FROM tags WHERE dir = '";
    command += path;
    command += "';";
    execSql( command );
        
    m_weaver->append( new CollectionReader( this, amaroK::StatusBar::self(), path, false ) );
}


void
CollectionView::renderView()  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    clear();

    //query database for all records with the specified category
    QString command = "SELECT DISTINCT ";
    command += m_category1.lower();
    command += " FROM tags;";
    QStringList values;
    QStringList names;
    execSql( command, &values, &names );
    
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
    if  ( item->depth() == 0 ) {    
        //Filter for category 1:
        QString cat = ( m_category2 == "None" ) ? "title,url" : m_category2.lower()
                                                                           .append( "," )
                                                                           .append( m_category2.lower() );
        QString command = "SELECT DISTINCT ";
        command += cat;
        command += " FROM tags WHERE ";
        command += m_category1.lower();
        command += " = '";
        command += item->text( 0 );
        command += "';";
        QStringList values;
        QStringList names;
        execSql( command, &values, &names );
    
        QPixmap pixmap = iconForCat( m_category2 );
        
        for ( uint i = 0; i < values.count(); i += 2 ) {
            if ( values[i].isEmpty() ) continue;
    
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
        //Filter for category 2:
        QString command = "SELECT title, url FROM tags WHERE ";
        command += m_category1.lower();
        command += " = '";
        command += item->parent()->text( 0 );
        command += "' AND ";
        command += m_category2.lower();
        command += " = '";
        command += item->text( 0 );
        command += "';";
        QStringList values;
        QStringList names;
        execSql( command, &values, &names );
    
        for ( uint i = 0; i < values.count(); i += 2 ) {
            if ( values[i].isEmpty() ) continue;
    
            Item* child = new Item( item );
            child->setDragEnabled( true );
            child->setDropEnabled( false );
            child->setText( 0, values[ i + 0 ] );
            child->setUrl( values[ i + 1 ] );
        }
    }
}


QPixmap
CollectionView::iconForCat( const QString& cat )
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
    
    renderView();
}


void
CollectionView::cat2Menu( int id )  //SLOT
{
    m_parent->m_cat2Menu->setItemChecked( idForCat( m_category2 ), false ); //uncheck old item
    m_category2 = catForId( id );
    m_parent->m_cat2Menu->setItemChecked( idForCat( m_category2 ), true );
    
    renderView();
}


QString
CollectionView::catForId( int id )
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
CollectionView::idForCat( const QString& cat )
{
    if ( cat == "Album"  ) return CollectionBrowser::IdAlbum;
    if ( cat == "Artist" ) return CollectionBrowser::IdArtist;
    if ( cat == "Genre"  ) return CollectionBrowser::IdGenre;
    if ( cat == "Year"   ) return CollectionBrowser::IdYear;

    //falltrough:
    return CollectionBrowser::IdNone;
}


void
CollectionView::customEvent( QCustomEvent *e ) {
    kdDebug() << k_funcinfo << endl;

    if ( e->type() == (QEvent::Type) ThreadWeaver::Job::CollectionReader ) {
        CollectionReader * c = static_cast<CollectionReader*>( e );

        kdDebug() << "********************************\n";
        kdDebug() << "CollectionEvent arrived.\n";
        kdDebug() << "********************************\n";

        //CollectionReader provides a list of all subdirs, which we feed into KDirWatch 
        if ( m_monitor )
            for ( uint i = 0; i < c->dirList().count(); i++ )
                if ( !m_dirWatch->contains( c->dirList()[i] ) ) {
                    m_dirWatch->addDir( c->dirList()[i], true );
//                     kdDebug() << "Adding to dirWatch: " << c->dirList()[i] << endl;
                }
                    
        MetaBundle* bundle;
        QString tag;
        kdDebug() << "Number of records to store in db: " << c->bundleList().count() << endl;
        execSql( "BEGIN TRANSACTION;" );
        
        for ( uint i = 0; i < c->bundleList().count(); i++ ) {
            bundle = c->bundleList().at( i );
            
            QString command = "INSERT INTO tags "
                              "( url, dir,  album, artist, genre, title, year ) "
                              "VALUES('";
                                                       
            tag = bundle->url().path();
            tag.remove( "'" );
            command += tag;
            command += "','";
            
            tag = bundle->url().directory();
            tag.remove( "'" );
            command += tag;
            command += "','";
            
            tag = bundle->album();
            tag.remove( "'" );
            command += tag;
            command += "','";
            
            tag = bundle->artist();
            tag.remove( "'" );
            command += tag;
            command += "','";
            
            tag = bundle->genre();
            tag.remove( "'" );
            command += tag;
            command += "','";
            
            tag = bundle->title();
            tag.remove( "'" );
            command += tag;
            command += "','";
            
            tag = bundle->year();
            tag.remove( "'" );
            command += tag;
            command += "');";

            execSql( command );
            delete bundle;
            //grant event loop some time for breathing
            if ( !(i % 10) ) kapp->processEvents();
        }
        execSql( "END TRANSACTION;" );
        
        emit tagsReady();
    }
}


bool
CollectionView::execSql( const QString& statement,
                         QStringList* const values,
                         QStringList* const names ) {
    //     kdDebug() << k_funcinfo << endl;

    if ( !m_db ) {
        kdWarning() << k_funcinfo << "SQLite pointer == NULL.\n";
        return false;
    }

    const char* tail;
    sqlite_vm* vm;
    char* errorStr;
    int error;
    //compile SQL program to virtual machine
    error = sqlite_compile( m_db, statement.local8Bit(), &tail, &vm, &errorStr );

    if ( error != SQLITE_OK ) {
        kdWarning() << k_funcinfo << "sqlite_compile error:\n";
        kdWarning() << errorStr << endl;
        sqlite_freemem( errorStr );
        return false;
    }

    int number;
    const char** value;
    const char** colName;
    //execute virtual machine by iterating over rows
    while ( true ) {
        error = sqlite_step( vm, &number, &value, &colName );

        if ( error == SQLITE_DONE || error == SQLITE_ERROR )
            break;
        //iterate over columns
        for ( int i = 0; values && names && i < number; i++ ) {
            *values << value [ i ];
            *names << colName[ i ];
        }
    }
    //deallocate vm ressources
    sqlite_finalize( vm, 0 );

    if ( error != SQLITE_DONE ) {
        kdWarning() << k_funcinfo << "sqlite_step error.\n";
        return "error";
    }

    return true;
}


void
CollectionView::startDrag() {
    //Here we determine the URLs of all selected items. We use two passes, one for the parent items,
    //and another one for the children.
    
    KURL::List list;
    QListViewItem* item;
    
    //first pass: parents
    for ( item = firstChild(); item; item = item->nextSibling() )
        if ( item->isSelected() ) {
            //query database for all tracks in our sub-category
            QString command = "SELECT url FROM tags WHERE ";
            command += m_category1.lower();
            command += " = '";
            command += item->text( 0 );
            command += "';";
            QStringList values;
            QStringList names;
            execSql( command, &values, &names );

            for ( uint i = 0; i < values.count(); i++ )
                list << values[i];
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
                    //query database for all tracks in our sub-category
                    QString command = "SELECT DISTINCT url FROM tags WHERE ";
                    command += m_category1.lower();
                    command += " = '";
                    command += item->text( 0 );
                    command += "' AND ";
                    command += m_category2.lower();
                    command += " = '";
                    command += child->text( 0 );
                    command += "';";
                    QStringList values;
                    QStringList names;
                    execSql( command, &values, &names );
        
                    for ( uint i = 0; i < values.count(); i++ )
                        list << values[i];
                }
    }
                        
    //third pass: category 2    
    for ( item = firstChild(); item; item = item->nextSibling() )
        for ( QListViewItem* child = item->firstChild(); child; child = child->nextSibling() )
            for ( QListViewItem* grandChild = child->firstChild(); grandChild; grandChild = grandChild->nextSibling() )
                if ( grandChild->isSelected() )
                    list << static_cast<Item*>( grandChild ) ->url();
    
    KURLDrag* d = new KURLDrag( list, this );
    d->dragCopy();
}


#include "collectionbrowser.moc"

#endif /* HAVE_SQLITE */


