/****************************************************************************************
 * Copyright (c) 2003 Stanislav Karchebny <berkus@users.sf.net>                         *
 * Copyright (c) 2003 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2004 Enrico Ros <eros.kde@email.it>                                    *
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2009 Kevin Funk <krf@electrostorm.net>                                 *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "Systray.h"

#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "amarokconfig.h"
#include "GlobalCurrentTrackActions.h"
#include "meta/Meta.h"
#include "meta/MetaConstants.h"
#include "meta/MetaUtility.h" // for time formatting
#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"

#include <KAction>
#include <KApplication>
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
#include <QTextDocument> // for Qt::escape()
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
        , EngineObserver( The::engineController() )
        , m_trackLength( 0 )
        , m_separator( 0 )
{
    DEBUG_BLOCK

    PERF_LOG( "Beginning TrayIcon Constructor" );
    KActionCollection* const ac = Amarok::actionCollection();

    //seems to be necessary
    /*QAction *quit = actionCollection()->action( "file_quit" );
    quit->disconnect();
    connect( quit, SIGNAL(activated()), kapp, SLOT(quit()) );*/

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
    if( m_track )
    {
        QString tooltip;

        QFontMetrics fm( QToolTip::font() );
        const int elideWidth = 200;
        tooltip = "<center><b>" + fm.elidedText( Qt::escape(m_track->prettyName()), Qt::ElideRight, elideWidth ) + "</b>";
        if( m_track->artist() ) {
            const QString artist = fm.elidedText( Qt::escape(m_track->artist()->prettyName()), Qt::ElideRight, elideWidth );
            if( !artist.isEmpty() )
                tooltip += i18n( " by <b>%1</b>", artist );
        }
        if( m_track->album() ) {
            const QString album = fm.elidedText( Qt::escape(m_track->album()->prettyName()), Qt::ElideRight, elideWidth );
            if( !album.isEmpty() )
                tooltip += i18n( " on <b>%1</b>", album );
        }
        tooltip += "</center>";

        tooltip += "<table cellspacing='2' align='center' width='100%'>";

        // HACK: This block is inefficient and more or less stupid
        // (Unnecessary I/O on disk. Workaround?)
        #ifndef Q_WS_WIN
            const QString tmpFilename = Amarok::saveLocation() + "tooltipcover.png";
            if( m_track->album() )
            {
                const QPixmap image = m_track->album()->imageWithBorder( 100, 5 );
                image.save( tmpFilename, "PNG" );
                tooltip += "<tr><td width='10' align='left' valign='bottom' rowspan='9'>";
                tooltip += "<img src='"+tmpFilename+"' /></td></tr>";
            }
        #endif

        QStringList left, right;

        QString volume;
        if ( The::engineController()->isMuted() )
            volume = i18n( "Muted" );
        else
            volume = QString( "%1%" ).arg( The::engineController()->volume() );
        right << QString("<i>%1</i>").arg( volume );
        left << "<i>Volume</i>";

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

        if( m_trackLength > 0 )
        {
            right << Meta::secToPrettyTime( m_trackLength );
            left << i18n( "Length" );
        }

        // NOTE: It seems to be necessary to <center> each element indivdually
        const QString tableRow = "<tr><td align='right'>%1: </td><td align='left'>%2</td></tr>";
        for( int x = 0; x < left.count(); ++x )
            if ( !right[x].isEmpty() )
                tooltip += tableRow.arg( left[x] ).arg( right[x] );

        tooltip += "</table>";

        #ifdef Q_WS_WIN
            tooltip.replace( "<tr>", "\n" );
            QRegExp rx( "(<[^>]+>)" );
            tooltip.replace( rx, "" );
        #endif
        setToolTip( tooltip );
    }
    else
    {
        setToolTip( i18n( "Amarok - No track playing" ) );
    }
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

    case QEvent::Drop:
        #define e static_cast<QDropEvent*>(e)
        {
            const KUrl::List list = KUrl::List::fromMimeData( e->mimeData() );
            if( !list.isEmpty() )
            {
                KMenu *popup = new KMenu;
                popup->addAction( KIcon( "media-track-add-amarok" ), i18n( "&Append to Playlist" ), this, SLOT( appendDrops() ) );
                popup->addAction( KIcon( "media-track-add-amarok" ), i18n( "Append && &Play" ), this, SLOT( appendAndPlayDrops() ) );
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

    default:
        return KSystemTrayIcon::event( e );
    }
    return true;
}

void
Amarok::TrayIcon::engineStateChanged( Phonon::State state, Phonon::State /*oldState*/ )
{
    switch( state )
    {
        case Phonon::PlayingState:
            m_track = The::engineController()->currentTrack();
            m_trackLength = m_track ? m_track->length() : 0;

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
    setupToolTip();
    setupMenu();
}

void
Amarok::TrayIcon::engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged )
{
    Q_UNUSED( trackChanged )
    Q_UNUSED( &newMetaData );

    setupToolTip();
    setupMenu();
}

void
Amarok::TrayIcon::engineTrackPositionChanged( long position, bool userSeek )
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
Amarok::TrayIcon::paintIcon( long trackPosition )
{
    static int oldMergePos = -1;

    // start up
    if( m_baseIcon.isNull() )
    {
        QIcon icon = KSystemTrayIcon::loadIcon( "amarok" );
        m_baseIcon = icon.pixmap( geometry().size() );
        setIcon( icon ); // show icon
        return; // return because m_baseIcon is still null after first startup
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

    const int mergePos = ( ( float( trackPosition ) / 1000 ) / m_trackLength ) * geometry().height();

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

    if ( m_track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) )
    {
        Meta::CurrentTrackActionsCapability *cac = m_track->create<Meta::CurrentTrackActionsCapability>();
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

#include "Systray.moc"
