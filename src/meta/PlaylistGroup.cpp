/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *   Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>              *
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

#include "PlaylistGroup.h"

#include "CollectionManager.h"
#include "Debug.h"
#include "meta/Playlist.h"
#include "SqlStorage.h"

#include <typeinfo>

Meta::PlaylistGroup::PlaylistGroup( const QStringList & dbResultRow, PlaylistGroupPtr parent )
    : PlaylistViewItem()
    , m_parent( parent )
    ,  m_hasFetchedChildGroups( false )
    , m_hasFetchedChildPlaylists( false )
{
    m_dbId = dbResultRow[0].toInt();
    m_name = dbResultRow[2];
    m_description = dbResultRow[3];

}

Meta::PlaylistGroup::PlaylistGroup( const QString & name, PlaylistGroupPtr parent )
    : PlaylistViewItem()
    , m_dbId( -1 )
    , m_parent( parent )
    , m_name( name )
    , m_description( QString() )
    , m_hasFetchedChildGroups( false )
    , m_hasFetchedChildPlaylists( false )
{

    if ( parent.isNull() ) {
        //root item
        m_dbId = -1;
    }
}


Meta::PlaylistGroup::~PlaylistGroup()
{
    //DEBUG_BLOCK
    //debug() << "deleting " << m_name;
    clear();
}

void
Meta::PlaylistGroup::save()
{
    int parentId = 0;
    if ( m_parent )
        parentId = m_parent->id();

    if ( m_dbId != -1 ) {
        //update existing
        QString query = "UPDATE playlist_groups SET parent_id=%1, name='%2', description='%3' WHERE id=%4;";
        query = query.arg( QString::number( parentId ) ).arg( m_name ).arg( m_description ).arg( QString::number( m_dbId ) );
        CollectionManager::instance()->sqlStorage()->query( query );

    } else {
        //insert new

        QString query = "INSERT INTO playlist_groups ( parent_id, name, description) VALUES ( %1, '%2', '%3' );";
        query = query.arg( QString::number( parentId ) ).arg( m_name ).arg( m_description );
        m_dbId = CollectionManager::instance()->sqlStorage()->insert( query, NULL );

    }

}

Meta::PlaylistGroupList
Meta::PlaylistGroup::childGroups() const
{
    //DEBUG_BLOCK
    if ( !m_hasFetchedChildGroups ) {

        QString query = "SELECT id, parent_id, name, description FROM playlist_groups where parent_id=%1 ORDER BY name;";
        query = query.arg( QString::number( m_dbId ) );
        QStringList result = CollectionManager::instance()->sqlStorage()->query( query );


        int resultRows = result.count() / 4;

        for( int i = 0; i < resultRows; i++ )
        {
            QStringList row = result.mid( i*4, 4 );
            PlaylistGroup* mutableThis = const_cast<PlaylistGroup*>( this );
            m_childGroups << PlaylistGroupPtr( new PlaylistGroup( row, PlaylistGroupPtr( mutableThis ) ) );
        }

        m_hasFetchedChildGroups = true;

    }

    return m_childGroups;
}

Meta::PlaylistList
Meta::PlaylistGroup::childPlaylists() const
{
    //DEBUG_BLOCK
    //debug() << "my name: " << m_name << " my pointer: " << this;
    if ( !m_hasFetchedChildPlaylists ) {
        QString query = "SELECT id, parent_id, name, description, urlid FROM playlists where parent_id=%1 ORDER BY name;";
        query = query.arg( QString::number( m_dbId ) );
        QStringList result = CollectionManager::instance()->sqlStorage()->query( query );

        //debug() << "Result: " << result;
        int resultRows = result.count() / 5;

        for( int i = 0; i < resultRows; i++ )
        {
            QStringList row = result.mid( i*5, 5 );
            PlaylistGroup* mutableThis = const_cast<PlaylistGroup*>( this );
            m_childPlaylists << Meta::PlaylistPtr( new Meta::Playlist( row, PlaylistGroupPtr( mutableThis ) ) );
        }
        m_hasFetchedChildPlaylists = true;
    }
    return m_childPlaylists;
}

QString PlaylistGroup::name() const
{
    return m_name;
}

QString PlaylistGroup::description() const
{
    return m_description;
}

int PlaylistGroup::childCount() const
{
    //DEBUG_BLOCK
    return childGroups().count() + childPlaylists().count();
}

void PlaylistGroup::clear()
{
    //DEBUG_BLOCK
//m_childPlaylists, m_childGroups are KSharedPtrs, so we should be able to just clear the list
//and the playlistptrs will delete themselves
    m_childGroups.clear();
    m_childPlaylists.clear();

    m_hasFetchedChildGroups = false;
    m_hasFetchedChildPlaylists = false;
}



void PlaylistGroup::rename(const QString & name)
{
    m_name = name;
    save();
}

void PlaylistGroup::deleteChild( PlaylistViewItemPtr item )
{
    if ( typeid( * item ) == typeid( PlaylistGroup ) )
    {
        PlaylistGroupPtr group = PlaylistGroupPtr::staticCast( item );
        m_childGroups.removeAll( group );
    }
    else if ( typeid( * item ) == typeid( Meta::Playlist ) )
    {
        Meta::PlaylistPtr playlist = Meta::PlaylistPtr::staticCast( item );
        m_childPlaylists.removeAll( playlist );
    }
}

void PlaylistGroup::removeFromDb()
{
    //DEBUG_BLOCK

    foreach( PlaylistGroupPtr group, m_childGroups )
        group->removeFromDb();
    foreach( Meta::PlaylistPtr playlist, m_childPlaylists )
        playlist->removeFromDb();

    QString query = "DELETE FROM playlist_groups where id=%1;";
    query = query.arg( QString::number( m_dbId ) );
    QStringList result = CollectionManager::instance()->sqlStorage()->query( query );
}

void PlaylistGroup::reparent( PlaylistGroupPtr parent )
{
    m_parent = parent;
    save();
}
