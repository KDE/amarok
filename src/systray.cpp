//
// AmarokSystray
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#include "app.h"
#include "enginecontroller.h"
#include "systray.h"

#include <qevent.h>
#include <kaction.h>
#include <kpopupmenu.h>


amaroK::TrayIcon::TrayIcon( QWidget *playerWidget )
  : KSystemTray( playerWidget )
{
    KActionCollection* const ac = pApp->actionCollection();

    setPixmap( KSystemTray::loadIcon("amarok") ); // @since 3.2
    setAcceptDrops( true );

    ac->action( "prev"  )->plug( contextMenu() );
    ac->action( "play"  )->plug( contextMenu() );
    ac->action( "pause" )->plug( contextMenu() );
    ac->action( "stop"  )->plug( contextMenu() );
    ac->action( "next"  )->plug( contextMenu() );

    QPopupMenu &p = *contextMenu();
    QStringList shortcuts; shortcuts << "" << "Z" << "X" << "C" << "V" << "B";
    const QString body = "|&%1| %2";

    for( uint index = 1; index < 6; ++index )
    {
        int id = p.idAt( index );
        p.changeItem( id, body.arg( *shortcuts.at( index ), p.text( id ) ) );
    }

    //seems to be necessary
    KAction *quit = actionCollection()->action( "file_quit" );
    quit->disconnect();
    connect( quit, SIGNAL( activated() ), kapp, SLOT( quit() ) );
}

bool
amaroK::TrayIcon::event( QEvent *e )
{
    switch( e->type() )
    {
    case QEvent::Drop:
    case QEvent::Wheel:
    case QEvent::DragEnter:
        pApp->genericEventHandler( this, e );
        return TRUE;

    case QEvent::MouseButtonPress:
        if( static_cast<QMouseEvent*>(e)->button() == Qt::MidButton )
        {
            EngineController::instance()->playPause();

            return TRUE;
        }

        //else FALL THROUGH

    default:
        return KSystemTray::event( e );
    }
}
