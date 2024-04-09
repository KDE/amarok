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

#include "amarok_export.h"
#include "core/support/Debug.h"
#include "core/playlists/Playlist.h"

#include <QQueue>
#include <QModelIndex>
#include <QUrl>

#include <phonon/Global>

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

/**
 * This class is a central hub between the playlist model stack, the playlist navigators,
 * and the track playback engine. It ties them together to provide simple "Play", "Play
 * Next", etc. commands to the GUI code.
 */

class AMAROK_EXPORT Actions : public QObject
{
    Q_OBJECT

public:

    static Actions* instance();
    static void destroy();


    Meta::TrackPtr likelyNextTrack();
    Meta::TrackPtr likelyPrevTrack();

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

    /**
     * Request that Amarok stops playback after playing certain track.
     * @param id playlist id of a track to stop playing after or 0 to disable
     *           stopping after some track. If not specified, enable stopping
     *           after currently playing track.
     */
    void stopAfterPlayingTrack( quint64 id = -1 );

    bool willStopAfterTrack( const quint64 id ) const;

    // This shouldn't be in Actions, it doesn't make sense
    int queuePosition( quint64 id );

    // nor should this, ideally this and queuePosition
    // should be in TrackNavigator and TrackNavigator
    // should be publicly accessible
    QQueue<quint64> queue();

    bool queueMoveUp( quint64 id );
    bool queueMoveDown( quint64 id );
    bool queueMoveTo( quint64 id, const int pos );
    void dequeue( quint64 id );

public Q_SLOTS:
    void play();
    void play( const int row );
    void play( const QModelIndex& index );
    void play( const quint64 id, bool now = true );
    void next();
    void back();
    void enableDynamicMode( bool enable );

    /** Changes the tracknavigator depending on the current configuration */
    void playlistModeChanged();

    void repopulateDynamicPlaylist();

    /**
      * Shuffles tracks (that are visible in the top model) at the bottom model level
      */
    void shuffle();

    /**
     * Adds a list of top playlist model rows to the queue.
     */
    void queue( const QList<int> &rows );

    /**
     * Adds a list of playlist item unique ids to the queue.
     */
    void queue( const QList<quint64> &ids );

    void dequeue( const QList<int> &rows );
    void restoreDefaultPlaylist();

    /**
     * Make sure that there are enough tracks in the current playlist
     * if it is dynamic and the user removed tracks.
     */
    void normalizeDynamicPlaylist();


    /**
    * Repaint the playlist.
    * Useful when triggering a change that will modify the visual appearance of one or more items in the playlist
    */
    void repaintPlaylist();

Q_SIGNALS:
    void navigatorChanged();

private Q_SLOTS:
    void slotTrackPlaying( Meta::TrackPtr engineTrack );
    void slotPlayingStopped( qint64 finalPosition, qint64 trackLength );

private:
    Actions();
    ~Actions() override;

    void init();

    quint64 m_nextTrackCandidate;
    /**
     * Playlist id if a track where the playback should be stopped or 0 if playback
     * shouldn't be stopped after certain track
     */
    quint64 m_stopAfterPlayingTrackId;
    TrackNavigator* m_navigator;                //!< the strategy of what to do when a track finishes playing
    bool m_waitingForNextTrack;

    static Actions* s_instance; //!< instance variable
};
} // namespace Playlist

#endif
