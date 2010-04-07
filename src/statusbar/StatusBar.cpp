/****************************************************************************************
 * Copyright (c) 2005 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "statusbar/StatusBar.h"

#include "core/support/Debug.h"
#include "EngineController.h"
#include "LongMessageWidget.h"
#include "core/meta/support/MetaUtility.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/interfaces/Logger.h"
#include "core/support/Components.h"
#include "core-impl/logger/ProxyLogger.h"
#include "playlist/PlaylistItem.h"
#include "playlist/PlaylistModelStack.h"

#include "KJobProgressBar.h"

#include <QVariant>

#include <cmath>


class LoggerAdaptor : public Amarok::Logger
{
public:
    LoggerAdaptor( StatusBar *bar )
        : m_statusBar( bar )
    {
        setParent( bar );
    }

    virtual void shortMessage( const QString &text )
    {
        m_statusBar->shortMessage( text );
    }

    virtual void longMessage( const QString &text, MessageType type )
    {
        StatusBar::MessageType otherType;
        switch( type )
        {
        case Amarok::Logger::Information:
            otherType = StatusBar::Information;
            break;
        case Amarok::Logger::Warning:
            otherType = StatusBar::Warning;
            break;
        case Amarok::Logger::Error:
            otherType = StatusBar::Error;
            break;
        }
        m_statusBar->longMessage( text, otherType );
    }

    virtual void newProgressOperation( KJob *job, const QString &text, QObject *obj, const char *slot, Qt::ConnectionType type )
    {
        ProgressBar *bar = m_statusBar->newProgressOperation( job, text );
        if( obj )
        {
            bar->setAbortSlot( obj, slot, type );
        }
    }

private:
    StatusBar *m_statusBar;
};

StatusBar* StatusBar::s_instance = 0;

namespace The
{
    StatusBar* statusBar()
    {
        return StatusBar::instance();
    }
}

StatusBar::StatusBar( QWidget * parent )
        : KStatusBar( parent )
        , Engine::EngineObserver( The::engineController() )
        , m_progressBar( new CompoundProgressBar( this ) )
        , m_busy( false )
        , m_shortMessageTimer( new QTimer( this ) )
{
    s_instance = this;

    addWidget( m_progressBar, 1 );
    m_progressBar->hide();

    connect( m_progressBar, SIGNAL( allDone() ), this, SLOT( hideProgress() ) );

    //setMaximumSize( The::mainWindow()->width(), m_progressBar->height() );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setContentsMargins( 0, 0, 0, 0 );
    setSizeGripEnabled( false );

    m_shortMessageTimer->setSingleShot( true );
    connect( m_shortMessageTimer, SIGNAL( timeout() ), this, SLOT( nextShortMessage() ) );

    m_nowPlayingWidget = new KHBox( 0 );
    m_nowPlayingWidget->setSpacing( 4 );

    m_nowPlayingLabel = new KSqueezedTextLabel( m_nowPlayingWidget );
    m_nowPlayingLabel->setTextElideMode( Qt::ElideRight );
    m_nowPlayingLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );

    m_nowPlayingEmblem = new QLabel( m_nowPlayingWidget );
    m_nowPlayingEmblem->setFixedSize( 16, 16 );

    m_separator = new QFrame( m_nowPlayingWidget );
    m_separator->setFrameShape( QFrame::VLine );

    m_playlistLengthLabel = new QLabel( m_nowPlayingWidget);
    m_playlistLengthLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );

    QWidget * spacer = new QWidget( m_nowPlayingWidget );
    spacer->setFixedWidth( 3 );

    addPermanentWidget( m_nowPlayingWidget );

    qRegisterMetaType<MessageType>( "MessageType" );
    connect( this, SIGNAL( signalLongMessage( const QString &, MessageType ) ), SLOT( slotLongMessage( const QString &, MessageType ) ), Qt::QueuedConnection );

    connect( The::playlist()->qaim(), SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( updateTotalPlaylistLength() ) );
    // Ignore The::playlist() layoutChanged: rows moving around does not change the total playlist length.
    connect( The::playlist()->qaim(), SIGNAL( modelReset() ), this, SLOT( updateTotalPlaylistLength() ) );
    connect( The::playlist()->qaim(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( updateTotalPlaylistLength() ) );
    connect( The::playlist()->qaim(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( updateTotalPlaylistLength() ) );

    connect( The::playlist()->qaim(), SIGNAL( queueChanged() ), this, SLOT( updateTotalPlaylistLength() ) );

    updateTotalPlaylistLength();

    Amarok::Logger *logger = Amarok::Components::logger();
    ProxyLogger *proxy = qobject_cast<ProxyLogger*>( logger );
    if( proxy )
    {
        proxy->setLogger( new LoggerAdaptor( this ) );
    }
    else
    {
        warning() << "Was not able to register statusbar as logger";
    }
}


StatusBar::~StatusBar()
{
    DEBUG_BLOCK

    delete m_progressBar;
    m_progressBar = 0;

    s_instance = 0;
}

ProgressBar * StatusBar::newProgressOperation( QObject * owner, const QString & description )
{
    //clear any short message currently being displayed and stop timer if running...
    clearMessage();
    m_shortMessageTimer->stop();

    //also hide the now playing stuff:
    m_nowPlayingWidget->hide();
    ProgressBar * newBar = new ProgressBar( 0 );
    newBar->setDescription( description );
    m_progressBar->addProgressBar( newBar, owner );
    m_progressBar->show();
    m_busy = true;

    return newBar;
}

ProgressBar * StatusBar::newProgressOperation( KJob * job, const QString & description )
{
    //clear any short message currently being displayed and stop timer if running...
    clearMessage();
    m_shortMessageTimer->stop();

    //also hide the now playing stuff:
    m_nowPlayingWidget->hide();
    KJobProgressBar * newBar = new KJobProgressBar( 0, job );
    newBar->setDescription( description );
    m_progressBar->addProgressBar( newBar, job );
    m_progressBar->show();
    m_busy = true;

    return newBar;
}

void StatusBar::shortMessage( const QString & text )
{
    if ( !m_busy )
    {
        //not busy, so show right away
        showMessage( text );
        m_shortMessageTimer->start( SHORT_MESSAGE_DURATION );
    }
    else
    {
        m_shortMessageQue.append( text );
    }
}

void StatusBar::hideProgress()
{
    DEBUG_BLOCK

    m_progressBar->hide();
    m_busy = false;

    nextShortMessage();
}

void StatusBar::nextShortMessage()
{
    if ( m_shortMessageQue.count() > 0 )
    {
        m_busy = true;
        showMessage( m_shortMessageQue.takeFirst() );
        m_shortMessageTimer->start( SHORT_MESSAGE_DURATION );
    }
    else
    {
        clearMessage();
        m_busy = false;
        m_nowPlayingWidget->show();
    }
}

void StatusBar::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track );

    if ( m_currentTrack )
        updateInfo( m_currentTrack );
    else
        engineNewTrackPlaying();
}

void StatusBar::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    Q_UNUSED( oldState )

    switch ( state )
    {
    case Phonon::StoppedState:
        m_nowPlayingLabel->setText( QString() );
        m_nowPlayingEmblem->hide();
        break;

    case Phonon::LoadingState:
        if ( m_currentTrack )
            updateInfo( m_currentTrack );
        else
            m_nowPlayingLabel->setText( QString() );
        m_nowPlayingEmblem->hide();
        break;

    case Phonon::PausedState:
        m_nowPlayingLabel->setText( i18n( "Amarok is paused" ) ); // display TEMPORARY message
        m_nowPlayingEmblem->hide();
        break;

    case Phonon::PlayingState:
        if ( m_currentTrack )
            updateInfo( m_currentTrack );
        //else
        //resetMainText(); // if we were paused, this is necessary
        break;

    case Phonon::ErrorState:
    case Phonon::BufferingState:
        break;
    }
}

void StatusBar::engineNewTrackPlaying()
{
    if ( m_currentTrack )
        unsubscribeFrom( m_currentTrack );

    m_currentTrack = The::engineController()->currentTrack();

    if ( !m_currentTrack )
    {
        m_currentTrack = Meta::TrackPtr();
        return;
    }
    subscribeTo( m_currentTrack );
    updateInfo( m_currentTrack );
}

void StatusBar::updateInfo( Meta::TrackPtr track )
{
    // Check if we have any source info:
    Capabilities::SourceInfoCapability *sic = track->create<Capabilities::SourceInfoCapability>();
    if ( sic )
    {
        if ( !sic->sourceName().isEmpty() )
        {
            m_nowPlayingEmblem->setPixmap( sic->emblem() );
            m_nowPlayingEmblem->show();
        }
        else
            m_nowPlayingEmblem->hide();
        delete sic;
    }
    else
        m_nowPlayingEmblem->hide();

    m_nowPlayingLabel->setText( i18n( "Playing: %1", The::engineController()->prettyNowPlaying() ) );
}

void StatusBar::longMessage( const QString & text, MessageType type )
{
    DEBUG_BLOCK

    // The purpose of this emit is to make the operation thread safe. If this
    // method is called from a non-GUI thread, the "emit" relays it over the
    // event loop to the GUI thread, so that we can safely create widgets.

    emit signalLongMessage( text, type );
}

void StatusBar::slotLongMessage( const QString & text, MessageType type ) //SLOT
{
    DEBUG_BLOCK

    LongMessageWidget * message = new LongMessageWidget( this, text, type );
    connect( message, SIGNAL( closed() ), this, SLOT( hideLongMessage() ) );
}

void StatusBar::hideLongMessage()
{
    sender()->deleteLater();
}

void
StatusBar::updateTotalPlaylistLength() //SLOT
{
    const quint64 totalLength = The::playlist()->totalLength();
    const quint64 totalSize = The::playlist()->totalSize();
    const int trackCount = The::playlist()->qaim()->rowCount();
    const QString prettyTotalLength = Meta::msToPrettyTime( totalLength );
    const QString prettyTotalSize = Meta::prettyFilesize( totalSize );

    if( totalLength > 0 && trackCount > 0 )
    {
        m_playlistLengthLabel->setText( i18ncp( "%1 is number of tracks, %2 is time", "%1 track (%2)", "%1 tracks (%2)", trackCount, prettyTotalLength ) );
        m_playlistLengthLabel->show();

        quint64 queuedTotalLength( 0 );
        quint64 queuedTotalSize( 0 );
        int queuedCount( 0 );

        for( int i = 0; i < trackCount; ++i )
        {
            if( The::playlist()->stateOfRow( i ) & Playlist::Item::Queued )
            {
                queuedTotalLength += The::playlist()->trackAt( i )->length();
                queuedTotalSize += The::playlist()->trackAt( i )->filesize();
                queuedCount++;
            }
        }

        const QString prettyQueuedTotalLength = Meta::msToPrettyTime( queuedTotalLength );
        const QString prettyQueuedTotalSize   = Meta::prettyFilesize( queuedTotalSize );

        QString tooltipLabel;
        if( queuedCount > 0 && queuedTotalLength > 0 )
        {
            tooltipLabel = i18n( "Total playlist size: %1", prettyTotalSize )       + '\n'
                         + i18n( "Queue size: %1",          prettyQueuedTotalSize ) + '\n'
                         + i18n( "Queue length: %1",        prettyQueuedTotalLength );
        }
        else
        {
            tooltipLabel = i18n( "Total playlist size: %1", prettyTotalSize );
        }

        m_playlistLengthLabel->setToolTip( tooltipLabel );
        m_separator->show();
    }
    else if( ( totalLength == 0 ) && ( trackCount > 0 ) )
    {
        m_playlistLengthLabel->setText( i18ncp( "%1 is number of tracks", "%1 track", "%1 tracks", trackCount ) );
        m_playlistLengthLabel->show();
        m_playlistLengthLabel->setToolTip( 0 );
        m_separator->show();
    }
    //Total Length will not be > 0 if trackCount is 0, so we can ignore it
    else { // TotalLength = 0 and trackCount = 0;
        m_playlistLengthLabel->hide();
        m_separator->hide();
    }
}

#include "StatusBar.moc"

