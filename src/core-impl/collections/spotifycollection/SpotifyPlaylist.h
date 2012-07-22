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
#ifndef SPOTIFYPLAYLIST_H
#define SPOTIFYPLAYLIST_H

#include "core/playlists/Playlist.h"
#include "SpotifyCollection.h"

class QObject;

namespace Playlists
{
class SpotifyPlaylist: public Playlist, public QObject
{
    Q_OBJECT
    public:
        SpotifyPlaylist( Collections::SpotifyCollection* collection );
        SpotifyPlaylist( const QString& name, const Meta::SpotifyTracklist& tracks,
                         Collections::SpotifyCollection* collection, const bool sync = true );
        virtual ~SpotifyPlaylist();

        // Methods from Playlist
        virtual KUrl uidUrl() const;
        virtual QString name() const;
        virtual PlaylistProvider* provider();

        virtual void setName( const QString& name );
        virtual int trackCount() const;
        virtual Meta::TrackList tracks();

        // Load track infomation using a background thread
        virtual void triggerTrackLoad();
        virtual void addTrack( Meta::TrackPtr track, int position = -1 );
        virtual void removeTrack( int position );

    private:
        QString m_name;
        bool m_sync;
        Collections::SpotifyCollection* m_collection;
        SpotifyPlaylistProvider* m_provider;
};
}

Q_DECLARE_METATYPE( Playlists::SpotifyPlaylist* )

#endif
