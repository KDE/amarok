/****************************************************************************************
 * Copyright (c) 2003 Stanislav Karchebny <berkus@users.sf.net>                         *
 * Copyright (c) 2003 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2004 Enrico Ros <eros.kde@email.it>                                    *
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2009,2010 Kevin Funk <krf@electrostorm.net>                            *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TrayIcon.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "amarokconfig.h"
#include "GlobalCurrentTrackActions.h"
#include "core/capabilities/CurrentTrackActionsCapability.h"
#include "playlist/PlaylistActions.h"
#include "SvgHandler.h"
#include <KAboutData>
#include <KAction>
#include <KCmdLineArgs>
#include <KLocale>
#include <KMenu>
#include <KStandardDirs>

#include <QAction>
#include <QFontMetrics>
#include <QPixmap>
#include <QToolTip>

Amarok::TrayIcon::TrayIcon( QObject *parent )
        : KStatusNotifierItem( parent )
        , Engine::EngineObserver( The::engineController() )
{
    DEBUG_BLOCK

    PERF_LOG( "Beginning TrayIcon Constructor" );
    KActionCollection* const ac = Amarok::actionCollection();

    setStatus( KStatusNotifierItem::Active );

    PERF_LOG( "Before adding actions" );

#ifdef Q_WS_MAC
    // Add these functions to the dock icon menu in OS X
    extern void qt_mac_set_dock_menu(QMenu *);
    qt_mac_set_dock_menu( contextMenu() );
    contextMenu()->addAction( ac->action( "playlist_playmedia" ) );
    contextMenu()->addSeparator();
#endif

    contextMenu()->addAction( ac->action( "prev"       ) );
    contextMenu()->addAction( ac->action( "play_pause" ) );
    contextMenu()->addAction( ac->action( "stop"       ) );
    contextMenu()->addAction( ac->action( "next"       ) );
    contextMenu()->setObjectName( "TrayIconContextMenu" );

    PERF_LOG( "Adding system tray icon" );

    setIconByName( "amarok" );

    setupToolTip( true );

    connect( this, SIGNAL( scrollRequested( int, Qt::Orientation ) ), SLOT( slotScrollRequested(int, Qt::Orientation) ) );
    connect( this, SIGNAL( secondaryActivateRequested( const QPoint & ) ), SLOT( slotActivated() ) );
}

void
Amarok::TrayIcon::setupToolTip( bool updateIcon )
{
    if( m_track )
    {
        setToolTipTitle( The::engineController()->prettyNowPlaying() );

        // check if we really need to update the icon (performance tweak)
        if( updateIcon )
        {
            if( m_track->album() && m_track->album()->hasImage() )
            {
                QPixmap image = The::svgHandler()->imageWithBorder( m_track->album(), KIconLoader::SizeLarge, 5 );
                setToolTipIconByPixmap( image );
            }
            else
            {
                setToolTipIconByName( "amarok" );
            }
        }

        QStringList tooltip;

        QString volume;
        if ( The::engineController()->isMuted() )
        {
            volume = i18n( "Muted" );
        }
        else
        {
            volume = QString( "%1%" ).arg( The::engineController()->volume() );
        }
        tooltip << QString("<i>%1: %2</i>").arg( i18n( "Volume" ) ).arg( volume );

        const float score = m_track->score();
        if( score > 0.f )
        {
            tooltip << QString( "%1: %2" ).arg( i18n( "Score" ) ).arg( QString::number( score, 'f', 2 ) );
        }

        const int rating = m_track->rating();
        if( rating > 0 )
        {
            QString stars;
            for( int i = 0; i < rating / 2; ++i )
                stars += QString( "<img src=\"%1\" height=\"%2\" width=\"%3\">" )
                        .arg( KStandardDirs::locate( "data", "amarok/images/star.png" ) )
                        .arg( QFontMetrics( QToolTip::font() ).height() )
                        .arg( QFontMetrics( QToolTip::font() ).height() );
            if( rating % 2 )
                stars += QString( "<img src=\"%1\" height=\"%2\" width=\"%3\">" )
                        .arg( KStandardDirs::locate( "data", "amarok/images/smallstar.png" ) )
                        .arg( QFontMetrics( QToolTip::font() ).height() )
                        .arg( QFontMetrics( QToolTip::font() ).height() );

            tooltip << QString( "%1: %2" ).arg( i18n( "Rating" ) ).arg( stars );
        }

        const int count = m_track->playCount();
        if( count > 0 )
        {
            tooltip << QString( "%1: %2" ).arg( i18n( "Play Count" ) ).arg( QString::number( count ) );
        }

        const uint lastPlayed = m_track->lastPlayed();
        tooltip << QString( "%1: %2" ).arg( i18n( "Last played" ) ).arg( Amarok::verboseTimeSince( lastPlayed ) );

        setToolTipSubTitle( tooltip.join("<br>") );
    }
    else
    {
        setToolTipIconByName( "amarok" );
        setToolTipTitle( KCmdLineArgs::aboutData()->programName() );
        setToolTipSubTitle( The::engineController()->prettyNowPlaying() );
    }
}

void
Amarok::TrayIcon::slotScrollRequested( int delta, Qt::Orientation orientation )
{
    Q_UNUSED( orientation )

    The::engineController()->increaseVolume( delta / Amarok::VOLUME_SENSITIVITY );
}

void
Amarok::TrayIcon::engineStateChanged( Phonon::State state, Phonon::State /*oldState*/ )
{
    Meta::TrackPtr track = The::engineController()->currentTrack();

    switch( state )
    {
        case Phonon::PlayingState:
            if ( m_track )
            {
                unsubscribeFrom( m_track );
                unsubscribeFrom( m_track->album() );
            }
            m_track = track;
            if ( track )
            {
                subscribeTo( track );
                subscribeTo( track->album() );
            }

            setOverlayIconByName( "media-playback-start" );
            setupMenu();
            break;

        case Phonon::StoppedState:
            m_track = 0;

            setOverlayIconByName( QString() );
            setupMenu(); // remove custom track actions on stop
            break;

        case Phonon::PausedState:
            setOverlayIconByName( "media-playback-pause" );
            break;

        case Phonon::LoadingState:
        case Phonon::ErrorState:
        case Phonon::BufferingState:
            setOverlayIconByName( QString() );
            break;
    }

    setupToolTip( true );
}

