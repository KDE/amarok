//
// AmarokSystray
//
// Contributors: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//               berkus, mxcl, eros, eean
//
// Copyright: like rest of Amarok
//

#include "systray.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "EngineController.h"
#include "playlist/PlaylistModel.h"
#include "meta/Meta.h"
#include "meta/MetaConstants.h"
#include "meta/CurrentTrackActionsCapability.h"
#include "TheInstances.h"

#include <KAction>
#include <KApplication>
#include <KIconEffect>
#include <KLocale>
#include <KMenu>
#include <KStandardDirs>

#include <QEvent>
#include <QImage>
#include <QMouseEvent>
#include <QPixmap>
#include <QTimerEvent>


namespace Amarok
{
    static QImage
    loadOverlay( const char *iconName )
    {
        return QImage( KStandardDirs::locate( "data", QString( "amarok/images/b_%1.png" ).arg( iconName ) ), "PNG" )
                                                      .scaled( 10, 10, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    }
}


Amarok::TrayIcon::TrayIcon( QWidget *playerWidget )
        : KSystemTrayIcon( playerWidget )
        , EngineObserver( EngineController::instance() )
        , trackLength( 0 )
        , mergeLevel( -1 )
        , overlay( 0 )
        , blinkTimerID( 0 )
        , overlayVisible( false )
{
    DEBUG_BLOCK

    PERF_LOG( "Beginning TrayIcon Constructor" );
    KActionCollection* const ac = Amarok::actionCollection();

    //seems to be necessary
    QAction *quit = actionCollection()->action( "file_quit" );
    quit->disconnect();
    connect( quit, SIGNAL(activated()), kapp, SLOT(quit()) );

    PERF_LOG( "Before adding actions" );
    contextMenu()->addAction( ac->action( "prev"       ) );
    //contextMenu()->addAction( ac->action( "play_pause" ) );
    contextMenu()->addAction( ac->action( "play_pause" ) );
    contextMenu()->addAction( ac->action( "next"       ) );

    baseIcon     = KSystemTrayIcon::loadIcon( "amarok" );
    playOverlay  = QPixmap::fromImage( Amarok::loadOverlay( "play" ) );
    pauseOverlay = QPixmap::fromImage( Amarok::loadOverlay( "pause" ) );
    overlayVisible = false;

    //paintIcon();
    PERF_LOG("Adding Icon");
    setIcon( baseIcon );
}

bool
Amarok::TrayIcon::event( QEvent *e )
{
    switch( e->type() )
    {
    case QEvent::DragEnter:
        #define e static_cast<QDragEnterEvent*>(e)
        {
            e->setAccepted( KUrl::List::canDecode( e->mimeData() ) );
            break;
        }
        #undef e

    case QEvent::Drop:
        #define e static_cast<QDropEvent*>(e)
        {
            KUrl::List list = KUrl::List::fromMimeData( e->mimeData() );
            if( !list.isEmpty() )
            {
                KMenu *popup = new KMenu;
                popup->addAction( KIcon( "list-add-amarok" ), i18n( "&Append to Playlist" ), this, SLOT( appendDrops() ) );
                popup->addAction( KIcon( "list-add-amarok" ), i18n( "Append && &Play" ), this, SLOT( appendAndPlayDrops() ) );
                if( The::playlistModel()->activeRow() >= 0 )
                    popup->addAction( KIcon( "go-next-amarok" ), i18n( "&Queue Track" ), this, SLOT( queueDrops() ) );

                popup->addSeparator();
                popup->addAction( i18n( "&Cancel" ) );
                popup->exec( e->pos() );
            }
            break;
        }
        #undef e

    case QEvent::Wheel:
        #define e static_cast<QWheelEvent*>(e)
        if( e->modifiers() == Qt::ControlModifier )
        {
            const bool up = e->delta() > 0;
            if( up ) The::playlistModel()->back();
            else     The::playlistModel()->next();
            break;
        }
        else if( e->modifiers() == Qt::ShiftModifier )
        {
            EngineController::instance()->seekRelative( (e->delta() / 120) * 5000 ); // 5 seconds for keyboard seeking
            break;
        }
        else
            EngineController::instance()->increaseVolume( e->delta() / Amarok::VOLUME_SENSITIVITY );

        e->accept();
        #undef e
        break;

    case QEvent::Timer:
        if( static_cast<QTimerEvent*>(e)->timerId() != blinkTimerID )
            return KSystemTrayIcon::event( e );

        // if we're playing, blink icon
        if ( overlay == &playOverlay )
        {
            overlayVisible = !overlayVisible;
            paintIcon( mergeLevel, true );
        }

        break;

    case QEvent::MouseButtonPress:
        if( static_cast<QMouseEvent*>(e)->button() == Qt::MidButton )
        {
            EngineController::instance()->playPause();

            return true;
        }

        //else FALL THROUGH

    default:
        return KSystemTrayIcon::event( e );
    }
    return true;
}

void
Amarok::TrayIcon::engineStateChanged( Phonon::State state, Phonon::State /*oldState*/ )
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
        case Phonon::PausedState:
            overlay = &pauseOverlay;
            paintIcon( mergeLevel, true );
            break;

