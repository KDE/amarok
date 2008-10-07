/******************************************************************************
 * Copyright (c) 2003 Stanislav Karchebny <berkus@users.sf.net>               *
 * Copyright (c) 2003 Max Howell <max.howell@methylblue.com>                  *
 * Copyright (c) 2004 Enrico Ros <eros.kde@email.it>                          *
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                              *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "Systray.h"

#include "Amarok.h"
#include "EngineController.h"
#include "TrackTooltip.h"
#include "amarokconfig.h"
#include "Debug.h"
#include "meta/CurrentTrackActionsCapability.h"
#include "meta/Meta.h"
#include "meta/MetaConstants.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModel.h"
#include "context/popupdropper/PopupDropperAction.h"

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
        QImage img = QImage( KStandardDirs::locate( "data", QString( "amarok/images/b_%1.png" ).arg( iconName ) ), "PNG" );
        if (!img.isNull())
            img = img.scaled( 10, 10, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
        return img;
    }

    TrayIcon* TrayIcon::s_instance = 0;
}


Amarok::TrayIcon::TrayIcon( QWidget *playerWidget )
        : KSystemTrayIcon( playerWidget )
        , EngineObserver( The::engineController() )
        , trackLength( 0 )
        , mergeLevel( -1 )
        , overlay( 0 )
        , blinkTimerID( 0 )
        , overlayVisible( false )
{
    DEBUG_BLOCK

    s_instance = this;

    PERF_LOG( "Beginning TrayIcon Constructor" );
    KActionCollection* const ac = Amarok::actionCollection();

    //seems to be necessary
    QAction *quit = actionCollection()->action( "file_quit" );
    quit->disconnect();
    connect( quit, SIGNAL(activated()), kapp, SLOT(quit()) );

    PERF_LOG( "Before adding actions" );
    contextMenu()->addAction( ac->action( "prev"       ) );
    contextMenu()->addAction( ac->action( "play_pause" ) );
    contextMenu()->addAction( ac->action( "stop"       ) );
    contextMenu()->addAction( ac->action( "next"       ) );

    baseIcon     = KSystemTrayIcon::loadIcon( "amarok" );
    playOverlay  = QPixmap::fromImage( Amarok::loadOverlay( "play" ) );
    pauseOverlay = QPixmap::fromImage( Amarok::loadOverlay( "pause" ) );
    overlayVisible = false;

    //paintIcon();
    PERF_LOG("Adding Icon");
    setIcon( baseIcon );

    connect( this, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ), SLOT( slotActivated( QSystemTrayIcon::ActivationReason ) ) );
}

bool
Amarok::TrayIcon::event( QEvent *e )
{
    switch( e->type() )
    {
    case QEvent::DragEnter:
        #define e static_cast<QDragEnterEvent*>(e)
        e->setAccepted( KUrl::List::canDecode( e->mimeData() ) );
        break;
        #undef e

    case QEvent::ToolTip:
        DEBUG_LINE_INFO
        TrackToolTip::instance()->show( static_cast<QHelpEvent*>(e)->globalPos() );
        return true;

    case QEvent::Drop:
        #define e static_cast<QDropEvent*>(e)
        {
            const KUrl::List list = KUrl::List::fromMimeData( e->mimeData() );
            if( !list.isEmpty() )
            {
                KMenu *popup = new KMenu;
                popup->addAction( KIcon( "media-track-add-amarok" ), i18n( "&Append to Playlist" ), this, SLOT( appendDrops() ) );
                popup->addAction( KIcon( "media-track-add-amarok" ), i18n( "Append && &Play" ), this, SLOT( appendAndPlayDrops() ) );
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
            if( up ) The::playlistActions()->back();
            else     The::playlistActions()->next();
            break;
        }
        else if( e->modifiers() == Qt::ShiftModifier )
        {
            The::engineController()->seekRelative( (e->delta() / 120) * 5000 ); // 5 seconds for keyboard seeking
            break;
        }
        else
            The::engineController()->increaseVolume( e->delta() / Amarok::VOLUME_SENSITIVITY );

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
            // FALL THROUGH

        case Phonon::ErrorState:
        case Phonon::BufferingState:
            break;
    }
}

void
Amarok::TrayIcon::engineNewTrackPlaying( )
{
    setupMenu();
}

void
Amarok::TrayIcon::engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged )
{
    Q_UNUSED( trackChanged )
    trackLength = newMetaData.value( Meta::valLength ).toInt() * 1000;

    setupMenu();
}

void
Amarok::TrayIcon::engineTrackPositionChanged( long position, bool userSeek )
{
    //AMAROK_NOTIMPLEMENTED
    Q_UNUSED( position )
    Q_UNUSED( userSeek )
/*
    mergeLevel = trackLength ? ((baseIcon.height() + 1) * position) / trackLength : -1;
    paintIcon( mergeLevel );
*/
}

void
Amarok::TrayIcon::paletteChange( const QPalette & op )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( op )
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
    Q_UNUSED( mergePixels )
    Q_UNUSED( force )

#if 0
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
#endif
}

void
Amarok::TrayIcon::blendOverlay( QPixmap &sourcePixmap )
{
    Q_UNUSED( sourcePixmap )

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

void
Amarok::TrayIcon::setupMenu()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track ) return;

    foreach( QAction * action, m_extraActions ) {
        contextMenu()->removeAction( action );
    }

    if ( track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) ) {
        Meta::CurrentTrackActionsCapability *cac = track->as<Meta::CurrentTrackActionsCapability>();
        if( cac )
        {
            //remove the two bottom items, so we can push them to the button again
            contextMenu()->removeAction( actionCollection()->action( "file_quit" ) );
            contextMenu()->removeAction( actionCollection()->action( "minimizeRestore" ) );

            m_extraActions = cac->customActions();

            //if ( contextMenu()->actions().size() < 5 )
                //m_extraActions.append( contextMenu()->addSeparator() );

            foreach( QAction *action, m_extraActions )
                contextMenu()->addAction( action );
            //m_extraActions.append( contextMenu()->addSeparator() );

            //readd
        #ifndef Q_WS_MAC
            contextMenu()->addAction( actionCollection()->action( "minimizeRestore" ) );
        #endif
            contextMenu()->addAction( actionCollection()->action( "file_quit" ) );
        }
    }
}

void
Amarok::TrayIcon::slotActivated( QSystemTrayIcon::ActivationReason reason )
{
    if( reason == QSystemTrayIcon::MiddleClick )
        The::engineController()->playPause();
}


#include "Systray.moc"

