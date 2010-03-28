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

#include "TrayIconLegacy.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "amarokconfig.h"
#include "GlobalCurrentTrackActions.h"
#include "core/meta/support/MetaConstants.h"
#include "core/capabilities/CurrentTrackActionsCapability.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"
#include "SvgHandler.h"
#include <KAction>
#include <KIcon>
#include <KIconEffect>
#include <KLocale>
#include <KMenu>
#include <KStandardDirs>

#include <QAction>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QTime>
#include <QToolTip>

namespace Amarok
{
    static QPixmap
    loadOverlay( const QString &iconName )
    {
        KIcon icon( iconName );
        if ( !icon.isNull() )
            return icon.pixmap( 10, 10 ); // overlay size, adjust here

        return 0;
    }
}

Amarok::TrayIcon::TrayIcon( QWidget *playerWidget )
        : KSystemTrayIcon( playerWidget )
        , Engine::EngineObserver( The::engineController() )
        , m_trackLength( 0 )
        , m_separator( 0 )
{
    DEBUG_BLOCK

    PERF_LOG( "Beginning TrayIcon Constructor" );
    KActionCollection* const ac = Amarok::actionCollection();

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

    m_playOverlay = loadOverlay( "media-playback-start" );
    m_pauseOverlay = loadOverlay( "media-playback-pause" );

    PERF_LOG( "Adding system tray icon" );
    paintIcon();

    setupToolTip();

    connect( this, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ), SLOT( slotActivated( QSystemTrayIcon::ActivationReason ) ) );
    #ifdef Q_WS_MAC
    KSystemTrayIcon::setVisible( false );
    #endif
    show();
}

void
Amarok::TrayIcon::setVisible( bool visible )
{
    #ifdef Q_WS_MAC
    Q_UNUSED( visible )
    #else
    KSystemTrayIcon::setVisible( visible );
    #endif
}

void
Amarok::TrayIcon::setupToolTip()
{
    QString tooltip;

    tooltip = "<center>";
    tooltip += The::engineController()->prettyNowPlaying();
    tooltip += "</center>";

    if( m_track )
    {
        tooltip += "<table cellspacing='2' align='center' width='100%'>";

        // HACK: This block is inefficient and more or less stupid
        // (Unnecessary I/O on disk. Workaround?)
        // TODO: Use Observer to get notified about changed album art
        const QString tmpFilename = Amarok::saveLocation() + "tooltipcover.png";
        if( m_track->album() )
        {
            const QPixmap image = The::svgHandler()->imageWithBorder( m_track->album(), 100, 5 );
            image.save( tmpFilename, "PNG" );
            tooltip += "<tr><td width='10' align='left' valign='bottom' rowspan='9'>";
            tooltip += "<img src='"+tmpFilename+"' /></td></tr>";
        }

        QStringList left, right;

        // TODO: Replace block by some other useful information
        QString volume;
        if ( The::engineController()->isMuted() )
            volume = i18n( "Muted" );
        else
            volume = QString( "%1%" ).arg( The::engineController()->volume() );
        right << QString("<i>%1</i>").arg( volume );
        left << QString( "<i>%1</i>" ).arg( i18n( "Volume" ) );

        const float score = m_track->score();
        if( score > 0.f )
        {
            right << QString::number( score, 'f', 2 );  // 2 digits after decimal point
            left << i18n( "Score" );
        }

        const int rating = m_track->rating();
        if( rating > 0 )
        {
            QString s;
            for( int i = 0; i < rating / 2; ++i )
                s += QString( "<img src=\"%1\" height=\"%2\" width=\"%3\">" )
                        .arg( KStandardDirs::locate( "data", "amarok/images/star.png" ) )
                        .arg( QFontMetrics( QToolTip::font() ).height() )
                        .arg( QFontMetrics( QToolTip::font() ).height() );
            if( rating % 2 )
                s += QString( "<img src=\"%1\" height=\"%2\" width=\"%3\">" )
                        .arg( KStandardDirs::locate( "data", "amarok/images/smallstar.png" ) )
                        .arg( QFontMetrics( QToolTip::font() ).height() )
                        .arg( QFontMetrics( QToolTip::font() ).height() );
            right << s;
            left << i18n( "Rating" );
        }

        const int count = m_track->playCount();
        if( count > 0 )
        {
            right << QString::number( count );
            left << i18n( "Play Count" );
        }

        const uint lastPlayed = m_track->lastPlayed();
        right << Amarok::verboseTimeSince( lastPlayed );
        left << i18n( "Last Played" );

        // NOTE: It seems to be necessary to <center> each element indivdually
        const QString tableRow = "<tr><td align='right'>%1: </td><td align='left'>%2</td></tr>";
        for( int x = 0; x < left.count(); ++x )
            if ( !right[x].isEmpty() )
                tooltip += tableRow.arg( left[x] ).arg( right[x] );

        tooltip += "</table>";
    }

    setToolTip( tooltip );
}

