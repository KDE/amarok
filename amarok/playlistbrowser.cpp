// (c) Max Howell 2004
// See COPYING file for licensing information

#include "playlistbrowser.h"

#include <kiconloader.h> //smallIcon
#include <kurldrag.h>    //dragObject()
#include <qevent.h>      //customEvent()


PlaylistBrowser::PlaylistBrowser( const char *name )
   : KIconView( 0, name )
{
    setResizeMode( QIconView::Adjust );
    setSelectionMode( QIconView::Extended );
}

PlaylistBrowser::~PlaylistBrowser()
{}

PlaylistBrowser::Item::Item( QIconView *parent, const KURL &u )
   : KIconViewItem( parent )
   , m_url( u )
{
    QString name = u.fileName();
    setText( name.mid( 0, name.findRev( '.' ) ) ); //FIXME improve
    setDragEnabled( true );
    setPixmap( DesktopIcon( "midi" ) );
}

void
PlaylistBrowser::newPlaylist( const KURL::List &urls )
{
    KURL url = urls.first();
    new PlaylistBrowser::Item( this, url );
}


QDragObject*
PlaylistBrowser::dragObject()
{
    return new KURLDrag( currentItem()->url(), this );
}

void
PlaylistBrowser::customEvent( QEvent *e )
{}

#include "playlistbrowser.moc"
