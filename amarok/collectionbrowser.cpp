// (c) Mark Kretschmann 2004
// See COPYING file for licensing information

    
#include "config.h"

#ifdef HAVE_SQLITE

#include "collectionbrowser.h"
#include "directorylist.h"
#include "metabundle.h"
#include "threadweaver.h"

#include <sqlite.h>

#include <qcstring.h>
#include <qdragobject.h>
#include <qptrlist.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>
#include <kurldrag.h>    //dragObject()


CollectionBrowser::CollectionBrowser( const char* name )
    : QVBox( 0, name )
{
    QHBox* hbox = new QHBox( this );
    
    m_actionsMenu = new KPopupMenu( hbox );
    m_catMenu = new KPopupMenu( hbox );
    
    KMenuBar* menu = new KMenuBar( this );
    menu->insertItem( "Actions",    m_actionsMenu );
    menu->insertItem( "Categories", m_catMenu );
    
    CollectionView* view = new CollectionView( this );

    m_actionsMenu->insertItem( "Setup Folders", view, SLOT( setupDirs() ) );
    m_actionsMenu->insertItem( "Scan Now",      view, SLOT( scan() ) );
    
    m_catMenu    ->insertItem( "Album",         view, SLOT( actionsMenu( int ) ), 0, IdAlbum  );
    m_catMenu    ->insertItem( "Artist",        view, SLOT( actionsMenu( int ) ), 0, IdArtist );
    m_catMenu    ->insertItem( "Genre",         view, SLOT( actionsMenu( int ) ), 0, IdGenre  );
    m_catMenu    ->insertItem( "Year",          view, SLOT( actionsMenu( int ) ), 0, IdYear   );
}  


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionView
//////////////////////////////////////////////////////////////////////////////////////////

CollectionView::CollectionView( CollectionBrowser* parent )
    : KListView( parent )
    , m_parent( parent )
    , m_weaver( new ThreadWeaver( this ) )
    , m_dirLister( new KDirLister() )
{
    kdDebug() << k_funcinfo << endl;
 
    setSelectionMode( QListView::Extended );
    setItemsMovable( false );
    setRootIsDecorated( true );
    setShowSortIndicator( true );
    setFullWidth( true );
    
    QCString path = ( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" )
                      + "collection.db" ).latin1(); 
    
    m_db = sqlite_open( path, 0, 0 );
    
    if ( !m_db )
        kdWarning() << k_funcinfo << "Could not open SQLite database\n";
    
    QCString command = "create table tags ( url varchar(100),"
                                           "album varchar(100),"
                                           "artist varchar(100),"
                                           "genre varchar(100),"
                                           "title varchar(100),"
                                           "year varchar(4) );";
    
    execSql( command, 0, 0 );

    KConfig* config = KGlobal::config();
    config->setGroup( "Collection Browser" );
    
    m_dirs = config->readListEntry( "Folders" );
    m_category = config->readEntry( "Category", "Album" );
    addColumn( m_category );
    
    connect( this, SIGNAL( tagsReady() ),
             this,   SLOT( renderView() ) );
    connect( this, SIGNAL( expanded( QListViewItem* ) ),
             this,   SLOT( slotExpanded( QListViewItem* ) ) );
    connect( this, SIGNAL( collapsed( QListViewItem* ) ),
             this,   SLOT( slotCollapsed( QListViewItem* ) ) );
             
    renderView();
}


CollectionView::~CollectionView()
{
    kdDebug() << k_funcinfo << endl;
    
    KConfig* config = KGlobal::config();
    config->setGroup( "Collection Browser" );
    config->writeEntry( "Folders", m_dirs );
    config->writeEntry( "Category", m_category );
    
    delete m_dirLister;
    sqlite_close( m_db );
}


//////////////////////////////////////////////////////////////////////////////////////////
// private
//////////////////////////////////////////////////////////////////////////////////////////

void
CollectionView::setupDirs() //SLOT
{
    DirectoryList list( m_dirs, false );
    DirectoryList::Result result = list.exec();

    m_dirs = result.dirs;
}


void 
CollectionView::scan() //SLOT
{
    //remove all records
    QCString command = "delete from tags;";
    execSql( command, 0, 0 );
    
    for ( uint i = 0; i < m_dirs.count(); i ++ ) {
        KURL url;   
        url.setPath( m_dirs[i] );
        readDir( url );                
    }
}


