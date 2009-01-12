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

#include "SqlPlaylistGroup.h"

#include "CollectionManager.h"
#include "Debug.h"
#include "SqlStorage.h"

#include <typeinfo>

Meta::SqlPlaylistGroup::SqlPlaylistGroup( const QStringList & dbResultRow )
{
    m_dbId = dbResultRow[0].toInt();
    m_parentId = dbResultRow[1].toInt();
    m_name = dbResultRow[2];
    m_description = dbResultRow[3];
}

Meta::SqlPlaylistGroup::SqlPlaylistGroup( const QString & name )
    : m_dbId( -1 )
    , m_parentId( -1 )
    , m_name( name )
    , m_description( QString() )
{
}

Meta::SqlPlaylistGroup::~SqlPlaylistGroup()
{
    //DEBUG_BLOCK
    //debug() << "deleting " << m_name;
}

void
Meta::SqlPlaylistGroup::save()
{
    if ( m_dbId != -1 )
    {
        //update existing
        QString query = "UPDATE playlist_groups SET parent_id=%1, name='%2', description='%3' WHERE id=%4;";
        query = query.arg( QString::number( m_parentId ) ).arg( m_name ).arg( m_description ).arg( QString::number( m_dbId ) );
        CollectionManager::instance()->sqlStorage()->query( query );

    }
    else
    {
        //insert new
        QString query = "INSERT INTO playlist_groups ( parent_id, name, description) VALUES ( %1, '%2', '%3' );";
        query = query.arg( QString::number( m_parentId ) ).arg( m_name ).arg( m_description );
        m_dbId = CollectionManager::instance()->sqlStorage()->insert( query, NULL );
    }
}

void
Meta::SqlPlaylistGroup::setName( const QString & name )
{
    m_name = name;
    save();
}

void
Meta::SqlPlaylistGroup::setDescription( const QString &description )
{
    m_description = description;
    save();
}

void
Meta::SqlPlaylistGroup::removeFromDb()
{
    DEBUG_BLOCK
    QString query = "DELETE FROM playlist_groups where id=%1;";
    query = query.arg( QString::number( m_dbId ) );
    QStringList result = CollectionManager::instance()->sqlStorage()->query( query );
}

void
Meta::SqlPlaylistGroup::setParent( Meta::PlaylistGroupPtr parent )
{
    if( parent )
        m_parent = Meta::SqlPlaylistGroupPtr::staticCast( parent );
    else
        debug() << "have to create the parent";

    save();
}