        case Phonon::PlayingState:
            overlay = &playOverlay;
            if( AmarokConfig::animateTrayIcon() )
            blinkTimerID = startTimer( 1500 );  // start 'blink' timer

            paintIcon( mergeLevel, true ); // repaint the icon
            setupMenu();
            break;

        case Phonon::StoppedState:
        case Phonon::LoadingState:
            overlayVisible = false;
            paintIcon( -1, true ); // repaint the icon
                                // fall through to default:

        case Phonon::ErrorState:
        case Phonon::BufferingState:
            break;
    }
}

void
Amarok::TrayIcon::engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged )
{
    Q_UNUSED( trackChanged )
    trackLength = newMetaData.value( Meta::valLength ).toInt() * 1000;

    setupMenu();

}

void
Amarok::TrayIcon::engineTrackPositionChanged( long position, bool /*userSeek*/ )
{
/*
    mergeLevel = trackLength ? ((baseIcon.height() + 1) * position) / trackLength : -1;
    paintIcon( mergeLevel );
*/
}


void
Amarok::TrayIcon::paletteChange( const QPalette & op )
{
/*
    if ( palette().active().highlight() == op.active().highlight() || alternateIcon.isNull() )
        return;

    alternateIcon.resize( 0, 0 );
    paintIcon( mergeLevel, true );
*/
}

void
Amarok::TrayIcon::paintIcon( int mergePixels, bool force )
{
    // skip redrawing the same pixmap
    static int mergePixelsCache = 0;
    if ( mergePixels == mergePixelsCache && !force )
         return;
    mergePixelsCache = mergePixels;

    if ( mergePixels < 0 ) {}
        //return blendOverlay( baseIcon );

    // make up the grayed icon
    if ( grayedIcon.isNull() )
    {
        //QImage tmpTrayIcon = baseIcon.convertToImage();
        //KIconEffect::semiTransparent( tmpTrayIcon );
        //grayedIcon = tmpTrayIcon;
    }

    // make up the alternate icon (use hilight color but more saturated)
    if ( alternateIcon.isNull() )
    {
        #if 0
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
        #endif
    }

    if ( mergePixels >= alternateIcon.height() ) {}
        //return blendOverlay( grayedIcon );
    if ( mergePixels == 0 ) {}
        //return blendOverlay( alternateIcon );

    // mix [ grayed <-> colored ] icons
    QPixmap tmpTrayPixmap = alternateIcon;
    copyBlt( &tmpTrayPixmap, 0,0, &grayedIcon, 0,0,
            alternateIcon.width(), mergePixels>0 ? mergePixels-1 : 0 );
    blendOverlay( tmpTrayPixmap );
}

void
Amarok::TrayIcon::blendOverlay( QPixmap &sourcePixmap )
{
    #if 0
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
    #endif
}

void Amarok::TrayIcon::setupMenu()
{
    Meta::TrackPtr track = EngineController::instance()->currentTrack();

    foreach( QAction * action, m_extraActions ) {
        contextMenu()->removeAction( action );
    }

    if ( track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) ) {
        Meta::CurrentTrackActionsCapability *cac = track->as<Meta::CurrentTrackActionsCapability>();
        if( cac )
        {
            //remove the two bottom itmes, so we can push them to the button again
            contextMenu()->removeAction( actionCollection()->action( "file_quit" ) );
            contextMenu()->removeAction( actionCollection()->action( "minimizeRestore" ) );


            m_extraActions = cac->customActions();

            if ( contextMenu()->actions().size() < 5 )
                m_extraActions.append( contextMenu()->addSeparator() );

            foreach( QAction *action, m_extraActions )
                contextMenu()->addAction( action );
            m_extraActions.append( contextMenu()->addSeparator() );

            //readd
            contextMenu()->addAction( actionCollection()->action( "minimizeRestore" ) );
            contextMenu()->addAction( actionCollection()->action( "file_quit" ) );

        }

    }
}
