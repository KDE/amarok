// (c) Mark Kretschmann 2004
// See COPYING file for licensing information


#include <config.h>
#ifdef HAVE_SQLITE

#include "collectionbrowser.h"
#include "metabundle.h"
#include "threadweaver.h"

#include <sqlite.h>
#include <vector>

#include <qptrlist.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kglobal.h>
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

    m_db = sqlite_open( "collection.db", 0, 0 );
    
    if ( !m_db )
        kdWarning() << k_funcinfo << "Could not open SQLite database\n";
    
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
            delete bundle;
        }
    }
}


// QDragObject*
// CollectionBrowser::dragObject()
// {
//     return new KURLDrag( currentItem()->url(), this );
// }


#include "collectionbrowser.moc"

#endif /* HAVE_SQLITE */


