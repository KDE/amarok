//
// AmarokSystray
//
// Contributors: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//               berkus, mxcl, eros
//
// Copyright: like rest of amaroK
//

#include "amarok.h"
#include "amarokconfig.h"
#include "enginecontroller.h"
#include "systray.h"

#include <qevent.h>
#include <qimage.h>
#include <kaction.h>
#include <kapplication.h>
#include <kpopupmenu.h>
#include <kiconeffect.h>
#include <kstandarddirs.h>

namespace amaroK
{
    static QPixmap
    loadOverlay( const char *iconName )
    {
        return QImage( locate( "data", QString( "amarok/images/b_%1.png" ).arg( iconName ) ), "PNG" ).smoothScale( 10, 10 );
    }
}


amaroK::TrayIcon::TrayIcon( QWidget *playerWidget )
        : KSystemTray( playerWidget )
        , EngineObserver( EngineController::instance() )
        , trackLength( 0 )
        , mergeLevel( -1 )
        , overlay( 0 )
        , blinkTimerID( 0 )
        , overlayVisible( false )
{
    KActionCollection* const ac = amaroK::actionCollection();

    setAcceptDrops( true );

    ac->action( "prev"        )->plug( contextMenu() );
    ac->action( "play_pause"  )->plug( contextMenu() );
    ac->action( "stop"        )->plug( contextMenu() );
    ac->action( "next"        )->plug( contextMenu() );

    //seems to be necessary
    KAction *quit = actionCollection()->action( "file_quit" );
    quit->disconnect();
    connect( quit, SIGNAL(activated()), kapp, SLOT(quit()) );

    baseIcon     = KSystemTray::loadIcon( "amarok" );
    playOverlay  = amaroK::loadOverlay( "play" );
    pauseOverlay = amaroK::loadOverlay( "pause" );
    overlayVisible = false;

    //paintIcon();
    setPixmap( baseIcon );
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

    case QEvent::Timer:
        if( static_cast<QTimerEvent*>(e)->timerId() != blinkTimerID )
            return KSystemTray::event( e );

        // if we're playing, blink icon
        if ( overlay == &playOverlay )
        {
            overlayVisible = !overlayVisible;
            paintIcon( mergeLevel, true );
        }

        return true;

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
amaroK::TrayIcon::engineStateChanged( Engine::State state, Engine::State /*oldState*/ )
{
    // stop timer
    if ( blinkTimerID )
    {
        killTimer( blinkTimerID );
        blinkTimerID = 0;
    }
    // draw overlay
    overlayVisible = true;

    // draw the right overlay for each state
    switch( state )
    {
    case Engine::Paused:
        overlay = &pauseOverlay;
        paintIcon( mergeLevel, true );
        break;

    case Engine::Playing:
        overlay = &playOverlay;
        if( AmarokConfig::animateTrayIcon() )
           blinkTimerID = startTimer( 1500 );  // start 'blink' timer

        paintIcon( mergeLevel, true ); // repaint the icon
        break;

    case Engine::Empty:
        overlayVisible = false;
        paintIcon( -1, true ); // repaint the icon

    default:
        ;
    }
}

void
amaroK::TrayIcon::engineNewMetaData( const MetaBundle &bundle, bool /*trackChanged*/ )
{
    trackLength = bundle.length() * 1000;
}

void
amaroK::TrayIcon::engineTrackPositionChanged( long position, bool /*userSeek*/ )
{
    mergeLevel = trackLength ? ((baseIcon.height() + 1) * position) / trackLength : -1;
    paintIcon( mergeLevel );
}

void
amaroK::TrayIcon::paletteChange( const QPalette & op )
{
    if ( palette().active().highlight() == op.active().highlight() || alternateIcon.isNull() )
        return;

    alternateIcon.resize( 0, 0 );
    paintIcon( mergeLevel, true );
}

void
amaroK::TrayIcon::paintIcon( int mergePixels, bool force )
{
    // skip redrawing the same pixmap
    static int mergePixelsCache = 0;
    if ( mergePixels == mergePixelsCache && !force )
         return;
    mergePixelsCache = mergePixels;

    if ( mergePixels < 0 )
        return blendOverlay( baseIcon );

    // make up the grayed icon
    if ( grayedIcon.isNull() )
    {
        QImage tmpTrayIcon = baseIcon.convertToImage();
        KIconEffect::semiTransparent( tmpTrayIcon );
        grayedIcon = tmpTrayIcon;
    }
    if ( mergePixels == 0 )
        return blendOverlay( grayedIcon );

    // make up the alternate icon (use hilight color but more saturated)
    if ( alternateIcon.isNull() )
    {
        QImage tmpTrayIcon = baseIcon.convertToImage();
        // eros: this looks cool with dark red blue or green but sucks with
        // other colors (such as kde default's pale pink..). maybe the effect
        // or the blended color has to be changed..
        QColor saturatedColor = palette().active().highlight();
        int hue, sat, value;
        saturatedColor.getHsv( &hue, &sat, &value );
        saturatedColor.setHsv( hue, sat > 200 ? 200 : sat, value < 100 ? 100 : value );
        KIconEffect::colorize( tmpTrayIcon, saturatedColor/* Qt::blue */, 0.9 );
        alternateIcon = tmpTrayIcon;
    }
    if ( mergePixels >= alternateIcon.height() )
        return blendOverlay( alternateIcon );

    // mix [ grayed <-> colored ] icons
    QPixmap tmpTrayPixmap = alternateIcon;
    copyBlt( &tmpTrayPixmap, 0,0, &grayedIcon, 0,0,
            alternateIcon.width(), alternateIcon.height() - mergePixels );
    blendOverlay( tmpTrayPixmap );
}

void
amaroK::TrayIcon::blendOverlay( QPixmap &sourcePixmap )
{
    if ( !overlayVisible || !overlay || overlay->isNull() )
        return setPixmap( sourcePixmap ); // @since 3.2

    // here comes the tricky part.. no kdefx functions are helping here.. :-(
    // we have to blend pixmaps with different sizes (blending will be done in
    // the bottom-left corner of source pixmap with a smaller overlay pixmap)
    int opW = overlay->width(),
        opH = overlay->height(),
        opX = 1,
        opY = sourcePixmap.height() - opH;

    // get the rectangle where blending will take place
    QPixmap sourceCropped( opW, opH, sourcePixmap.depth() );
    copyBlt( &sourceCropped, 0,0, &sourcePixmap, opX,opY, opW,opH );

    //speculative fix for a bactrace we received
    //crash was in covertToImage() somewhere in this function
    if( sourceCropped.isNull() )
        return setPixmap( sourcePixmap );

    // blend the overlay image over the cropped rectangle
    QImage blendedImage = sourceCropped.convertToImage();
    QImage overlayImage = overlay->convertToImage();
    KIconEffect::overlay( blendedImage, overlayImage );
    sourceCropped.convertFromImage( blendedImage );

    // put back the blended rectangle to the original image
    QPixmap sourcePixmapCopy = sourcePixmap;
    copyBlt( &sourcePixmapCopy, opX,opY, &sourceCropped, 0,0, opW,opH );

    setPixmap( sourcePixmapCopy ); // @since 3.2
}
