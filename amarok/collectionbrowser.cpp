// (c) Mark Kretschmann 2004
// See COPYING file for licensing information


#include "collectionbrowser.h"

#include <qsqldatabase.h>

#include <kdebug.h>
#include <kurldrag.h>    //dragObject()



CollectionBrowser::CollectionBrowser( const char* name )
   : KIconView( 0, name )
{
    setSelectionMode( QIconView::Extended );
    setItemsMovable( false );
//     setGridX( 140 );
    
    m_pDb = QSqlDatabase::addDatabase( "QSQLITE" );
    m_pDb->setDatabaseName( "collection.db" );
    m_pDb->setUserName    ( "mark" );
    m_pDb->setPassword    ( "test" );
    m_pDb->setHostName    ( "localhost" );

    if ( !m_pDb->open() )
        kdWarning() << k_funcinfo << "Could not open collection database.\n";
        
    for ( int i = 0; i < 100; i++ ) {
        QIconViewItem* item =  new QIconViewItem( this );
        item->setText( "Album" );
    }
}


CollectionBrowser::~CollectionBrowser()
{
    m_pDb->close();
}


// QDragObject*
// CollectionBrowser::dragObject()
// {
//     return new KURLDrag( currentItem()->url(), this );
// }


#include "collectionbrowser.moc"

