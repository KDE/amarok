/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
 * Copyright (c) 2011 Lucas Lira Gomes <x8lucas8x@gmail.com>                            *
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

#ifndef SYNCEDPLAYLIST_H
#define SYNCEDPLAYLIST_H

#include "core/playlists/Playlist.h"

/** A synchronized playlist that will try to get the attached slaves into the same state as the
  * master. The first playlist attached (through constructor) is the master.
  */
class SyncedPlaylist : public Playlists::Playlist, public Playlists::PlaylistObserver
{
    public:
        explicit SyncedPlaylist( Playlists::PlaylistPtr playlist );
        virtual ~SyncedPlaylist() {}

        //Playlists::Playlist methods
        virtual QUrl uidUrl() const;
        virtual QString name() const;
        virtual QString prettyName() const;
        virtual Playlists::PlaylistProvider *provider() const;
        virtual void setName( const QString &name ) { Q_UNUSED( name ); }

        virtual Meta::TrackList tracks();
        virtual int trackCount() const;

        virtual void addTrack( Meta::TrackPtr track, int position = -1 );
        virtual void removeTrack( int position );

        //PlaylistObserver methods
        virtual void metadataChanged( Playlists::PlaylistPtr playlist );
        virtual void tracksLoaded( Playlists::PlaylistPtr);
        virtual void trackAdded( Playlists::PlaylistPtr playlist, Meta::TrackPtr track,
                                 int position );
        virtual void trackRemoved( Playlists::PlaylistPtr playlist, int position );

        //SyncedPlaylist methods
        /** returns true when there is no active playlist associated with it anymore. */
        virtual bool isEmpty() const;
        virtual void addPlaylist( Playlists::PlaylistPtr playlist );

        virtual bool syncNeeded() const;
        virtual void doSync() const;

        virtual void removePlaylistsFrom( Playlists::PlaylistProvider *provider );

        virtual Playlists::PlaylistList playlists() const { return m_playlists; }

        virtual Playlists::PlaylistPtr master() const;
        virtual Playlists::PlaylistList slaves() const;

    protected:
        SyncedPlaylist() {};

    private:
        Playlists::PlaylistList m_playlists;
};

typedef AmarokSharedPointer<SyncedPlaylist> SyncedPlaylistPtr;
typedef QList<SyncedPlaylistPtr> SyncedPlaylistList;

Q_DECLARE_METATYPE( SyncedPlaylistPtr )
Q_DECLARE_METATYPE( SyncedPlaylistList )

#endif // SYNCEDPLAYLIST_H
