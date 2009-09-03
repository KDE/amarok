/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
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

#ifndef AMAROK_PLAYLISTACTIONS_H
#define AMAROK_PLAYLISTACTIONS_H

#include "Debug.h"
#include "EngineObserver.h"
#include "meta/Playlist.h"
#include "proxymodels/AbstractModel.h"


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

enum StopAfterMode
{
    StopNever = 0,
    StopAfterCurrent,
    StopAfterQueue
};


class AMAROK_EXPORT Actions : public QObject, public EngineObserver
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

    /**
     * Set next track from track id, but don't start playing immediately
     */
    void requestTrack( quint64 id );

    StopAfterMode stopAfterMode() const { return m_stopAfterMode; }
    void setStopAfterMode( StopAfterMode m ) { m_stopAfterMode = m; }
    void setTrackToBeLast( quint64 id ) { m_trackToBeLast = id; }
    bool willStopAfterTrack( const quint64 id ) const { return m_trackToBeLast == id; }

    /**
     * Make sure that there are enough tracks in the current playlist
     * if it is dynamic and the user removed tracks.
     */
    void normalizeDynamicPlayist();

    // This shouldn't be in Actions, it doesn't make sense
    int queuePosition( quint64 id );

public slots:
    void play();
    void play( const int row );
    void play( const QModelIndex& index );
    void play( const quint64 id, bool now = true );
    void next();
    void back();
    void playlistModeChanged(); //! Changes the tracknavigator
    void repopulateDynamicPlaylist();
    void queue( QList<int> rows );
    void dequeue( QList<int> rows );

private:
    Actions();
    ~Actions();

    void engineStateChanged( Phonon::State currentState, Phonon::State oldState ); //from EngineObserver
    void engineNewTrackPlaying(); //from EngineObserver

    quint64 m_nextTrackCandidate;
    quint64 m_currentTrack;
    quint64 m_trackToBeLast;
    TrackNavigator* m_navigator;                //! the strategy of what to do when a track finishes playing
    Playlist::StopAfterMode m_stopAfterMode;
    bool m_trackError;
    bool m_waitingForNextTrack;

    AbstractModel *m_topmostModel;

    static Actions* s_instance; //! instance variable
};
} // namespace Playlist

#endif
