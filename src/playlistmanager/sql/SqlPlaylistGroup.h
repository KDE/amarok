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
#include "meta/PlaylistGroup.h"

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
            SqlPlaylistGroup( const QStringList &dbResultRow );
            SqlPlaylistGroup( const QString &name );

            ~SqlPlaylistGroup();

            /* Meta::PlaylistGroup virtual functions */
            QString name() const { return m_name; }
            QString description() const { return m_description; }

            virtual Meta::PlaylistGroupPtr parent() const { return
                    Meta::PlaylistGroupPtr::staticCast(m_parent); }

            virtual void setName( const QString &name );
            virtual void setParent( Meta::PlaylistGroupPtr parent );
            virtual void setDescription( const QString &description );

            /* SqlPlaylistGroup specific functions */
            int id() const { return m_dbId; }
            int parentId() const { return m_parentId; }
            void save();
            void removeFromDb();

        private:
            int m_dbId;
            int m_parentId;
            Meta::SqlPlaylistGroupPtr m_parent;
            QString m_name;
            QString m_description;
    };
}

Q_DECLARE_METATYPE( Meta::SqlPlaylistGroupPtr )
Q_DECLARE_METATYPE( Meta::SqlPlaylistGroupList )

#endif
