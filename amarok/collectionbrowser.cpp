// (c) Mark Kretschmann 2004
// See COPYING file for licensing information


#include <config.h>
#ifdef HAVE_SQLITE

#include "collectionbrowser.h"
#include "metabundle.h"
#include "threadweaver.h"

#include <sqlite.h>
#include <vector>

#include <kdebug.h>
#include <kurldrag.h>    //dragObject()


//////////////////////////////////////////////////////////////////////////////////////////
// public
//////////////////////////////////////////////////////////////////////////////////////////

CollectionBrowser::CollectionBrowser( const char* name )
   : KIconView( 0, name )
   , m_weaver( new ThreadWeaver( this ) )
{
    kdDebug() << k_funcinfo << endl;
    
    setSelectionMode( QIconView::Extended );
    setItemsMovable( false );
//     setGridX( 140 );

    KURL url;
    url.setPath( "/home/mark/mp3/mosaic_days.mp3" );
    m_dirs << url;    
                
    m_weaver->append( new CollectionReader( this, url ) );
}


CollectionBrowser::~CollectionBrowser()
{
    kdDebug() << k_funcinfo << endl;
}


// QDragObject*
// CollectionBrowser::dragObject()
// {
//     return new KURLDrag( currentItem()->url(), this );
// }


//////////////////////////////////////////////////////////////////////////////////////////
// private
//////////////////////////////////////////////////////////////////////////////////////////

void CollectionBrowser::customEvent( QCustomEvent *e )
{
    kdDebug() << k_funcinfo << endl;
    
    if ( e->type() == ThreadWeaver::Job::CollectionReader ) {
        CollectionReader* c = static_cast<CollectionReader*>( e );
       
        kdDebug() << "********************************\n";
        kdDebug() << "CollectionEvent arrived.\n";
        kdDebug() << "********************************\n";
        
        if ( c->bundle() ) 
            kdDebug() << "Artist: " << c->bundle()->artist() << endl;
    }
}


#include "collectionbrowser.moc"

#endif /* HAVE_SQLITE */


