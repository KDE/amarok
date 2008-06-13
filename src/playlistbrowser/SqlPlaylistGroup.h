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
#include "SqlPlaylistViewItem.h"
#include "meta/SqlPlaylist.h"

#include <QString>
#include <QStringList>

class SqlPlaylistGroup;


typedef QList<SqlPlaylistGroup *> SqlPlaylistGroupList;
typedef QList<Meta::SqlPlaylist *> SqlPlaylistDirectList;





/**
A class for allowing a "folder structure" in the playlist browser and the database. Takes care of reading and writing  itself to the database.

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class SqlPlaylistGroup : public SqlPlaylistViewItem
{
    public:

        SqlPlaylistGroup ( const QStringList &dbResultRow, SqlPlaylistGroup * parent );
        SqlPlaylistGroup ( const QString &name, SqlPlaylistGroup * parent = 0 );

        ~SqlPlaylistGroup();

        int id();
        QString name() const;
        QString description() const;

        virtual int childCount();

        virtual SqlPlaylistGroup * parent() { return m_parent; }

        virtual void rename( const QString &name );

        void save();
        SqlPlaylistGroupList childGroups();
        SqlPlaylistDirectList childPlaylists();

        void clear();

        void deleteChild( SqlPlaylistViewItem * item );
        void removeFromDb();

    private:

        int m_dbId;
        SqlPlaylistGroup * m_parent;
        QString m_name;
        QString m_description;

        SqlPlaylistGroupList m_childGroups;
        SqlPlaylistDirectList m_childPlaylists;

        bool m_hasFetchedChildGroups;
        bool m_hasFetchedChildPlaylists;





};

#endif
