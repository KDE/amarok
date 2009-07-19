/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef METASQLPLAYLIST_H
#define METASQLPLAYLIST_H

#include "Playlist.h"
// #include "playlistmanager/sql/SqlPlaylistGroup.h"

namespace Meta
{
class SqlPlaylist;
typedef KSharedPtr<SqlPlaylist> SqlPlaylistPtr;
typedef QList<SqlPlaylistPtr> SqlPlaylistList;

class SqlPlaylistGroup;
typedef KSharedPtr<SqlPlaylistGroup> SqlPlaylistGroupPtr;
typedef QList<SqlPlaylistGroupPtr> SqlPlaylistGroupList;

/**
    A playlist that saves and loads itself from the Amarok database

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class SqlPlaylist : public Playlist
{
    public:
        //SqlPlaylist( int id );
        SqlPlaylist( const QString &name, const TrackList &tracks,
                SqlPlaylistGroupPtr parent, const QString &urlId = QString() );
        SqlPlaylist( const QStringList & resultRow, SqlPlaylistGroupPtr parent );

        ~SqlPlaylist();

        /* Playlist virtual functions */
        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; }
        virtual QString description() const { return m_description; }
        virtual void setName( const QString &name );

        virtual QStringList groups();
        void setGroups( const QStringList &groups );

        bool saveToDb( bool tracks = true );
        void setDescription( const QString &description ) { m_description = description; }

        void setParent( Meta::SqlPlaylistGroupPtr parent );

        int id();

        /** returns all tracks in this playlist */
        virtual TrackList tracks();
        virtual void addTrack( Meta::TrackPtr track, int position = -1 );
        virtual void removeTrack( int position );

        bool hasCapabilityInterface( Meta::Capability::Type type ) const { Q_UNUSED( type ); return false; }
        Capability* createCapabilityInterface( Capability::Type type ) { Q_UNUSED( type ); return 0; }

        Meta::SqlPlaylistGroupPtr parent() const;

        void removeFromDb();

    private:
        void loadTracks();
        void saveTracks();

        int m_dbId;
        Meta::SqlPlaylistGroupPtr m_parent;
        Meta::TrackList m_tracks;
        QString m_name;
        QString m_description;
        QString m_urlId;

        bool m_tracksLoaded;
};

}

Q_DECLARE_METATYPE( Meta::SqlPlaylistPtr )
Q_DECLARE_METATYPE( Meta::SqlPlaylistList )

#endif