bool
Amarok::TrayIcon::event( QEvent *e )
{
    static QTime lastEventCall = QTime();

    switch( e->type() )
    {
    case QEvent::DragEnter:
        #define e static_cast<QDragEnterEvent*>(e)
        e->setAccepted( KUrl::List::canDecode( e->mimeData() ) );
        break;
        #undef e

    case QEvent::Drop:
        #define e static_cast<QDropEvent*>(e)
        {
            const KUrl::List list = KUrl::List::fromMimeData( e->mimeData() );
            if( !list.isEmpty() )
            {
                KMenu *popup = new KMenu;
                popup->addAction( KIcon( "media-track-add-amarok" ), i18n( "&Add to Playlist" ), this, SLOT( appendDrops() ) );
                popup->addAction( KIcon( "media-track-add-amarok" ), i18n( "Add && &Play" ), this, SLOT( appendAndPlayDrops() ) );
                if( The::playlist()->activeRow() >= 0 )
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
        if( e->modifiers() == Qt::ControlModifier || e->orientation() == Qt::Horizontal )
        {
            if (lastEventCall.elapsed() < 500) // block event for some ms
                break;

            lastEventCall.restart();

            if( e->delta() > 0 ) // up
                The::playlistActions()->back();
            else
                The::playlistActions()->next();
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

    default:
        return KSystemTrayIcon::event( e );
    }
    return true;
}

void
Amarok::TrayIcon::engineStateChanged( Phonon::State state, Phonon::State /*oldState*/ )
{
    Meta::TrackPtr track = The::engineController()->currentTrack();

    switch( state )
    {
        case Phonon::PlayingState:
            unsubscribeFrom( m_track );
            m_track = track;
            m_trackLength = m_track ? m_track->length() : 0;
            subscribeTo( track );

            paintIcon( 0 );
            setupMenu();
            break;

        case Phonon::StoppedState:
            m_track = 0;
            m_trackLength = 0;

            paintIcon();
            setupMenu(); // remove custom track actions on stop
            break;

        case Phonon::PausedState:
            blendOverlay( m_pauseOverlay );
            break;

        case Phonon::LoadingState:
        case Phonon::ErrorState:
        case Phonon::BufferingState:
            break;
    }

    setupToolTip();
}

void
Amarok::TrayIcon::engineNewTrackPlaying()
{
    m_track = The::engineController()->currentTrack();
    m_trackLength = m_track ? m_track->length() : 0;

    paintIcon( 0 );

    setupToolTip();
    setupMenu();
}

void
Amarok::TrayIcon::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )

    setupToolTip();
    setupMenu();
}

void
Amarok::TrayIcon::engineTrackPositionChanged( qint64 position, bool userSeek )
{
    Q_UNUSED( userSeek );

    if( m_trackLength )
        paintIcon( position );
}

void
Amarok::TrayIcon::engineVolumeChanged( int percent )
{
    Q_UNUSED( percent );

    setupToolTip();
}

void
Amarok::TrayIcon::engineMuteStateChanged( bool mute )
{
    Q_UNUSED( mute );

    setupToolTip();
}

void
Amarok::TrayIcon::paletteChange( const QPalette & op )
{
    Q_UNUSED( op );

    paintIcon();
}

void
Amarok::TrayIcon::paintIcon( qint64 trackPosition )
{
    static qint64 oldMergePos = -1;

    // start up
    if( m_baseIcon.isNull() )
    {
        QIcon icon = KSystemTrayIcon::loadIcon( "amarok" );
        m_baseIcon = icon.pixmap( geometry().size() );
        setIcon( icon ); // show icon
        return; // HACK: return because m_baseIcon is still null after first startup (why?)
    }

    if( m_grayedIcon.isNull() )
    {
        m_grayedIcon = m_baseIcon; // copies object
        KIconEffect::semiTransparent( m_grayedIcon );
    }

    // trackPosition < 0 means reset
    if( trackPosition < 0 )
    {
        oldMergePos = -1;
        setIcon( m_baseIcon );
        return;
    }

    // check if we are playing a stream
    if( !m_trackLength )
    {
        m_icon = m_baseIcon;
        blendOverlay( m_playOverlay );
        return;
    }

    const qint64 mergePos = ( float( trackPosition ) / m_trackLength ) * geometry().height();

    // return if pixmap would stay the same
    if( oldMergePos == mergePos )
        return;

    // draw m_baseIcon on top of the gray version
    m_icon = m_grayedIcon; // copies object
    QPainter p( &m_icon );
    p.drawPixmap( 0, 0, m_baseIcon, 0, 0, 0, geometry().height() - mergePos );
    p.end();

    oldMergePos = mergePos;

    blendOverlay( m_playOverlay );
}

void
Amarok::TrayIcon::blendOverlay( const QPixmap &overlay )
{
    if ( !overlay.isNull() )
    {
        // draw overlay at bottom right
        const int x = geometry().size().width() - overlay.size().width();
        const int y = geometry().size().height() - overlay.size().width();
        QPainter p( &m_icon );
        p.drawPixmap( x, y, overlay );
        p.end();
        setIcon( m_icon );
    }
}

void
Amarok::TrayIcon::setupMenu()
{
    foreach( QAction* action, m_extraActions )
        contextMenu()->removeAction( action );

    contextMenu()->removeAction( m_separator );

    delete m_separator;

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
Amarok::TrayIcon::slotActivated( QSystemTrayIcon::ActivationReason reason )
{
    if( reason == QSystemTrayIcon::MiddleClick )
        The::engineController()->playPause();
}

#include "TrayIconLegacy.moc"
