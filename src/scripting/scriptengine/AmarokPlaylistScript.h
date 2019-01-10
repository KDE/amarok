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
            explicit AmarokPlaylistScript( AmarokScriptEngine *engine );

            /**
             * Return the index of the currently playing track in the playlist.
             */
            Q_INVOKABLE int activeIndex();

            /**
             * Return the number of tracks in the playlist.
             */
            Q_INVOKABLE int totalTrackCount();

            /**
             * Save the current playlist in the default playlist path.
             */
            Q_INVOKABLE QString saveCurrentPlaylist();

            /**
             * Load the track represented by the url and append to playlist.
             */
            Q_INVOKABLE void addMedia( const QUrl &url );

            /**
             * Append @param track to playlist.
             */
            Q_INVOKABLE void addTrack( const Meta::TrackPtr &track );

            /**
             * Load the list of tracks represented by the urls and append to playlist.
             */
            Q_INVOKABLE void addMediaList( const QList<QUrl> &urls );

            /**
             * Append the list of tracks to playlist.
             */
            Q_INVOKABLE void addTrackList( const Meta::TrackList &tracks );

            /**
             * Clear the current playlist.
             */
            Q_INVOKABLE void clearPlaylist();

            /**
             * Play the track at the specified index in the playlist.
             */
            Q_INVOKABLE void playByIndex( int index );

            /**
             * Prepend the track represented by the passed url and start playing it.
             */
            Q_INVOKABLE void playMedia( const QUrl &url );

            /**
             * Prepend @param track and start playing it.
             */
            Q_INVOKABLE void playTrack( const Meta::TrackPtr &track );

            /**
             * Prepend the tracks represented by the passed urls and start playing them.
             */
            Q_INVOKABLE void playMediaList( const QList<QUrl> &urls );

            /**
             * Prepend tracks in @param trackList and start playing them.
             */
            Q_INVOKABLE void playTrackList( const Meta::TrackList &trackList );

            /**
             * Remove the currently playing track from the playlist.
             */
            Q_INVOKABLE void removeCurrentTrack();

            /**
             * Remove the track at @param index from the playlist.
             */
            Q_INVOKABLE void removeByIndex( int index );

            /**
             * Save the current playlist at the absolute path @p path.
             *
             * @param path the absolute path.
             */
            Q_INVOKABLE void savePlaylist( const QString& path );

            /**
             * Set whether to stop playing after the current track.
             *
             * @param on @c true if on, @c false otherwise.
             */
            Q_INVOKABLE void setStopAfterCurrent( bool on );

            /*
             * Indicates whether will stop playing after the current track.
             */
            Q_INVOKABLE bool stopAfterCurrent();

            /**
             * Show/ Hide the playlist.
             */
            Q_INVOKABLE void togglePlaylist();

            /**
             * Return a list of urls representing all the tracks in the playlist.
             */
            Q_INVOKABLE QStringList filenames();

            /**
             * Return the track at the specified position in the playlist.
             */
            Q_INVOKABLE Meta::TrackPtr trackAt( int row );

            /**
             * Get an unsorted list of indices of the currently selected tracks in the playlist.
             */
            Q_INVOKABLE QList<int> selectedIndexes();

            /**
             * Get an unsorted list of urls of the currently selected tracks in the playlist.
             */
            Q_INVOKABLE QStringList selectedFilenames();

        Q_SIGNALS:
            /**
             * Emitted when tracks are added to the playlist.
             */
            void trackInserted( int start, int end );

            /**
             * Emitted when tracks are removed from the playlist.
             */
            void trackRemoved( int start, int end );

        private Q_SLOTS:
            void slotTrackInserted( const QModelIndex&, int start, int end );
            void slotTrackRemoved( const QModelIndex&, int start, int end );

        private:
            AmarokScriptEngine* m_scriptEngine;
    };
}

#endif
