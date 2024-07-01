/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2007-2009 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::Actions"

#include "PlaylistActions.h"

#include "EngineController.h"
#include "MainWindow.h"
#include "amarokconfig.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core/logger/Logger.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "navigators/DynamicTrackNavigator.h"
#include "navigators/RandomAlbumNavigator.h"
#include "navigators/RandomTrackNavigator.h"
#include "navigators/RepeatAlbumNavigator.h"
#include "navigators/RepeatTrackNavigator.h"
#include "navigators/StandardTrackNavigator.h"
#include "navigators/FavoredRandomTrackNavigator.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistDock.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistRestorer.h"
#include "playlistmanager/PlaylistManager.h"

#include <QRandomGenerator>
#include <typeinfo>

Playlist::Actions* Playlist::Actions::s_instance = nullptr;

Playlist::Actions* Playlist::Actions::instance()
{
    if( !s_instance )
    {
        s_instance = new Actions();
        s_instance->init(); // prevent infinite recursion by using the playlist actions only after setting the instance.
    }
    return s_instance;
}

void
Playlist::Actions::destroy()
{
    delete s_instance;
    s_instance = nullptr;
}

Playlist::Actions::Actions()
        : QObject()
        , m_nextTrackCandidate( 0 )
        , m_stopAfterPlayingTrackId( 0 )
        , m_navigator( nullptr )
        , m_waitingForNextTrack( false )
{
    EngineController *engine = The::engineController();

    if( engine ) // test cases might create a playlist without having an EngineController
    {
        connect( engine, &EngineController::trackPlaying,
                 this, &Playlist::Actions::slotTrackPlaying );
        connect( engine, &EngineController::stopped,
                 this, &Playlist::Actions::slotPlayingStopped );
    }
}

Playlist::Actions::~Actions()
{
    delete m_navigator;
}

void
Playlist::Actions::init()
{
    playlistModeChanged(); // sets m_navigator.
    restoreDefaultPlaylist();
}

Meta::TrackPtr
Playlist::Actions::likelyNextTrack()
{
    return The::playlist()->trackForId( m_navigator->likelyNextTrack() );
}

Meta::TrackPtr
Playlist::Actions::likelyPrevTrack()
{
    return The::playlist()->trackForId( m_navigator->likelyLastTrack() );
}

void
Playlist::Actions::requestNextTrack()
{
    DEBUG_BLOCK
    if ( m_nextTrackCandidate != 0 )
        return;

    m_nextTrackCandidate = m_navigator->requestNextTrack();
    if( m_nextTrackCandidate == 0 )
        return;

    if( willStopAfterTrack( ModelStack::instance()->bottom()->activeId() ) )
        // Tell playlist what track to play after users hits Play again:
        The::playlist()->setActiveId( m_nextTrackCandidate );
    else
        play( m_nextTrackCandidate, false );
}

void
Playlist::Actions::requestUserNextTrack()
{
    m_nextTrackCandidate = m_navigator->requestUserNextTrack();
    play( m_nextTrackCandidate );
}

void
Playlist::Actions::requestPrevTrack()
{
    m_nextTrackCandidate = m_navigator->requestLastTrack();
    play( m_nextTrackCandidate );
}

void
Playlist::Actions::requestTrack( quint64 id )
{
    m_nextTrackCandidate = id;
}

void
Playlist::Actions::stopAfterPlayingTrack( quint64 id )
{
    if( id == quint64( -1 ) )
        id = ModelStack::instance()->bottom()->activeId(); // 0 is fine
    if( id != m_stopAfterPlayingTrackId )
    {
        m_stopAfterPlayingTrackId = id;
        repaintPlaylist(); // to get the visual change
    }
}

bool
Playlist::Actions::willStopAfterTrack( const quint64 id ) const
{
    return m_stopAfterPlayingTrackId && m_stopAfterPlayingTrackId == id;
}

void
Playlist::Actions::play()
{
    DEBUG_BLOCK

    if ( m_nextTrackCandidate == 0 )
    {
        m_nextTrackCandidate = The::playlist()->activeId();
        // the queue has priority, and requestNextTrack() respects the queue.
        // this is a bit of a hack because we "know" that all navigators will look at the queue first.
        if ( !m_nextTrackCandidate || !m_navigator->queue().isEmpty() )
            m_nextTrackCandidate = m_navigator->requestNextTrack();
    }

    play( m_nextTrackCandidate );
}

void
Playlist::Actions::play( const QModelIndex &index )
{
    DEBUG_BLOCK

    if( index.isValid() )
    {
        m_nextTrackCandidate = index.data( UniqueIdRole ).value<quint64>();
        play( m_nextTrackCandidate );
    }
}

void
Playlist::Actions::play( const int row )
{
    DEBUG_BLOCK

    m_nextTrackCandidate = The::playlist()->idAt( row );
    play( m_nextTrackCandidate );
}

