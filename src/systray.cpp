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
#include <kstandarddirs.h>


amaroK::TrayIcon::TrayIcon( QWidget *playerWidget )
  : KSystemTray( playerWidget ),
  baseIcon( 0 ), grayedIcon( 0 ), alternateIcon( 0 ),
  playOverlay( 0 ), pauseOverlay( 0 ), stopOverlay( 0 ),
  trackLength( 0 ), trackPercent( -1 ), drawnPercent( -1 )
{
    KActionCollection* const ac = amaroK::actionCollection();

    paintIcon( 100, OV_none );
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
    // draw the right overlay for each state
    switch( state )
    {
        case Engine::Paused:
            drawnPercent = -1;  //force repaint
            paintIcon( trackPercent, OV_pause );
            break;

        case Engine::Playing:
            drawnPercent = -1;  //force repaint on next paint op
            break;

        default: //if idle/stopped
            drawnPercent = -1;  //force repaint
            paintIcon( 100, OV_stop );
            break;
    }
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
    paintIcon( trackPercent, OV_play );
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
    paintIcon( trackPercent, OV_none );
}

QPixmap *
amaroK::TrayIcon::loadOverlay( const char * iconName )
{
    QPixmap icon( locate( "data", QString( "amarok/images/b_%1.png" ).arg( iconName ) ), "PNG" );
    return new QPixmap( icon.convertToImage().smoothScale( 12, 12 ) );
}

void
amaroK::TrayIcon::paintIcon( int percent, TrayOverlay overlay )
{
    // skip redrawing the same pixmap
    if ( percent == drawnPercent )
        return;
    drawnPercent = percent;

    // load the base trayIcon
    if ( !baseIcon )
        baseIcon = new QPixmap( KSystemTray::loadIcon("amarok") );
    if ( percent > 99 )
    {
        blendOverlay( baseIcon, overlay );
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
        blendOverlay( grayedIcon, overlay );
        return;
    }

    // make up the alternate icon (use hilight color but more saturated)
    if ( !alternateIcon )
    {
        QImage tmpTrayIcon = baseIcon->convertToImage();
        // eros: this looks cool with dark red blue or green but sucks with
        // other colors (such as kde default's pale pink..). maybe the effect
        // or the blended color has to be changed..
        QColor saturatedColor = palette().active().highlight();
        int hue, sat, value;
        saturatedColor.getHsv( &hue, &sat, &value );
        saturatedColor.setHsv( hue, (sat + 255) / 2, value );
        KIconEffect::colorize( tmpTrayIcon, saturatedColor, 0.9 );
        alternateIcon = new QPixmap( tmpTrayIcon );
    }

    // mix [ grayed <-> colored ] icons
    QPixmap tmpTrayPixmap( *grayedIcon );
    int height = grayedIcon->height(),
        sourceH = 1 + (height * percent) / 100, // 1 .. height
        sourceY = height - sourceH;             // height-1 .. 0
    copyBlt( &tmpTrayPixmap, 0,sourceY, alternateIcon, 0,sourceY,
            grayedIcon->width(), sourceH );
    blendOverlay( &tmpTrayPixmap, overlay );
}

void
amaroK::TrayIcon::blendOverlay( QPixmap * sourcePixmap, TrayOverlay overlay )
{
    // load and cache overlay pixmaps (on demand)
    QPixmap * overlayPixmap = 0;
    switch( overlay )
    {
        case OV_play:
            if ( !playOverlay )
                playOverlay = loadOverlay( "play" );
            overlayPixmap = playOverlay;
            break;
        case OV_pause:
            if ( !pauseOverlay )
                pauseOverlay = loadOverlay( "pause" );
            overlayPixmap = pauseOverlay;
            break;
        case OV_stop:
            if ( !stopOverlay )
                stopOverlay = loadOverlay( "stop" );
            overlayPixmap = stopOverlay;
            break;
        default:
            ;
    }

    if ( overlayPixmap && !overlayPixmap->isNull() )
    {
        // here comes the tricky part.. no kdefx functions are helping here.. :-(
        // we have to blend pixmaps with different sizes (blending will be done in
        // the bottom-left corner of source pixmap with a smaller overlay pixmap)
        int opW = overlayPixmap->width(),
            opH = overlayPixmap->height(),
            opX = 0,
            opY = sourcePixmap->height() - opH;
    
        // get the rectangle where blending will take place 
        QPixmap sourceCropped( opW, opH, sourcePixmap->depth() );
        copyBlt( &sourceCropped, 0,0, sourcePixmap, opX,opY, opW,opH );
    
        // blend the overlay image over the cropped rectangle
        QImage blendedImage = sourceCropped.convertToImage();
        QImage overlayImage = overlayPixmap->convertToImage();
        KIconEffect::overlay( blendedImage, overlayImage );
        sourceCropped.convertFromImage( blendedImage );
        
        // put back the blended rectangle to the original image
        QPixmap sourcePixmapCopy = *sourcePixmap;
        copyBlt( &sourcePixmapCopy, opX,opY, &sourceCropped, 0,0, opW,opH );
    
        //
        setPixmap( sourcePixmapCopy ); // @since 3.2
    } else
        setPixmap( *sourcePixmap ); // @since 3.2
}
