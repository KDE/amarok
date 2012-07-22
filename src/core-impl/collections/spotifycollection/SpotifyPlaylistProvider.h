/****************************************************************************************
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
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
#ifndef SPOTIFYPLAYLISTPROVIDER_H
#define SPOTIFYPLAYLISTPROVIDER_H

#include "core-impl/playlists/UserPlaylistProvider.h"
#include "SpotifyMeta.h"
#include "SpotifyCollection.h"
#include "./support/Controller.h"

class QAction;

namespace Playlists {

/**
 * @author Ryan Feng <odayfans@gmail.com>
 */
class AMAROK_EXPORT SpotifyPlaylistProvider : public UserPlaylistProvider, public PlaylistObserver
{
    Q_OBJECT

    public:
        SpotifyPlaylistProvider( Collections::SpotifyCollection* collection, QObject* parent = 0 );
        virtual ~SpotifyPlaylistProvider();

        // PlaylistProvider methods
        virtual QString prettyName() const;
        virtual KIcon icon() const;

        virtual int playlistCount() const;
        virtual Playlists::PlaylistList playlists();

        virtual Playlists::PlaylistPtr addPlaylist( Playlists::PlaylistPtr playlist );

        // UserPlaylistProvider methods
        virtual Playlist::PlaylistPtr save( const Meta::TrackList &tracks,
                                            const QString& name = QString() );
        virtual QActionList providerActions();
        virtual QActionList playlistActions( Playlists::PlaylistPtr playlist );
        virtual QActionList trackActions( Playlists::PlaylistPtr playlist, int trackIndex );

        virtual bool isWritable();
        virtual void rename( Playlists::PlaylistPtr playlist, const QString& newName );
        virtual bool deletePlaylists( Playlists::PlaylistList playlistList );

        // Methods from PlaylistObserver
        virtual void trackAdded( Playlists::PlaylistPtr playlist, Meta::TrackPtr track, int position );
        virtual void trackRemoved( Playlists::PlaylistPtr playlist, int position );

        // SpotifyPlaylistProvider specific methods:
        // Set a playlist in sync mode or not
        virtual void setSync( Playlists::SpotifyPlaylistPtr, bool sync = true );
        virtual bool sync( const Playlists::SpotifyPlaylistPtr playlistPtr ) const;

    public slots:

    private:
        SpotifyCollection* m_collection;
        SpotifyController* m_controller;

}; // class SpotifyPlaylistProvider
}; // namespace Playlists

#endif
