//
// AmarokSystray
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#include "amaroksystray.h"
#include "playerapp.h"
#include "playerwidget.h"

#include <kactioncollection.h>
#include <kapplication.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>
#include <ksystemtray.h>


AmarokSystray::AmarokSystray( PlayerWidget *playerWidget, KActionCollection *ac ) : KSystemTray( playerWidget )
{
    //    setPixmap( KSystemTray::loadIcon("amarok") ); // @since 3.2
    setPixmap( kapp->miniIcon() ); // 3.1 compatibility for 0.7

    // <berkus> Since it doesn't come to you well, i'll explain it here:
    // We put playlist actions last because: 1) you don't want to accidentally
    // switch amaroK off by pushing rmb on tray icon and then suddenly lmb on the
    // bottom item. 2) if you do like in case 1) the most frequent operation is to
    // change to next track, so it must be at bottom. [usability]

    //<mxcl> usability is much more than just making it so you don't have to move the mouse
    //far, in fact that's a tiny segment of the overly broad term, "usability" and is called ergonomics.
    //Another element of usability, and far more important here than ergonomics, is consistency.
    //Every KDE app has the quit button at the bottom. We should do the same.
    //Also having the actions at the bottom for the reasons described only works when the panel is
    //at the bottom of the screen. This can not be guarenteed.
    //Finally the reasons berkus gave are less relevant now since all the actions can be controlled by
    //various mouse actions over the tray icon.

    contextMenu()->clear();
    contextMenu()->insertTitle( kapp->miniIcon(), kapp->caption() );

    ac->action( "options_configure" )->plug( contextMenu() );
    contextMenu()->insertItem( i18n( "&Help" ), (QPopupMenu *)playerWidget->helpMenu() );
    ac->action( "file_quit" )->plug( contextMenu() );

    contextMenu()->insertSeparator();

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

    // don't love them just yet
    setAcceptDrops( false );
}


void AmarokSystray::wheelEvent( QWheelEvent *e )
{
    if ( e->orientation() == Horizontal )
        return ;

    switch ( e->state() )
    {
    case ShiftButton:
        static_cast<PlayerApp *>( kapp ) ->m_pPlayerWidget->wheelEvent( e );
        break;
    default:
        if ( e->delta() > 0 )
            static_cast<PlayerApp *>( kapp ) ->slotNext();
        else
            static_cast<PlayerApp *>( kapp ) ->slotPrev();
        break;
    }

    e->accept();
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


#include "amaroksystray.moc"