void
Playlist::Actions::play( const quint64 trackid, bool now )
{
    DEBUG_BLOCK

    Meta::TrackPtr track = The::playlist()->trackForId( trackid );
    if ( track )
    {
        if ( now )
            The::engineController()->play( track );
        else
            The::engineController()->setNextTrack( track );
    }
    else
    {
        warning() << "Invalid trackid" << trackid;
    }
}

void
Playlist::Actions::next()
{
    DEBUG_BLOCK
    requestUserNextTrack();
}

void
Playlist::Actions::back()
{
    DEBUG_BLOCK
    requestPrevTrack();
}


void
Playlist::Actions::enableDynamicMode( bool enable )
{
    if( AmarokConfig::dynamicMode() == enable )
        return;

    AmarokConfig::setDynamicMode( enable );
    // TODO: turn off other incompatible modes
    // TODO: should we restore the state of other modes?
    AmarokConfig::self()->save();

    Playlist::Dock *dock = The::mainWindow()->playlistDock();
    Playlist::SortWidget *sorting = dock ? dock->sortWidget() : nullptr;
    if( sorting )
        sorting->trimToLevel();

    playlistModeChanged();

    /* append upcoming tracks to satisfy user's with about number of upcoming tracks.
     * Needs to be _after_ playlistModeChanged() because before calling it the old
     * m_navigator still reigns. */
    if( enable )
        normalizeDynamicPlaylist();
}


void
Playlist::Actions::playlistModeChanged()
{
    DEBUG_BLOCK

    QQueue<quint64> currentQueue;

    if ( m_navigator )
    {
        //HACK: Migrate the queue to the new navigator
        //TODO: The queue really should not be maintained by the navigators in this way
        // but should be handled by a separate and persistent object.

        currentQueue = m_navigator->queue();
        m_navigator->deleteLater();
    }

    debug() << "Dynamic mode:   " << AmarokConfig::dynamicMode();

    if ( AmarokConfig::dynamicMode() )
    {
        m_navigator = new DynamicTrackNavigator();
        Q_EMIT navigatorChanged();
        return;
    }

    m_navigator = nullptr;

    switch( AmarokConfig::trackProgression() )
    {

        case AmarokConfig::EnumTrackProgression::RepeatTrack:
            m_navigator = new RepeatTrackNavigator();
            break;

        case AmarokConfig::EnumTrackProgression::RepeatAlbum:
            m_navigator = new RepeatAlbumNavigator();
            break;

        case AmarokConfig::EnumTrackProgression::RandomTrack:
            switch( AmarokConfig::favorTracks() )
            {
                case AmarokConfig::EnumFavorTracks::HigherScores:
                case AmarokConfig::EnumFavorTracks::HigherRatings:
                case AmarokConfig::EnumFavorTracks::LessRecentlyPlayed:
                    m_navigator = new FavoredRandomTrackNavigator();
                    break;

                case AmarokConfig::EnumFavorTracks::Off:
                default:
                    m_navigator = new RandomTrackNavigator();
                    break;
            }
            break;

        case AmarokConfig::EnumTrackProgression::RandomAlbum:
            m_navigator = new RandomAlbumNavigator();
            break;

        //repeat playlist, standard, only queue and fallback are all the normal navigator.
        case AmarokConfig::EnumTrackProgression::RepeatPlaylist:
        case AmarokConfig::EnumTrackProgression::OnlyQueue:
        case AmarokConfig::EnumTrackProgression::Normal:
        default:
            m_navigator = new StandardTrackNavigator();
            break;
    }

    m_navigator->queueIds( currentQueue );

    Q_EMIT navigatorChanged();
}

void
Playlist::Actions::repopulateDynamicPlaylist()
{
    DEBUG_BLOCK

    if ( typeid( *m_navigator ) == typeid( DynamicTrackNavigator ) )
    {
        static_cast<DynamicTrackNavigator*>(m_navigator)->repopulate();
    }
}

void
Playlist::Actions::shuffle()
{
    QList<int> fromRows, toRows;

    {
        const int rowCount = The::playlist()->qaim()->rowCount();
        fromRows.reserve( rowCount );

        QMultiMap<int, int> shuffleToRows;
        for( int row = 0; row < rowCount; ++row )
        {
            fromRows.append( row );
            shuffleToRows.insert( QRandomGenerator::global()->generate(), row );
        }
        toRows = shuffleToRows.values();
    }

    The::playlistController()->reorderRows( fromRows, toRows );
}

int
Playlist::Actions::queuePosition( quint64 id )
{
    return m_navigator->queuePosition( id );
}

QQueue<quint64>
Playlist::Actions::queue()
{
    return m_navigator->queue();
}

