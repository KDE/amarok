// (c) Mark Kretschmann 2004
// See COPYING file for licensing information

    
#include "config.h"

#ifdef HAVE_SQLITE

#include "collectionbrowser.h"
#include "metabundle.h"
#include "threadweaver.h"

#include <sqlite.h>
#include <vector>

#include <qcstring.h>
#include <qptrlist.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kurldrag.h>    //dragObject()


//////////////////////////////////////////////////////////////////////////////////////////
// public
//////////////////////////////////////////////////////////////////////////////////////////

CollectionBrowser::CollectionBrowser( const char* name )
    : KIconView( 0, name )
    , m_weaver( new ThreadWeaver( this ) )
    , m_dirLister( new KDirLister() )
{
    kdDebug() << k_funcinfo << endl;
        
    setSelectionMode( QIconView::Extended );
    setItemsMovable( false );

    m_db = sqlite_open( ( KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" )
                          + "collection.db" ).latin1(),
                        0, 0 );
    
    if ( !m_db )
        kdWarning() << k_funcinfo << "Could not open SQLite database\n";
    
    QCString command = "create table tags ( title varchar(100), artist varchar(100) );";
    execSql( command, 0 );
                
    KURL url;
    url.setPath( "/home/mark/mp3" );

    readDir( url );                
}


CollectionBrowser::~CollectionBrowser()
{
    kdDebug() << k_funcinfo << endl;
    
    delete m_dirLister;
    sqlite_close( m_db );
}


//////////////////////////////////////////////////////////////////////////////////////////
// private
//////////////////////////////////////////////////////////////////////////////////////////

void CollectionBrowser::readDir( const KURL& url )
{
    m_dirLister->openURL( url );
    
    while ( !m_dirLister->isFinished() )
        kapp->processEvents(); 
       
    m_weaver->append( new CollectionReader( this, m_dirLister->items() ) );
}


void CollectionBrowser::customEvent( QCustomEvent *e )
{
    kdDebug() << k_funcinfo << endl;
    
    if ( e->type() == ThreadWeaver::Job::CollectionReader ) {
        CollectionReader* c = static_cast<CollectionReader*>( e );
       
        kdDebug() << "********************************\n";
        kdDebug() << "CollectionEvent arrived.\n";
        kdDebug() << "********************************\n";
        
        kdDebug() << "Number of bundles: " << c->list().count() << endl;

        MetaBundle* bundle;
        
        for ( int i = 0; i < c->list().count(); i++ ) {
            bundle = c->list().at( i );
            kdDebug() << bundle->artist() << endl;
            
            QCString command = "insert into tags( artist ) values ('";
            command += bundle->artist().latin1();
            command +=         "');";
            
            execSql( command, 0 );
            delete bundle;
        }
    }
}


int CollectionBrowser::execSql( const QCString& statement, void* callback )
{
    if ( !m_db ) {
        kdWarning() << k_funcinfo << "SQLite pointer == NULL.\n";
        return SQLITE_ERROR;
    }
    
    char* errorStr;
    int error = sqlite_exec( m_db, statement, ( int (*)(void*,int,char**,char**) ) callback, 0, &errorStr );
    
    if ( error != SQLITE_OK ) {
        kdWarning() << k_funcinfo << "SQLite error while executing statement:\n";
        kdWarning() << errorStr << endl;
    }
    free( errorStr );
    
    return error;
}


// QDragObject*
// CollectionBrowser::dragObject()
// {
//     return new KURLDrag( currentItem()->url(), this );
// }


#include "collectionbrowser.moc"

#endif /* HAVE_SQLITE */


