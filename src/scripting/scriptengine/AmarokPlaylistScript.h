/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#ifndef AMAROK_PLAYLIST_SCRIPT_H
#define AMAROK_PLAYLIST_SCRIPT_H

#include "core/meta/forward_declarations.h"

#include <QObject>
#include <QMetaType>
#include <QStringList>

class QModelIndex;
class QUrl;

namespace AmarokScript
{
    class AmarokScriptEngine;

    // SCRIPTDOX: Amarok.Playlist
    class AmarokPlaylistScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokPlaylistScript( AmarokScriptEngine *engine );

        public slots:

            /**
             * Return the index of the currently playing track in the playlist.
             */
            int activeIndex();

            /**
             * Return the number of tracks in the playlist.
             */
            int totalTrackCount();

            /**
             * Save the current playlist in the default playlist path.
             */
            QString saveCurrentPlaylist();

            /**
             * Load the track represented by the url and append to playlist.
             */
            void addMedia( const QUrl &url );

            /**
             * Append @param track to playlist.
             */
            void addTrack( Meta::TrackPtr track );

            /**
             * Load the list of tracks represented by the urls and append to playlist.
             */
            void addMediaList( const QList<QUrl> &urls );

            /**
             * Append the list of tracks to playlist.
             */
            void addTrackList( const Meta::TrackList &tracks );

            /**
             * Clear the current playlist.
             */
            void clearPlaylist();

            /**
             * Play the track at the specified index in the playlist.
             */
            void playByIndex( int index );

            /**
             * Prepend the the track represented by the passed url and start playing it.
             */
            void playMedia( const QUrl &url );

            /**
             * Prepend @param track and start playing it.
             */
            void playTrack( Meta::TrackPtr track );

            /**
             * Prepend the the tracks represented by the passed urls and start playing them.
             */
            void playMediaList( const QList<QUrl> &urls );

            /**
             * Prepend tracks in @param trackList and start playing them.
             */
            void playTrackList( const Meta::TrackList &trackList );

            /**
             * Remove the currently playing track from the playlist.
             */
            void removeCurrentTrack();

            /**
             * Remove the track at @param index from the playlist.
             */
            void removeByIndex( int index );

            /**
             * Save the current playlist at the absolute path @param path.
             */
            void savePlaylist( const QString& path );

            /**
             * Set whether to stop playing after the current track.
             */
            void setStopAfterCurrent( bool on );

            /*
             * Indicates whether will stop playing after the current track.
             */
            bool stopAfterCurrent();

            /**
             * Show/ Hide the playlist.
             */
            void togglePlaylist();

            /**
             * Return a list of urls representing all the tracks in the playlist.
             */
            QStringList filenames();

            /**
             * Return the track at the specified position in the playlist.
             */
            Meta::TrackPtr trackAt( int row );

            /**
             * Get an unsorted list of indices of the currently selected tracks in the playlist.
             */
            QList<int> selectedIndexes();

            /**
             * Get an unsorted list of urls of the currently selected tracks in the playlist.
             */
            QStringList selectedFilenames();

        signals:

            /**
             * Emitted when tracks are added to the playlist.
             */
            void trackInserted( int start, int end );

            /**
             * Emitted when tracks are removed from the playlist.
             */
            void trackRemoved( int start, int end );

        private slots:
            void slotTrackInserted( const QModelIndex&, int start, int end );
            void slotTrackRemoved( const QModelIndex&, int start, int end );

        private:
            AmarokScriptEngine* m_scriptEngine;
    };
}

Q_DECLARE_METATYPE( QList<QUrl> )

#endif
