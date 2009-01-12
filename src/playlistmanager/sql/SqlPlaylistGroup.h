/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef SQLPLAYLISTGROUP_H
#define SQLPLAYLISTGROUP_H

#include "meta/Meta.h"
#include "meta/SqlPlaylist.h"

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

        @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
    */
    class SqlPlaylistGroup : public PlaylistGroup
    {
        public:

            SqlPlaylistGroup( const QStringList &dbResultRow, SqlPlaylistGroupPtr parent );
            explicit SqlPlaylistGroup( const QString &name, SqlPlaylistGroupPtr parent = SqlPlaylistGroupPtr() );

            ~SqlPlaylistGroup();

            /* Meta::PlaylistGroup virtual functions */
            QString name() const;
            QString description() const;

            virtual int childCount() const;

            virtual PlaylistGroupPtr parent() const { return m_parent; }

            virtual void rename( const QString &name );

            virtual SqlPlaylistGroupList childGroups() const;
            virtual Meta::SqlPlaylistList childPlaylists() const;

            virtual void reparent( PlaylistGroupPtr parent );

            virtual void clear();

            virtual void addChildPlaylist( Meta::PlaylistPtr playlist );
            virtual void removeChildPlaylist( Meta::PlaylistPtr playlist );

            virtual void addChildGroup( Meta::PlaylistGroup group );
            virtual void removeChildGroup( Meta::PlaylistGroup group );

            /* SqlPlaylistGroup specific functions */
            int id() const;
            void save();

            void deleteChild( SqlPlaylistViewItemPtr item );
            void removeFromDb();

        private:
            int m_dbId;
            SqlPlaylistGroupPtr m_parent;
            QString m_name;
            QString m_description;

            mutable SqlPlaylistGroupList m_childGroups;
            mutable Meta::SqlPlaylistList m_childPlaylists;

            mutable bool m_hasFetchedChildGroups;
            mutable bool m_hasFetchedChildPlaylists;

    };
}

#endif
