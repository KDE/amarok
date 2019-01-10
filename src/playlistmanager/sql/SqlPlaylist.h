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
typedef AmarokSharedPointer<SqlPlaylist> SqlPlaylistPtr;
typedef QList<SqlPlaylistPtr> SqlPlaylistList;

class SqlPlaylistGroup;
typedef AmarokSharedPointer<SqlPlaylistGroup> SqlPlaylistGroupPtr;
typedef QList<SqlPlaylistGroupPtr> SqlPlaylistGroupList;

/**
    A playlist that saves and loads itself from the Amarok database

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class SqlPlaylist : public Playlist
{
    public:
        SqlPlaylist( const QString &name, const Meta::TrackList &tracks,
                SqlPlaylistGroupPtr parent, PlaylistProvider *provider,
                const QString &urlId = QString() );
        SqlPlaylist( const QStringList & resultRow, SqlPlaylistGroupPtr parent,
                     PlaylistProvider *provider );

        ~SqlPlaylist();

        /* Playlist virtual functions */
        QUrl uidUrl() const override;
        QString name() const override { return m_name; }

        PlaylistProvider *provider() const override { return m_provider; }

        void setName( const QString &name ) override;

        QStringList groups() override;
        void setGroups( const QStringList &groups ) override;

        int trackCount() const override;
        Meta::TrackList tracks() override;
        void triggerTrackLoad() override;

        void addTrack( const Meta::TrackPtr &track, int position = -1 ) override;
        void removeTrack( int position ) override;

        // SqlPlaylist-specific methods
        bool saveToDb( bool tracks = true );
        void removeFromDb();

    private:
        void loadTracks();
        void saveTracks();

        int m_dbId;
        SqlPlaylistGroupPtr m_parent;
        Meta::TrackList m_tracks;
        PlaylistProvider *m_provider;
        QString m_name;
        QString m_urlId;

        bool m_tracksLoaded;
};

}

Q_DECLARE_METATYPE( Playlists::SqlPlaylistPtr )
Q_DECLARE_METATYPE( Playlists::SqlPlaylistList )

#endif
