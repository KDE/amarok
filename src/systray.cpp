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


amaroK::TrayIcon::TrayIcon( QWidget *playerWidget, KActionCollection *ac ) : KSystemTray( playerWidget )
{
    setPixmap( KSystemTray::loadIcon("amarok") ); // @since 3.2
    setAcceptDrops( true );

    ac->action( "prev"  )->plug( contextMenu() );
    ac->action( "play"  )->plug( contextMenu() );
    ac->action( "pause" )->plug( contextMenu() );
    ac->action( "stop"  )->plug( contextMenu() );
    ac->action( "next"  )->plug( contextMenu() );

    QPopupMenu &p = *contextMenu();
    QStringList shortcuts; shortcuts << "" << "Z" << "X" << "C" << "V" << "B";
    QString body = "|&%1| %2";

    for( uint index = 1; index < 6; ++index )
    {
        int id = p.idAt( index );
        p.changeItem( id, body.arg( *shortcuts.at( index ), p.text( id ) ) );
    }

    contextMenu()->insertSeparator();

    ac->action( "options_configure" )->plug( contextMenu() );

    //seems to be necessary
    KAction *quit = actionCollection()->action( "file_quit" );
    quit->disconnect();
    connect( quit, SIGNAL( activated() ), kapp, SLOT( quit() ) );
}

#include <klocale.h>
bool
amaroK::TrayIcon::event( QEvent *e )
{
    switch( e->type() ) {
    case QEvent::Drop:
    {
        QPopupMenu popup;

        //popup.insertItem( i18n( "Play" ) );
        popup.insertItem( i18n( "Append and &Play" ) );
        popup.insertItem( i18n( "&Append" ) );
        popup.insertSeparator();
        popup.insertItem( i18n( "&Cancel" ) );

        popup.exec( mapToGlobal( static_cast<QDropEvent*>(e)->pos() ) );

        return TRUE;
    }
    case QEvent::Wheel:
        EngineController::instance()->setVolume( EngineController::engine()->volume() +
                                                 static_cast<QWheelEvent*>(e)->delta() / 18 );
        return TRUE;
    
    case QEvent::DragEnter:
        //ignore() doesn't pass the event to parent unless
        //the parent isVisible() and the active window!

        //send the event to the parent PlayerWidget, it'll handle it with much wisdom
        QApplication::sendEvent( parentWidget(), e );
        return TRUE;

    case QEvent::MouseButtonPress:
        if( static_cast<QMouseEvent*>(e)->button() == Qt::MidButton )
        {
            EngineController::instance()->playPause();

            return TRUE;
        }

    default:
        return KSystemTray::event( e );
    }
}

#include "systray.moc"
