//
// AmarokSystray
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#include "amaroksystray.h"
#include "playerapp.h"
#include "enginecontroller.h"

#include <qevent.h>
#include <kaction.h>
#include <klocale.h>
#include <kpopupmenu.h>


amaroK::TrayIcon::TrayIcon( QWidget *playerWidget, KActionCollection *ac ) : KSystemTray( playerWidget )
{
    setPixmap( KSystemTray::loadIcon("amarok") ); // @since 3.2
    setAcceptDrops( true );

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

    ac->action( "prev"  )->plug( contextMenu() );
    ac->action( "play"  )->plug( contextMenu() );
    ac->action( "pause" )->plug( contextMenu() );
    ac->action( "stop"  )->plug( contextMenu() );
    ac->action( "next"  )->plug( contextMenu() );

    QPopupMenu &p = *contextMenu();
    QStringList shortcuts; shortcuts << "" << "Z" << "X" << "C" << "V" << "B";
    QString body = "[&%1] %2";

    for( uint index = 1; index < 6; ++index )
    {
        int id = p.idAt( index );
        p.changeItem( id, body.arg( *shortcuts.at( index ), p.text( id ) ) );
    }

    contextMenu()->insertSeparator();

    ac->action( "options_configure" )->plug( contextMenu() );

    //seems to be necessary
    actionCollection()->action( "file_quit" )->disconnect();
    connect( actionCollection()->action( "file_quit" ), SIGNAL( activated() ), kapp, SLOT( quit() ) );
}

bool
amaroK::TrayIcon::event( QEvent *e )
{
    switch( e->type() ) {
    case QEvent::Wheel:
    case QEvent::DragEnter:
    case QEvent::Drop:

        //ignore() doesn't pass the event to parent unless
        //the parent isVisible() and the active window!

        //send the event to the parent PlayerWidget, it'll handle it with much wisdom
        QApplication::sendEvent( parentWidget(), e );
        return TRUE;

    case QEvent::MouseButtonPress:
        if( static_cast<QMouseEvent*>(e)->button() == Qt::MidButton )
        {
            bool isPlaying = EngineController::instance()->engine() ? EngineController::instance()->engine()->loaded() : false;
            if( isPlaying ) EngineController::instance()->pause();
            else EngineController::instance()->play();
            return TRUE;
        }
    default:
        return KSystemTray::event( e );
    }
}

#include "amaroksystray.moc"
