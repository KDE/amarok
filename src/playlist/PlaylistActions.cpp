/***************************************************************************
 * copyright        : (C) 2007-2008 Ian Monroe <ian@monroe.nu>
 *                    (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 *                    (C) 2008 Seb Ruiz <ruiz@kde.org>
 *                    (C) 2008 Soren Harward <stharward@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#define DEBUG_PREFIX "Playlist::Actions"

#include "PlaylistActions.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "Debug.h"
#include "DynamicModel.h"
#include "EngineController.h"
#include "EngineObserver.h"
#include "navigators/DynamicTrackNavigator.h"
#include "navigators/RandomAlbumNavigator.h"
#include "navigators/RandomTrackNavigator.h"
#include "navigators/RepeatAlbumNavigator.h"
#include "navigators/RepeatPlaylistNavigator.h"
#include "navigators/RepeatTrackNavigator.h"
#include "navigators/StandardTrackNavigator.h"
#include "PlaylistModel.h"
#include "statusbar/StatusBar.h"

#include <QAction>

#include <typeinfo>

Playlist::Actions* Playlist::Actions::s_instance = 0;

Playlist::Actions* Playlist::Actions::instance()
{
    return ( s_instance ) ? s_instance : new Actions( 0 );
}

void
Playlist::Actions::destroy()
{
    if ( s_instance )
    {
        delete s_instance;
        s_instance = 0;
    }
}

Playlist::Actions::Actions( QObject* parent )
        : QObject( parent )
        , EngineObserver( The::engineController() )
        , m_nextTrackCandidate( 0 )
        , m_navigator( 0 )
        , m_stopAfterMode( StopNever )
        , m_trackError( false )
        , m_waitingForNextTrack( false )
{
    DEBUG_BLOCK
    s_instance = this;
    playlistModeChanged(); // sets m_navigator.
}

Playlist::Actions::~Actions()
{
    DEBUG_BLOCK
    m_navigator->deleteLater();
}


void
Playlist::Actions::requestNextTrack()
{
    if ( !m_waitingForNextTrack )
    {
        m_waitingForNextTrack = true;
        m_navigator->requestNextTrack();
    }
}

void
Playlist::Actions::requestUserNextTrack()
{
    if ( !m_waitingForNextTrack )
    {
        m_waitingForNextTrack = true;
        m_navigator->requestUserNextTrack();
    }
}

void
Playlist::Actions::requestPrevTrack()
{
    if ( !m_waitingForNextTrack )
    {
        m_waitingForNextTrack = true;
        m_navigator->requestLastTrack();
    }
}


void
Playlist::Actions::setNextTrack( quint64 trackid )
{
    m_trackError = false;
    m_waitingForNextTrack = false;
    play( trackid, false );
}

void
Playlist::Actions::setUserNextTrack( quint64 trackid )
{
    m_trackError = false;
    m_waitingForNextTrack = false;
    play( trackid );
}

void
Playlist::Actions::setPrevTrack( quint64 trackid )
{
    m_trackError = false;
    m_waitingForNextTrack = false;
    play( trackid );
}

void
Playlist::Actions::play()
{
    if ( m_waitingForNextTrack )
        return;

    if ( m_nextTrackCandidate == 0 )
        requestUserNextTrack();

    play( m_nextTrackCandidate );
}

void
Playlist::Actions::play( const QModelIndex& index )
{
    play( index.row() );
}

void
Playlist::Actions::play( int row )
{
    play( Model::instance()->idAt( row ) );
}

void
Playlist::Actions::play( quint64 trackid, bool now )
{
    if ( m_trackError )
        return;

    Model* model = The::playlistModel();

    if ( model->containsId( trackid ) )
    {
        m_nextTrackCandidate = trackid;
        if ( now )
        {
            The::engineController()->play( model->trackForId( trackid ) );
        }
        else
        {
            The::engineController()->setNextTrack( model->trackForId( trackid ) );
        }
    }
    else
    {
        m_trackError = true;
        warning() << "Invalid trackid" << trackid;
    }
}

void
Playlist::Actions::next()
{
    requestUserNextTrack();
}

void
Playlist::Actions::back()
{
    requestPrevTrack();
}

void
Playlist::Actions::playlistModeChanged()
{
    if ( m_navigator )
        m_navigator->deleteLater();

    int options = Playlist::StandardPlayback;

    debug() << "Repeat enabled: " << Amarok::repeatEnabled();
    debug() << "Random enabled: " << Amarok::randomEnabled();
    debug() << "Track mode:     " << ( Amarok::repeatTrack() || Amarok::randomTracks() );
    debug() << "Album mode:     " << ( Amarok::repeatAlbum() || Amarok::randomAlbums() );
    debug() << "Playlist mode:  " << Amarok::repeatPlaylist();
    debug() << "Dynamic mode:   " << AmarokConfig::dynamicMode();

    if ( AmarokConfig::dynamicMode() )
    {
        PlaylistBrowserNS::DynamicModel* dm = PlaylistBrowserNS::DynamicModel::instance();

        Dynamic::DynamicPlaylistPtr playlist = dm->activePlaylist();

        if ( !playlist )
            playlist = dm->defaultPlaylist();

        m_navigator = new DynamicTrackNavigator( playlist );

        return;
    }

    m_navigator = 0;

    if ( Amarok::repeatEnabled() )
        options |= Playlist::RepeatPlayback;
    if ( Amarok::randomEnabled() )
        options |= Playlist::RandomPlayback;
    if ( Amarok::repeatTrack() || Amarok::randomTracks() )
        options |= Playlist::TrackPlayback;
    if ( Amarok::repeatAlbum() || Amarok::randomAlbums() )
        options |= Playlist::AlbumPlayback;
    if ( Amarok::repeatPlaylist() )
        options |= Playlist::PlaylistPlayback;

    if ( options == Playlist::StandardPlayback )
    {
        m_navigator = new StandardTrackNavigator();
    }
    else if ( options & Playlist::RepeatPlayback )
    {
        if ( options & Playlist::TrackPlayback )
            m_navigator = new RepeatTrackNavigator();
        else if ( options & Playlist::PlaylistPlayback )
            m_navigator = new RepeatPlaylistNavigator();
        else if ( options & Playlist::AlbumPlayback )
            m_navigator = new RepeatAlbumNavigator();
    }
    else if ( options & Playlist::RandomPlayback )
    {
        if ( options & Playlist::TrackPlayback )
            m_navigator = new RandomTrackNavigator();
        else if ( options & Playlist::AlbumPlayback )
            m_navigator = new RandomAlbumNavigator();
    }

    if ( m_navigator == 0 )
    {
        debug() << "Play mode not implemented, defaulting to Standard Playback";
        m_navigator = new StandardTrackNavigator();
    }

    connect( The::playlistModel(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), m_navigator, SLOT( setPlaylistChanged() ) );
    connect( The::playlistModel(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), m_navigator, SLOT( setPlaylistChanged() ) );
}

void
Playlist::Actions::repopulateDynamicPlaylist()
{
    if ( typeid( *m_navigator ) == typeid( DynamicTrackNavigator ) )
    {
        (( DynamicTrackNavigator* )m_navigator )->repopulate();
    }
}

void
Playlist::Actions::engineStateChanged( Phonon::State currentState, Phonon::State )
{
    static int failures = 0;
    const int maxFailures = 4;

    if ( currentState == Phonon::ErrorState )
    {
        failures++;
        warning() << "Error, can not play this track.";
        warning() << "Failure count: " << failures;
        if ( failures >= maxFailures )
        {
            The::statusBar()->longMessageThreadSafe( i18n( "Too many errors encountered in playlist. Playback stopped." ), KDE::StatusBar::Warning );
            error() << "Stopping playlist.";
            failures = 0;
            m_trackError = true;
        }
    }
    else if ( currentState == Phonon::PlayingState )
    {
        if ( failures > 0 )
        {
            debug() << "Successfully played track. Resetting failure count.";
        }
        failures = 0;
        m_trackError = false;
    }
}


void
Playlist::Actions::engineNewTrackPlaying()
{
    Model* model = The::playlistModel();
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if ( track )
    {
        if ( model->containsId( m_nextTrackCandidate ) && track == model->trackForId( m_nextTrackCandidate ) )
        {
            model->setActiveId( m_nextTrackCandidate );
        }
        else
        {
            warning() << "engineNewTrackPlaying:" << track->prettyName() << "does not match what the playlist controller thought it should be";
            model->setActiveRow( model->rowForTrack( track ) ); // this will set active row to -1 if the track isn't in the playlist at all
        }
    }
    else
    {
        warning() << "engineNewTrackPlaying: not really a track";
    }

    m_nextTrackCandidate = 0;
}

namespace The
{
AMAROK_EXPORT Playlist::Actions* playlistActions()
{
    return Playlist::Actions::instance();
}
}
