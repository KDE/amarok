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
#include <qptrlist.h>
#include <qpushbutton.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kurldrag.h>    //dragObject()


CollectionBrowser::CollectionBrowser( const char* name )
    : QVBox( 0, name )
{
    QHBox* hbox = new QHBox( this );
    QPushButton* button  = new QPushButton( i18n( "Folders" ), hbox );
    QPushButton* button1 = new QPushButton( i18n( "Scan" ), hbox );
    
    m_comboBox = new KComboBox( this );    
    m_comboBox->insertItem( "Album" );
    m_comboBox->insertItem( "Artist" );
    m_comboBox->insertItem( "Genre" );
    m_comboBox->insertItem( "Year" );
    
    CollectionView* view = new CollectionView( this );

    connect( button,     SIGNAL( clicked() ),
             view,         SLOT( setupDirs() ) );
    connect( button1,    SIGNAL( clicked() ),
             view,         SLOT( scan() ) );
    connect( m_comboBox, SIGNAL( activated( int ) ),
             view,         SLOT( renderView() ) );
}  


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS CollectionView
//////////////////////////////////////////////////////////////////////////////////////////

CollectionView::CollectionView( CollectionBrowser* parent )
    : KIconView( parent )
    , m_parent( parent )
    , m_weaver( new ThreadWeaver( this ) )
    , m_dirLister( new KDirLister() )
{
    kdDebug() << k_funcinfo << endl;
 
    setGridX( 150 );
    setGridY(  35 );
               
    setSelectionMode( QIconView::Extended );
    setItemsMovable( false );

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
    QString cat = config->readEntry( "Category" );
    
    for ( int i = 0; i < m_parent->m_comboBox->count(); i++ )
        if ( m_parent->m_comboBox->text( i ) == cat )
            m_parent->m_comboBox->setCurrentItem( i );
    
    connect( this, SIGNAL( tagsReady() ),
             this,   SLOT( renderView() ) );

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
    m_category = m_parent->m_comboBox->currentText();
    QCString command = "select ";
    command += m_category.lower().latin1();
    command += " from tags;";
    
    QStringList values;
    QStringList names;
    
    execSql( command, &values, &names );
    
    for ( uint i = 0; i < values.count(); i++ ) {
        KIconViewItem* item = new KIconViewItem( this );    
        item->setText( values[i] );
    }    
        
    kdDebug() << values << endl;
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
    
    int n;
    const char** value;
    const char** colName;
    //execute virtual machine by iterating over rows
        while( true ) {
        error = sqlite_step( vm, &n, &value, &colName );
        
        if ( error == SQLITE_DONE || error == SQLITE_ERROR )
            break; 
        if ( values && value )
            *values << *value;
        if ( names && colName )
            *names << *colName;
    }
    //deallocate vm ressources
    sqlite_finalize( vm, 0 );
    
    if ( error != SQLITE_DONE ) {
        kdWarning() << k_funcinfo << "sqlite_step error.\n";
        return "error";
    }
        
    return true;
}


// QDragObject*
// CollectionBrowser::dragObject()
// {
//     return new KURLDrag( currentItem()->url(), this );
// }


#include "collectionbrowser.moc"

#endif /* HAVE_SQLITE */


