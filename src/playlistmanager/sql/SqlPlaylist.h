/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef PLAYLISTSSQLPLAYLIST_H
#define PLAYLISTSSQLPLAYLIST_H

#include "core/playlists/Playlist.h"


namespace Playlists
{
class PlaylistProvider;
class SqlPlaylist;
typedef KSharedPtr<SqlPlaylist> SqlPlaylistPtr;
typedef QList<SqlPlaylistPtr> SqlPlaylistList;

class SqlPlaylistGroup;
typedef KSharedPtr<SqlPlaylistGroup> SqlPlaylistGroupPtr;
typedef QList<SqlPlaylistGroupPtr> SqlPlaylistGroupList;

/**
    A playlist that saves and loads itself from the Amarok database

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class SqlPlaylist : public Playlist, public Meta::Observer
{
    public:
        SqlPlaylist( const QString &name, const Meta::TrackList &tracks,
                SqlPlaylistGroupPtr parent, PlaylistProvider *provider,
                const QString &urlId = QString() );
        SqlPlaylist( const QStringList & resultRow, SqlPlaylistGroupPtr parent,
                     PlaylistProvider *provider );

        ~SqlPlaylist();

        /* Playlist virtual functions */
        virtual KUrl uidUrl() const;
        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; }
        virtual QString description() const { return m_description; }

        virtual PlaylistProvider *provider() const { return m_provider; }

        virtual void setName( const QString &name );

        virtual QStringList groups();
        void setGroups( const QStringList &groups );

        bool saveToDb( bool tracks = true );
        void setDescription( const QString &description ) { m_description = description; }

        void setParent( SqlPlaylistGroupPtr parent );

        int id();

        /** returns all tracks in this playlist */
        virtual Meta::TrackList tracks();
        virtual void addTrack( Meta::TrackPtr track, int position = -1 );
        virtual void removeTrack( int position );

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const
                { Q_UNUSED( type ); return false; }
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type )
                { Q_UNUSED( type ); return 0; }

        /* Meta::Observer methods */
        virtual void metadataChanged( Meta::TrackPtr track );
        virtual void metadataChanged( Meta::ArtistPtr artist ) { Q_UNUSED( artist ) }
        virtual void metadataChanged( Meta::AlbumPtr album ) { Q_UNUSED( album ) }
        virtual void metadataChanged( Meta::GenrePtr genre ) { Q_UNUSED( genre ) }
        virtual void metadataChanged( Meta::ComposerPtr composer ) { Q_UNUSED( composer ) }
        virtual void metadataChanged( Meta::YearPtr year ) { Q_UNUSED( year ) }

        SqlPlaylistGroupPtr parent() const;

        void removeFromDb();

    private:
        void loadTracks();
        void saveTracks();

        int m_dbId;
        SqlPlaylistGroupPtr m_parent;
        Meta::TrackList m_tracks;
        PlaylistProvider *m_provider;
        QString m_name;
        QString m_description;
        QString m_urlId;

        bool m_tracksLoaded;
};

}

Q_DECLARE_METATYPE( Playlists::SqlPlaylistPtr )
Q_DECLARE_METATYPE( Playlists::SqlPlaylistList )

#endif
