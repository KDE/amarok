// (c) Mark Kretschmann 2004
// See COPYING file for licensing information


#include <config.h>
#ifdef HAVE_SQLITE

#include "collectionbrowser.h"

#include <sqlite.h>
#include <vector>

#include <kdebug.h>
#include <kurldrag.h>    //dragObject()



CollectionBrowser::CollectionBrowser( const char* name )
   : KIconView( 0, name )
{
    setSelectionMode( QIconView::Extended );
    setItemsMovable( false );
//     setGridX( 140 );

    KURL url;
    url.setPath( "/home/mark/mp3" );
    
    m_dirs << url;    
}


CollectionBrowser::~CollectionBrowser()
{
}


// QDragObject*
// CollectionBrowser::dragObject()
// {
//     return new KURLDrag( currentItem()->url(), this );
// }


#include "collectionbrowser.moc"

#endif /* HAVE_SQLITE */


