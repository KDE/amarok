//
// AmarokSystray
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#include "amaroksystray.h"
#include "playerapp.h"

#include <kaction.h>
#include <kapplication.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>
#include <ksystemtray.h>

#include <kurldrag.h>


AmarokSystray::AmarokSystray( QWidget *playerWidget, KActionCollection *ac ) : KSystemTray( playerWidget )
{
    setPixmap( KSystemTray::loadIcon("amarok") ); // @since 3.2

    // Usability note:
    // Popping up menu item has some implications..
    //  1. you most probably would want to do track-related operations from
    //     popup menu
    //  2. you probably don't want to hit "quit" by accident.
    //  3. you may have your menu popping up from bottom, top or side of
    //     the screen - so the relative placement of items may differ.

    //<mxcl> despite the usability concerns, we have to be consistent with the KDE style guide
    //       hence quit is now placed at the bottom
    //<berkus> fuck you, i'm forking

    //<mxcl> Ok here is my reasoning (again):
    // 1. true
    // 2. I can't believe it is possible to hit quit by accident unless you need to replace your mouse
    // 3. exactly why we should stick to the KDE guidelines unless you want to implement something
    //    that changes the menu order depending on systray position

    //FIXME it's high time these were KActions
    contextMenu()->insertItem( QIconSet( locate( "data", "amarok/images/b_prev.png" ) ),
                               i18n( "[&Z] Prev" ), kapp, SLOT( slotPrev() ) );
    contextMenu()->insertItem( QIconSet( locate( "data", "amarok/images/b_play.png" ) ),
                               i18n( "[&X] Play" ), kapp, SLOT( slotPlay() ) );
    contextMenu()->insertItem( QIconSet( locate( "data", "amarok/images/b_pause.png" ) ),
                               i18n( "[&C] Pause" ), kapp, SLOT( slotPause() ) );
    contextMenu()->insertItem( QIconSet( locate( "data", "amarok/images/b_stop.png" ) ),
                               i18n( "[&V] Stop" ), kapp, SLOT( slotStop() ) );
    contextMenu()->insertItem( QIconSet( locate( "data", "amarok/images/b_next.png" ) ),
                               i18n( "[&B] Next" ), kapp, SLOT( slotNext() ) );

    contextMenu()->insertSeparator();

    ac->action( "options_configure" )->plug( contextMenu() );

    setAcceptDrops( true );

    actionCollection()->action( "file_quit" )->disconnect();
    connect( actionCollection()->action( "file_quit" ), SIGNAL( activated() ), kapp, SLOT( quit() ) );
}


void AmarokSystray::wheelEvent( QWheelEvent *e )
{
    //NOTE for some reason ignore() doesn't pass the event to parent unless
    //the parent isVisible() and the active window!

    //send the event to the parent PlayerWidget, it'll handle it with much wisdom
    QApplication::sendEvent( parentWidget(), e );
}



void AmarokSystray::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == MidButton )
    {
        if( static_cast<PlayerApp *>(kapp)->isPlaying() )
            static_cast<PlayerApp *>(kapp)->slotPause();
        else
            static_cast<PlayerApp *>(kapp)->slotPlay();
    }
    else
        KSystemTray::mousePressEvent( e );
}


void AmarokSystray::dragEnterEvent( QDragEnterEvent *e )
{
   e->accept( KURLDrag::canDecode(e) );
}

void AmarokSystray::dropEvent( QDropEvent *e )
{
   KURL::List list;
   if( KURLDrag::decode(e, list) )
      pApp->insertMedia(list);
}


#include "amaroksystray.moc"
