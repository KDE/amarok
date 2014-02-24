/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#ifndef PLAYLIST_EXPORTER_H
#define PLAYLIST_EXPORTER_H

#include "core/meta/forward_declarations.h"
#include "core/playlists/Playlist.h"

#include <QObject>

class QScriptValue;
class QScriptEngine;

namespace AmarokScript
{
    class AmarokScriptEngine;

    // SCRIPTDOX PROTOTYPE Playlists::Playlist Playlist
    class PlaylistPrototype : public QObject, public Playlists::PlaylistObserver
    {
        Q_OBJECT

        /**
         * Indicates whether this playlist object is valid.
         */
        Q_PROPERTY( bool isValid READ isValid )

        Q_PROPERTY( QString name READ toString WRITE setName )

        /**
         * @returns a unique identifier for a playlist.
         */
        Q_PROPERTY( QUrl uidUrl READ uidUrl )

        /**
         * Returns the number of tracks this playlist contains. -1 if tracks are not
         * yet loaded (call {@link #triggerFullLoad triggerFullLoad} or {@link #triggerQuickLoad triggerQuickLoad} in this case).
         * If you get non-negative number, all tracks have been already loaded.
         */
        Q_PROPERTY( int trackCount READ trackCount )

        /**
         * @returns the provider this playlist belongs to.
         */
        Q_PROPERTY( Playlists::PlaylistProvider* provider READ provider )

    public:
        static void init( QScriptEngine *engine );
        PlaylistPrototype( Playlists::PlaylistPtr playlist );
        Playlists::PlaylistPtr data() const { return m_playlist; }

    public slots:
            /**
             * Returns loaded tracks in this playlist. Note that the list may be incomplete,
             * to be sure, check that trackCount() is non-negative. Otherwise you have to call
             * {@link #triggerFullLoad triggerFullLoad} or {@link #triggerQuickLoad triggerQuickLoad}.
             */
            Meta::TrackList tracks();

            /**
             * Trigger asynchronous loading of this playlist.
             * Postpone the finished() signal until the all constituent tracks have resolved
             * and their full metadata is available. Also use this flag when you need to immediately play
             * the tracks.
             * Note: This means you'll just get the finished signal a bit later.
             */
            void triggerFullLoad();

            /**
             * Trigger asynchronous loading of this playlist.
             * Don't wait for constituent tracks to fully load before emitting the finished signal.
             */
            void triggerQuickLoad();

            /**
             * Add the track to a certain position in the playlist
             *
             * @param position place to add this track. The default value -1 appends to
             * the end.
             *
             * @note if the position is larger then the size of the playlist append to the
             * end without generating an error.
             */
            void addTrack( Meta::TrackPtr track, int position = -1 );

            /**
             * Remove track at the specified position
             */
            void removeTrack( int position );

            QString toString() const;

    private:
        void trackAdded( Playlists::PlaylistPtr playlist, Meta::TrackPtr track, int position ) override;
        void trackRemoved( Playlists::PlaylistPtr playlist, int position ) override;

        bool isValid() const;
        QUrl uidUrl() const;
        void setName( const QString &name );
        int trackCount() const;
        Playlists::PlaylistProvider *provider() const;

        QScriptEngine *m_engine;
        Playlists::PlaylistPtr m_playlist;

    signals:
        void loaded( Meta::TrackList tracks );
        void addedTrack( Meta::TrackPtr track, int position );
        void removedTrack( int position );
    };
}

#endif
