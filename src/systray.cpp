//
// AmarokSystray
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#include "amarok.h"
#include "enginecontroller.h"
#include "systray.h"

#include <qevent.h>
#include <qimage.h>
#include <kaction.h>
#include <kapplication.h>
#include <kpopupmenu.h>
#include <kiconeffect.h>


amaroK::TrayIcon::TrayIcon( QWidget *playerWidget )
  : KSystemTray( playerWidget ),
  baseIcon( 0 ), grayedIcon( 0 ), alternateIcon( 0 ),
  trackLength( 0 ), trackPercent( -1 ), drawnPercent( -1 )
{
    KActionCollection* const ac = amaroK::actionCollection();

    paintProgressIcon();
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

    // attach to get notified about engine events
    EngineController::instance()->attach( this );
}

amaroK::TrayIcon::~TrayIcon( )
{
    EngineController::instance()->detach( this );
    delete baseIcon;
    delete grayedIcon;
    delete alternateIcon;
}

bool
amaroK::TrayIcon::event( QEvent *e )
{
    switch( e->type() )
    {
    case QEvent::Drop:
    case QEvent::Wheel:
    case QEvent::DragEnter:
        return amaroK::genericEventHandler( this, e );

    case QEvent::MouseButtonPress:
        if( static_cast<QMouseEvent*>(e)->button() == Qt::MidButton )
        {
            EngineController::instance()->playPause();

            return true;
        }

        //else FALL THROUGH

    default:
        return KSystemTray::event( e );
    }
}

void
amaroK::TrayIcon::engineStateChanged( Engine::State state )
{
    // draw the base icon if stopped or idle
    if ( state != Engine::Playing && state != Engine::Paused )
        paintProgressIcon();
}

void
amaroK::TrayIcon::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    trackLength = bundle.length() * 1000;
}

void
amaroK::TrayIcon::engineTrackPositionChanged( long position )
{
    trackPercent = trackLength ? (100 * position) / trackLength : 100;
    paintProgressIcon( trackPercent );
}

void
amaroK::TrayIcon::paletteChange( const QPalette & op )
{
    if ( palette().active().highlight() == op.active().highlight() || !alternateIcon )
        return;
        
    delete alternateIcon;
    alternateIcon = 0;
    // force pixmap rebuild and repaint
    drawnPercent = -1;
    paintProgressIcon( trackPercent );
}

void
amaroK::TrayIcon::paintProgressIcon( int percent )
{
    // skip redrawing the same pixmap
    if ( percent == drawnPercent )
        return;
    drawnPercent = percent;

    // load the base trayIcon
    if ( !baseIcon )
        baseIcon = new QPixmap( KSystemTray::loadIcon("amarok") );
    if ( percent > 99  )
    {
        setPixmap( *baseIcon ); // @since 3.2
        return;
    }

    // make up the grayed icon
    if ( !grayedIcon )
    {
        QImage tmpTrayIcon = baseIcon->convertToImage();
        KIconEffect::semiTransparent( tmpTrayIcon );
        grayedIcon = new QPixmap( tmpTrayIcon );
    }
    if ( percent < 1  )
    {
        setPixmap( *grayedIcon ); // @since 3.2
        return;
    }
    
    // make up the colored icon
    if ( !alternateIcon )
    {
        QImage tmpTrayIcon = baseIcon->convertToImage();
        // eros: this looks cool with dark red blue or green but sucks with
        // other colors (such as kde default's pale pink..). maybe the effect
        // or the blended color has to be changed..
        KIconEffect::colorize( tmpTrayIcon, palette().active().highlight().dark(105), 0.9 );
        alternateIcon = new QPixmap( tmpTrayIcon );
    }

    // mix [ grayed <-> colored ] icons
    QPixmap tmpTrayPixmap( *grayedIcon );
    int height = grayedIcon->height(),
        sourceH = 1 + (height * percent) / 100, // 1 .. height
        sourceY = height - sourceH;             // height-1 .. 0
    copyBlt( &tmpTrayPixmap, 0,sourceY, alternateIcon, 0,sourceY,
             grayedIcon->width(), sourceH );
    setPixmap( tmpTrayPixmap );
}
