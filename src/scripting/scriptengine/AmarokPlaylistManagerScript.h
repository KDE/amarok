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

#ifndef AMAROK_PLAYLISTMANAGER_SCRIPT_H
#define AMAROK_PLAYLISTMANAGER_SCRIPT_H

#include "core/meta/forward_declarations.h"

#include <QObject>

class QScriptEngine;
namespace Playlists
{
    class Playlist;
    class PlaylistProvider;

    typedef KSharedPtr<Playlist> PlaylistPtr;
    typedef QList<PlaylistPtr> PlaylistList;
    typedef QList<Playlists::PlaylistProvider *> PlaylistProviderList;
}

namespace AmarokScript
{
    class AmarokScriptEngine;

    //ANM-TODO podcasts!
    // SCRIPTDOX Amarok.PlaylistManager
    class AmarokPlaylistManagerScript : public QObject
    {
        Q_OBJECT

        // SCRIPTDOX ENUM Amarok.PlaylistManager.PlaylistCategory
        // enum PlaylistCategory { UserPlaylist, PodcastChannel };

        /**
         * @returns all available categories registered at that moment
         */
        Q_PROPERTY( QList<int> availableCategories READ availableCategories )

        public:
            AmarokPlaylistManagerScript( AmarokScriptEngine* engine );

        public slots:

            /**
             * @returns playlists of a certain category from all registered PlaylistProviders
             */
            Playlists::PlaylistList playlistsOfCategory( int playlistCategory );

            /**
             * @returns all PlaylistProviders that provide a certain playlist category.
             **/
            Playlists::PlaylistProviderList providersForCategory( int playlistCategory );

            // ANM-TODO synced playlists
            /**
             * Do all the work necessary to sync playlists, including the
             * SyncedPlaylist creation and more. This sync is persistent.
             * @arg playlist of the master playlist
             * @arg playlist of the slave playlist
             */
            //void setupSync( const Playlists::PlaylistPtr master, const Playlists::PlaylistPtr slave );

            /**
             * Return provider with name @param name and category @param category.
             */
            Playlists::PlaylistProvider *playlistProvider( int category, QString name );

            /**
             *   Saves a list of tracks to a new playlist. Used in the Playlist save button.
             *   @arg tracks list of tracks to save
             *   @arg name of playlist to save
             *   @arg toProvider If 0 (default) will save to the default UserPlaylistProvider ( SQLPlaylistProvider )
             */
            bool save( Meta::TrackList tracks, const QString &name = QString(),
                       Playlists::PlaylistProvider *toProvider = 0 );

            /**
             *  Saves a playlist from a file to the database.
             *  @arg fromLocation Saved playlist file to load
             */
            bool import( const QString &fromLocation );

            /**
             * Rename @param playlist to @param newName, return true if renaming was successful,
             * false otherwise.
             */
            bool rename( Playlists::PlaylistPtr playlist, const QString &newName );

            bool deletePlaylists( Playlists::PlaylistList playlistList );

            /**
             *  Retrieves the provider owning the given playlist.
             *  Will only return multiple providers if this is a synced playlist
             *  @arg playlist the playlist whose provider we want
             */
            QList<Playlists::PlaylistProvider*>
            getProvidersForPlaylist( const Playlists::PlaylistPtr playlist );

            /**
             *  Checks if the provider to whom this playlist belongs supports writing
             *  @arg playlist the playlist we are testing for writability
             *  @return whether or not the playlist is writable
             */
            bool isWritable( const Playlists::PlaylistPtr &playlist );

        private:
            QList<int> availableCategories();

        signals:
            void updated( int category );
            void categoryAdded( int category );
            void providerAdded( Playlists::PlaylistProvider *provider, int category );
            void providerRemoved( Playlists::PlaylistProvider *provider, int category );
            void playlistAdded( Playlists::PlaylistPtr playlist, int category );
            void playlistRemoved( Playlists::PlaylistPtr playlist, int category );
            void playlistUpdated( Playlists::PlaylistPtr playlist, int category );
            void renamePlaylist( Playlists::PlaylistPtr playlist );
    };
}

#endif