bool
Playlist::Actions::queueMoveUp( quint64 id )
{
    const bool ret = m_navigator->queueMoveUp( id );
    if ( ret )
        Playlist::ModelStack::instance()->bottom()->emitQueueChanged();
    return ret;
}

bool
Playlist::Actions::queueMoveDown( quint64 id )
{
    const bool ret = m_navigator->queueMoveDown( id );
    if ( ret )
        Playlist::ModelStack::instance()->bottom()->emitQueueChanged();
    return ret;
}

bool
Playlist::Actions::queueMoveTo( quint64 id, const int pos )
{
    const bool ret = m_navigator->queueMoveTo( id, pos );
    if ( ret )
        Playlist::ModelStack::instance()->bottom()->emitQueueChanged();
    return ret;
}

void
Playlist::Actions::dequeue( quint64 id )
{
    m_navigator->dequeueId( id ); // has no return value, *shrug*
    Playlist::ModelStack::instance()->bottom()->emitQueueChanged();
    return;
}

void
Playlist::Actions::queue( const QList<int> &rows )
{
    QList<quint64> ids;
    for( int row : rows )
        ids << The::playlist()->idAt( row );
    queue( ids );
}

void
Playlist::Actions::queue( const QList<quint64> &ids )
{
    m_navigator->queueIds( ids );
    if ( !ids.isEmpty() )
        Playlist::ModelStack::instance()->bottom()->emitQueueChanged();
}

void
Playlist::Actions::dequeue( const QList<int> &rows )
{
    DEBUG_BLOCK

    for( int row : rows )
    {
        quint64 id = The::playlist()->idAt( row );
        m_navigator->dequeueId( id );
    }
    if ( !rows.isEmpty() )
        Playlist::ModelStack::instance()->bottom()->emitQueueChanged();
}

void
Playlist::Actions::slotTrackPlaying( Meta::TrackPtr engineTrack )
{
    DEBUG_BLOCK

    if ( engineTrack )
    {
        Meta::TrackPtr candidateTrack = The::playlist()->trackForId( m_nextTrackCandidate );    // May be 0.
        if ( engineTrack == candidateTrack )
        {   // The engine is playing what we planned: everything is OK.
            The::playlist()->setActiveId( m_nextTrackCandidate );
        }
        else
        {
            warning() << "engineNewTrackPlaying:" << engineTrack->prettyName() << "does not match what the playlist controller thought it should be";
            if ( The::playlist()->activeTrack() != engineTrack )
            {
                 // this will set active row to -1 if the track isn't in the playlist at all
                int row = The::playlist()->firstRowForTrack( engineTrack );
                if( row != -1 )
                    The::playlist()->setActiveRow( row );
                else
                    The::playlist()->setActiveRow( AmarokConfig::lastPlaying() );
            }
            //else
            //  Engine and playlist are in sync even though we didn't plan it; do nothing
        }
    }
    else
        warning() << "engineNewTrackPlaying: not really a track";

    m_nextTrackCandidate = 0;
}

void
Playlist::Actions::slotPlayingStopped( qint64 finalPosition, qint64 trackLength )
{
    DEBUG_BLOCK;

    stopAfterPlayingTrack( 0 ); // reset possible "Stop after playing track";

    // we have to determine if we reached the end of the playlist.
    // in such a case there would be no new track and the current one
    // played until the end.
    // else this must be a result of StopAfterCurrent or the user stopped
    if( m_nextTrackCandidate || finalPosition < trackLength )
        return;

    debug() << "nothing more to play...";
    // no more stuff to play. make sure to reset the active track so that pressing play
    // will start at the top of the playlist (or whereever the navigator wants to start)
    // instead of just replaying the last track
    The::playlist()->setActiveRow( -1 );

    // we also need to mark all tracks as unplayed or some navigators might be unhappy
    The::playlist()->setAllUnplayed();
}

void
Playlist::Actions::normalizeDynamicPlaylist()
{
    if ( typeid( *m_navigator ) == typeid( DynamicTrackNavigator ) )
    {
        static_cast<DynamicTrackNavigator*>(m_navigator)->appendUpcoming();
    }
}

void
Playlist::Actions::repaintPlaylist()
{
    The::mainWindow()->playlistDock()->currentView()->update();
}

void
Playlist::Actions::restoreDefaultPlaylist()
{
    DEBUG_BLOCK

    // The PlaylistManager needs to be loaded or podcast episodes and other
    // non-collection Tracks will not be loaded correctly.
    The::playlistManager();
    Playlist::Restorer *restorer = new Playlist::Restorer();
    restorer->restore( QUrl::fromLocalFile(Amarok::defaultPlaylistPath()) );
    connect( restorer, &Playlist::Restorer::restoreFinished, restorer, &QObject::deleteLater );
}

namespace The
{
    AMAROK_EXPORT Playlist::Actions* playlistActions() { return Playlist::Actions::instance(); }
}