void
Amarok::TrayIcon::engineNewTrackPlaying()
{
    m_track = The::engineController()->currentTrack();

    setupToolTip( true );
    setupMenu();
}

void
Amarok::TrayIcon::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )

    setupToolTip( false );
    setupMenu();
}

void
Amarok::TrayIcon::metadataChanged( Meta::AlbumPtr album )
{
    Q_UNUSED( album )

    setupToolTip( true );
    setupMenu();
}

void
Amarok::TrayIcon::engineVolumeChanged( int percent )
{
    Q_UNUSED( percent );

    setupToolTip( false );
}

void
Amarok::TrayIcon::engineMuteStateChanged( bool mute )
{
    Q_UNUSED( mute );

    setupToolTip( false );
}

void
Amarok::TrayIcon::setupMenu()
{
    foreach( QAction* action, m_extraActions )
        contextMenu()->removeAction( action );

    contextMenu()->removeAction( m_separator.data() );

    delete m_separator.data();

    if( !m_track )
        return;

    m_extraActions.clear();
    foreach( QAction *action, The::globalCurrentTrackActions()->actions() )
        m_extraActions.append( action );

    if ( m_track->hasCapabilityInterface( Capabilities::Capability::CurrentTrackActions ) )
    {
        Capabilities::CurrentTrackActionsCapability *cac = m_track->create<Capabilities::CurrentTrackActionsCapability>();
        if( cac )
        {
            QList<QAction *> currentTrackActions = cac->customActions();
            foreach( QAction *action, currentTrackActions )
                m_extraActions.append( action );
        }
        delete cac;
    }

    if ( m_extraActions.count() > 0 )
    {
        // remove the two bottom items, so we can push them to the button again
        contextMenu()->removeAction( actionCollection()->action( "file_quit" ) );
        contextMenu()->removeAction( actionCollection()->action( "minimizeRestore" ) );

        foreach( QAction* action, m_extraActions )
            contextMenu()->addAction( action );

        m_separator = contextMenu()->addSeparator();
        // readd
        contextMenu()->addAction( actionCollection()->action( "minimizeRestore" ) );
        contextMenu()->addAction( actionCollection()->action( "file_quit" ) );
    }
}

void
Amarok::TrayIcon::slotActivated()
{
    The::engineController()->playPause();
}

#include "TrayIcon.moc"