void 
CollectionView::renderView() //SLOT
{
    kdDebug() << k_funcinfo << endl;
    
    clear();

    //query database for all records with the specified category
    QCString command = "select distinct ";
    command += m_category.lower().latin1();
    command += " from tags;";
    
    QStringList values;
    QStringList names;
    
    execSql( command, &values, &names );
    
    for ( uint i = 0; i < values.count(); i++ ) {
        if ( values[i].isEmpty() ) continue;
        
        KListViewItem* item = new KListViewItem( this );    
        item->setExpandable( true );
        item->setDragEnabled( false );
        item->setDropEnabled( false );
        item->setText( 0, values[i] );
    }    
        
    kdDebug() << values << endl;
}


void 
CollectionView::slotExpanded( QListViewItem* item ) //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !item ) return;
    
    //query database for all tracks in our sub-category
    QCString command = "select title, url from tags where ";
    command += m_category.lower().latin1();
    command += " = '";
    command += item->text( 0 ).latin1();
    command += "';";
    
    QStringList values;
    QStringList names;
    
    execSql( command, &values, &names );
    
    for ( uint i = 0; i < values.count(); i += 2 ) {
        if ( values[i].isEmpty() ) continue;
        
        Item* child = new Item( item );    
        child->setDragEnabled( true );
        child->setDropEnabled( false );
        child->setText( 0, values[i+0] );
        kdDebug() << "url: " << values[i+1] << endl;
        child->setUrl( values[i+1] );
    }    
        
    kdDebug() << values << endl;
}


void 
CollectionView::slotCollapsed( QListViewItem* item ) //SLOT
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
CollectionView::actionsMenu( int id ) //SLOT
{
    switch( id ) {
    
    case CollectionBrowser::IdAlbum:
        m_category = "Album";
        break;
    case CollectionBrowser::IdArtist:
        m_category = "Artist";
        break;
    case CollectionBrowser::IdGenre:
        m_category = "Genre";
        break;
    case CollectionBrowser::IdYear:
        m_category = "Year";
        break;
    default:
        return;
    }
    
    setColumnText( 0, m_category );
    renderView();
}


void 
CollectionView::readDir( const KURL& url )
{
    m_dirLister->openURL( url );
    
    while ( !m_dirLister->isFinished() )
        kapp->processEvents(); 
       
    m_weaver->append( new CollectionReader( this, m_dirLister->items() ) );
}


void 
CollectionView::customEvent( QCustomEvent *e )
{
    kdDebug() << k_funcinfo << endl;
    
    if ( e->type() == ThreadWeaver::Job::CollectionReader ) {
        CollectionReader* c = static_cast<CollectionReader*>( e );
       
        kdDebug() << "********************************\n";
        kdDebug() << "CollectionEvent arrived.\n";
        kdDebug() << "********************************\n";
        
        kdDebug() << "Number of bundles: " << c->list().count() << endl;

        MetaBundle* bundle;
        
        for ( uint i = 0; i < c->list().count(); i++ ) {
            bundle = c->list().at( i );
//             kdDebug() << bundle->artist() << endl;
            
            QCString command = "insert into tags( url, album, artist, genre, title, year ) values ('";
            command += bundle->url().path().latin1();
            command +=         "','";
            command += bundle->album().latin1();
            command +=         "','";
            command += bundle->artist().latin1();
            command +=         "','";
            command += bundle->genre().latin1();
            command +=         "','";
            command += bundle->title().latin1();
            command +=         "','";
            command += bundle->year().latin1();
            command +=         "');";
            
            execSql( command, 0, 0 );
            delete bundle;
        }
    }
    
    emit tagsReady();
}


void 
CollectionView::dumpDb()
{
    kdDebug() << k_funcinfo << endl;
    
    QCString command = "select artist from tags;";
   
    QStringList values;
    QStringList names;
    
    execSql( command, &values, &names );
    kdDebug() << values << endl;
}


bool 
CollectionView::execSql( const QCString& statement,
                                     QStringList* const values,
                                     QStringList* const names )
{
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
    error = sqlite_compile( m_db, statement, &tail, &vm, &errorStr );
        
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
    while( true ) {
        error = sqlite_step( vm, &number, &value, &colName );
        
        if ( error == SQLITE_DONE || error == SQLITE_ERROR )
            break; 
        //iterate over columns
        for ( int i = 0; values && names && i < number; i++ ) {
            *values << value  [i];
            *names  << colName[i];
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
CollectionView::startDrag()
{
    if ( currentItem() ) {
        KURLDrag* d = new KURLDrag( currentItem()->url(), this, "DragObject" );
            d->dragCopy();
    }
        
}


#include "collectionbrowser.moc"

#endif /* HAVE_SQLITE */


