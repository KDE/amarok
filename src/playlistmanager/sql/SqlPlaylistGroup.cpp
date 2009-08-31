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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "SqlPlaylistGroup.h"

#include "CollectionManager.h"
#include "Debug.h"
#include "SqlStorage.h"

#include <typeinfo>

Meta::SqlPlaylistGroup::SqlPlaylistGroup( const QStringList & dbResultRow,
                                          Meta::SqlPlaylistGroupPtr parent,
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

Meta::SqlPlaylistGroup::SqlPlaylistGroup( const QString & name,
                                          Meta::SqlPlaylistGroupPtr parent,
                                          PlaylistProvider *provider )
    : m_dbId( -1 )
    , m_hasFetchedChildGroups( false )
    , m_hasFetchedChildPlaylists( false )
    , m_name( name )
    , m_description( QString() )
    , m_parent( parent )
    , m_provider( provider )
{}

Meta::SqlPlaylistGroup::~SqlPlaylistGroup()
{
    //DEBUG_BLOCK
    //debug() << "deleting " << m_name;
}

void
Meta::SqlPlaylistGroup::save()
{
    int parentId = 0;
    if ( m_parent )
        parentId = m_parent->id();

    if ( m_dbId != -1 )
    {
        //update existing
        QString query = "UPDATE playlist_groups SET parent_id=%1, name='%2', \
                description='%3' WHERE id=%4;";
        query = query.arg( QString::number( parentId ) ).arg( m_name ).arg(
            m_description ).arg( QString::number( m_dbId ) );
        CollectionManager::instance()->sqlStorage()->query( query );
    }
    else
    {
        //insert new
        QString query = "INSERT INTO playlist_groups ( parent_id, name, \
                description) VALUES ( %1, '%2', '%3' );";
        query = query.arg( QString::number( parentId ) ).arg( m_name ).arg(
            m_description );
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
Meta::SqlPlaylistGroup::clear()
{
    DEBUG_BLOCK
    /* m_childPlaylists, m_childGroups are KSharedPtrs, so we should be able to
       just clear the list and the playlistptrs will delete themselves
    */
    m_childGroups.clear();
    m_childPlaylists.clear();

    m_hasFetchedChildGroups = false;
    m_hasFetchedChildPlaylists = false;
}

void
Meta::SqlPlaylistGroup::setParent( Meta::SqlPlaylistGroupPtr parent )
{
    if( parent )
        m_parent = Meta::SqlPlaylistGroupPtr::staticCast( parent );
    else
        debug() << "You have to create the parent first before " << name() <<
            " can be added to it";
    save();
}

Meta::SqlPlaylistGroupList
Meta::SqlPlaylistGroup::childSqlGroups() const
{
    DEBUG_BLOCK
    if ( !m_hasFetchedChildGroups )
    {
        QString query = "SELECT id, parent_id, name, description FROM \
                playlist_groups where parent_id=%1 ORDER BY name;";
        query = query.arg( QString::number( m_dbId ) );
        QStringList result =
                CollectionManager::instance()->sqlStorage()->query( query );

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

Meta::SqlPlaylistList
Meta::SqlPlaylistGroup::childSqlPlaylists() const
{
    DEBUG_BLOCK
    if ( !m_hasFetchedChildPlaylists )
    {
        QString query = "SELECT id, parent_id, name, description, urlid FROM \
                playlists where parent_id=%1 ORDER BY name;";
        query = query.arg( QString::number( m_dbId ) );
        QStringList result =
                CollectionManager::instance()->sqlStorage()->query( query );

        //debug() << "Result: " << result;
        int resultRows = result.count() / 5;

        for( int i = 0; i < resultRows; i++ )
        {
            QStringList row = result.mid( i*5, 5 );
            SqlPlaylistGroup* mutableThis =
                    const_cast<SqlPlaylistGroup*>( this );
            m_childPlaylists << Meta::SqlPlaylistPtr(
                    new Meta::SqlPlaylist(
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

Meta::SqlPlaylistGroupList
Meta::SqlPlaylistGroup::allChildGroups() const
{
    Meta::SqlPlaylistGroupList groups;
    groups << childSqlGroups();
    foreach( Meta::SqlPlaylistGroupPtr childGroup, groups )
    {
        groups << childGroup->allChildGroups();
    }
    return groups;
}

Meta::SqlPlaylistList
Meta::SqlPlaylistGroup::allChildPlaylists() const
{
    Meta::SqlPlaylistList playlists;
    playlists << childSqlPlaylists();
    foreach( Meta::SqlPlaylistGroupPtr childGroup, childSqlGroups() )
    {
        playlists << childGroup->allChildPlaylists();
    }
    return playlists;
}
