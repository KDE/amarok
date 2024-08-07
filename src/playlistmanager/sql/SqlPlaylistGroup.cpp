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

#include "SqlPlaylistGroup.h"

#include "core-impl/storage/StorageManager.h"
#include "core/support/Debug.h"
#include <core/storage/SqlStorage.h>

#include <typeinfo>

namespace Playlists {

SqlPlaylistGroup::SqlPlaylistGroup( const QStringList & dbResultRow,
                                          const SqlPlaylistGroupPtr &parent,
                                          PlaylistProvider *provider )
    : m_hasFetchedChildGroups( false )
    , m_hasFetchedChildPlaylists( false )
    , m_parent( parent )
    , m_provider( provider )
{
    m_dbId = dbResultRow[0].toInt();
    m_name = dbResultRow[2];
    m_description = dbResultRow[3];
}

SqlPlaylistGroup::SqlPlaylistGroup( const QString & name,
                                          const SqlPlaylistGroupPtr &parent,
                                          PlaylistProvider *provider )
    : m_dbId( -1 )
    , m_hasFetchedChildGroups( false )
    , m_hasFetchedChildPlaylists( false )
    , m_name( name )
    , m_description( QString() )
    , m_parent( parent )
    , m_provider( provider )
{}

SqlPlaylistGroup::~SqlPlaylistGroup()
{
    //DEBUG_BLOCK
    //debug() << "deleting " << m_name;
}

void
SqlPlaylistGroup::save()
{
    int parentId = 0;
    if ( m_parent )
        parentId = m_parent->id();

    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;

    if ( m_dbId != -1 )
    {
        //update existing
        QString query = QStringLiteral("UPDATE playlist_groups SET parent_id=%1, name='%2', \
                description='%3' WHERE id=%4;");
        query = query.arg( QString::number( parentId ), m_name,
                           m_description, QString::number( m_dbId ) );
        sqlStorage->query( query );
    }
    else
    {
        //insert new
        QString query = QStringLiteral("INSERT INTO playlist_groups ( parent_id, name, \
                description) VALUES ( %1, '%2', '%3' );");
        query = query.arg( QString::number( parentId ), m_name, m_description );
        m_dbId = sqlStorage->insert( query, QString() );
    }
}

void
SqlPlaylistGroup::setName( const QString & name )
{
    m_name = name;
    save();
}

void
SqlPlaylistGroup::setDescription( const QString &description )
{
    m_description = description;
    save();
}

void
SqlPlaylistGroup::removeFromDb()
{
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return;


    QString query = QStringLiteral("DELETE FROM playlist_groups where id=%1;");
    query = query.arg( QString::number( m_dbId ) );
    QStringList result = sqlStorage->query( query );
}

void
SqlPlaylistGroup::clear()
{
    /* m_childPlaylists, m_childGroups are AmarokSharedPointers, so we should be able to
       just clear the list and the playlistptrs will delete themselves
    */
    m_childGroups.clear();
    m_childPlaylists.clear();

    m_hasFetchedChildGroups = false;
    m_hasFetchedChildPlaylists = false;
}

void
SqlPlaylistGroup::setParent( const SqlPlaylistGroupPtr &parent )
{
    if( parent )
        m_parent = SqlPlaylistGroupPtr::staticCast( parent );
    else
        debug() << "You have to create the parent first before " << name() <<
            " can be added to it";
    save();
}

SqlPlaylistGroupList
SqlPlaylistGroup::childSqlGroups() const
{
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return SqlPlaylistGroupList();

    if ( !m_hasFetchedChildGroups )
    {
        QString query = QStringLiteral("SELECT id, parent_id, name, description FROM \
                playlist_groups where parent_id=%1 ORDER BY name;");
        query = query.arg( QString::number( m_dbId ) );
        QStringList result = sqlStorage->query( query );

        int resultRows = result.count() / 4;

        for( int i = 0; i < resultRows; i++ )
        {
            QStringList row = result.mid( i*4, 4 );
            SqlPlaylistGroup* mutableThis =
                    const_cast<SqlPlaylistGroup*>( this );
            m_childGroups << SqlPlaylistGroupPtr(
                new SqlPlaylistGroup( row, SqlPlaylistGroupPtr( mutableThis ), m_provider )
            );
        }

        m_hasFetchedChildGroups = true;
    }

    return m_childGroups;
}

SqlPlaylistList
SqlPlaylistGroup::childSqlPlaylists() const
{
    auto sqlStorage = StorageManager::instance()->sqlStorage();
    if( !sqlStorage )
        return SqlPlaylistList();

    if ( !m_hasFetchedChildPlaylists )
    {
        QString query = QStringLiteral("SELECT id, parent_id, name, urlid FROM \
                playlists where parent_id=%1 ORDER BY name;");
        query = query.arg( QString::number( m_dbId ) );
        QStringList result = sqlStorage->query( query );

        int resultRows = result.count() / 4;

        for( int i = 0; i < resultRows; i++ )
        {
            QStringList row = result.mid( i*4, 4 );
            SqlPlaylistGroup* mutableThis =
                    const_cast<SqlPlaylistGroup*>( this );
            m_childPlaylists << SqlPlaylistPtr(
                    new SqlPlaylist(
                                row,
                                SqlPlaylistGroupPtr( mutableThis ),
                                m_provider
                    )
            );
        }
        m_hasFetchedChildPlaylists = true;
    }
    return m_childPlaylists;
}

SqlPlaylistGroupList
SqlPlaylistGroup::allChildGroups() const
{
    SqlPlaylistGroupList groups;
    groups << childSqlGroups();
    const auto cg = groups;
    for( const SqlPlaylistGroupPtr &childGroup : cg )
    {
        groups << childGroup->allChildGroups();
    }
    return groups;
}

SqlPlaylistList
SqlPlaylistGroup::allChildPlaylists() const
{
    SqlPlaylistList playlists;
    playlists << childSqlPlaylists();
    for( SqlPlaylistGroupPtr childGroup : childSqlGroups() )
    {
        playlists << childGroup->allChildPlaylists();
    }
    return playlists;
}

} //namespace Playlists

