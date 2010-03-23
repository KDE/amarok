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

#ifndef SQLPLAYLISTGROUP_H
#define SQLPLAYLISTGROUP_H

#include "core/meta/Meta.h"
#include "SqlPlaylist.h"

#include <QString>
#include <QStringList>

#include <KSharedPtr>
namespace Meta
{
    class SqlPlaylistGroup;
    typedef KSharedPtr<SqlPlaylistGroup> SqlPlaylistGroupPtr;
    typedef QList<SqlPlaylistGroupPtr> SqlPlaylistGroupList;

    /**
    A class for allowing a "folder structure" in the playlist browser and the database. Takes care of reading and writing  itself to the database.

        @author Nikolaj Hald Nielsen <nhn@kde.org>
    */
    class SqlPlaylistGroup : public virtual QSharedData
    {
        public:
            SqlPlaylistGroup( const QStringList &dbResultRow,
                    Meta::SqlPlaylistGroupPtr parent, PlaylistProvider *provider );
            SqlPlaylistGroup( const QString &name,
                    Meta::SqlPlaylistGroupPtr parent, PlaylistProvider *provider );

            ~SqlPlaylistGroup();

            QString name() const { return m_name; }
            QString description() const { return m_description; }

            Meta::SqlPlaylistGroupPtr parent() const { return m_parent; }

            void setName( const QString &name );
            void setParent( Meta::SqlPlaylistGroupPtr parent );
            void setDescription( const QString &description );

            int id() const { return m_dbId; }
            void save();
            void removeFromDb();
            void clear();
            Meta::SqlPlaylistGroupList allChildGroups() const;
            Meta::SqlPlaylistList allChildPlaylists() const;

        private:
            Meta::SqlPlaylistGroupList childSqlGroups() const;
            Meta::SqlPlaylistList childSqlPlaylists() const;

            int m_dbId;
            mutable bool m_hasFetchedChildGroups;
            mutable bool m_hasFetchedChildPlaylists;
            mutable Meta::SqlPlaylistGroupList m_childGroups;
            mutable Meta::SqlPlaylistList m_childPlaylists;
            QString m_name;
            QString m_description;
            Meta::SqlPlaylistGroupPtr m_parent;

            PlaylistProvider *m_provider;
    };
}

Q_DECLARE_METATYPE( Meta::SqlPlaylistGroupPtr )
Q_DECLARE_METATYPE( Meta::SqlPlaylistGroupList )

#endif
