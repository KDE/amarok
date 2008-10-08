/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 *                        (C) 2008 Seb Ruiz <ruiz@kde.org>
 *                        (C) 2008 Soren Harward <stharward@gmail.com>
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

#ifndef AMAROK_PLAYLISTACTIONS_H
#define AMAROK_PLAYLISTACTIONS_H

#include "Debug.h"
#include "EngineObserver.h"
#include "meta/Playlist.h"
#include "navigators/TrackNavigator.h"
#include "PlaylistModel.h"


namespace Playlist
{
class Actions;
}

namespace The
{
AMAROK_EXPORT Playlist::Actions* playlistActions();
}

namespace Playlist
{
class TrackNavigator;

enum PlaybackMode
{
    StandardPlayback  = 0,
    TrackPlayback     = 1,
    AlbumPlayback     = 2,
    PlaylistPlayback  = 4,
    RandomPlayback    = 8,
    RepeatPlayback    = 16
};

enum StopAfterMode
{
    StopNever = 0,
    StopAfterCurrent,
    StopAfterQueue
};


class AMAROK_EXPORT Actions : public QObject , public EngineObserver
{
    Q_OBJECT

public:
    static Actions* instance();
    static void destroy();

    /**
     * This is called by the engine before the current track ends. It
     * will figure out the next track and enqueue it. This won't
     * actually change the track. That happens in the engine when the
     * current track ends.
     */
    void requestNextTrack();

    /**
     * Figure out the next track, and start playing it immediately.
     */
    void requestUserNextTrack();

    /**
     * Figure out the previous track and start playing it immediately.
     */
    void requestPrevTrack();

    /*
     * When request[X]Track functions are called, they will call back to one
     * these functions so the controller knows which track to send to the engine
     */
    void setNextRow( int row )
    {
        setNextTrack( Model::instance()->idAt( row ) );
    }
    void setUserNextRow( int row )
    {
        setUserNextTrack( Model::instance()->idAt( row ) );
    }
    void setPrevRow( int row )
    {
        setUserNextTrack( Model::instance()->idAt( row ) );
    }

    void setNextTrack( quint64 trackid );
    void setUserNextTrack( quint64 trackid );
    void setPrevTrack( quint64 trackid );

    StopAfterMode stopAfterMode() const
    {
        return m_stopAfterMode;
    }
    void setStopAfterMode( StopAfterMode m )
    {
        m_stopAfterMode = m;
    }

public slots:
    void play();
    void play( int );
    void play( const QModelIndex& index );
    void play( quint64, bool now = true );
    void next();
    void back();
    void playlistModeChanged(); //! Changes the tracknavigator
    void repopulateDynamicPlaylist();

private:
    Actions( QObject* parent = 0 );
    ~Actions();

    void engineStateChanged( Phonon::State currentState, Phonon::State oldState ); //from EngineObserver
    void engineNewTrackPlaying(); //from EngineObserver

    quint64 m_nextTrackCandidate;
    TrackNavigator* m_navigator;                //! the strategy of what to do when a track finishes playing
    Playlist::StopAfterMode m_stopAfterMode;
    bool m_trackError;
    bool m_waitingForNextTrack;

    static Actions* s_instance; //! instance variable
};
} // namespace Playlist

#endif
