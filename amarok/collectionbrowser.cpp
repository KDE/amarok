// (c) Mark Kretschmann 2004
// See COPYING file for licensing information


#include "collectionbrowser.h"

#include <kdebug.h>
#include <kurldrag.h>    //dragObject()


CollectionBrowser::CollectionBrowser( const char* name )
   : KIconView( 0, name )
{
    setSelectionMode( QIconView::Extended );
    setItemsMovable( false );
    
    for ( int i = 0; i < 100; i++ ) {
        QIconViewItem* item =  new QIconViewItem( this );
        item->setText( "Album" );
    }
}


CollectionBrowser::~CollectionBrowser()
{}


// QDragObject*
// CollectionBrowser::dragObject()
// {
//     return new KURLDrag( currentItem()->url(), this );
// }


#include "collectionbrowser.moc"

