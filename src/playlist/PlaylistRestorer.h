/****************************************************************************************
 * Copyright (c) 2012 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#ifndef AMAROK_PLAYLISTRESTORER_H
#define AMAROK_PLAYLISTRESTORER_H

#include "core-impl/playlists/types/file/PlaylistFile.h"

namespace Playlist
{

class Actions;
/**
 * Implements loading of default playlist or default track
 * in case if no playlist was saved (e.g. first run of Amarok)
 */
class Restorer: public QObject, public Playlists::PlaylistObserver
{
    Q_OBJECT

    public:
        Restorer();

        /**
         * Initiate restoring procedure.
         * @param path path to the playlist to restore
         */
        void restore( const QUrl &path );

        // PlaylistObserver methods:
        virtual void tracksLoaded( Playlists::PlaylistPtr );

    Q_SIGNALS:
        void restoreFinished();

    private:
        /**
         * Runs default tune if there is no playlist to restore
         */
        void runJingle();

        /**
         * Processes so far loaded tracks.
         * If track is a playlist, then its loading is triggered
         */
        void processTracks();

        Playlists::PlaylistFilePtr m_playlistToRestore;
        Meta::TrackList m_tracks;
        /// tracks last processed track position
        QMutableListIterator<Meta::TrackPtr> m_position;
};
}

#endif
