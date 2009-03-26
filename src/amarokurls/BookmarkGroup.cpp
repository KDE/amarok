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

#include "BookmarkGroup.h"

#include "CollectionManager.h"
#include "Debug.h"
#include "SqlStorage.h"

#include <typeinfo>

BookmarkGroup::BookmarkGroup( const QStringList & dbResultRow, BookmarkGroupPtr parent )
    : BookmarkViewItem()
    , m_parent( parent )
    ,  m_hasFetchedChildGroups( false )
    , m_hasFetchedChildPlaylists( false )
{
    m_dbId = dbResultRow[0].toInt();
    m_name = dbResultRow[2];
    m_description = dbResultRow[3];

}

BookmarkGroup::BookmarkGroup( const QString & name, BookmarkGroupPtr parent )
    : BookmarkViewItem()
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


BookmarkGroup::~BookmarkGroup()
{
    //DEBUG_BLOCK
    //debug() << "deleting " << m_name;
    clear();
}

void BookmarkGroup::save()
{
    int parentId = 0;
    if ( m_parent )
        parentId = m_parent->id();

    if ( m_dbId != -1 ) {
        //update existing
        QString query = "UPDATE bookmark_groups SET parent_id=%1, name='%2', description='%3' WHERE id=%4;";
        query = query.arg( QString::number( parentId ) ).arg( m_name ).arg( m_description ).arg( QString::number( m_dbId ) );
        CollectionManager::instance()->sqlStorage()->query( query );

    } else {
        //insert new

        QString query = "INSERT INTO bookmark_groups ( parent_id, name, description) VALUES ( %1, '%2', '%3' );";
        query = query.arg( QString::number( parentId ) ).arg( m_name ).arg( m_description );
        m_dbId = CollectionManager::instance()->sqlStorage()->insert( query, NULL );

    }

}

BookmarkGroupList BookmarkGroup::childGroups() const
{
    //DEBUG_BLOCK
    if ( !m_hasFetchedChildGroups ) {

        QString query = "SELECT id, parent_id, name, description FROM bookmark_groups where parent_id=%1 ORDER BY name;";
        query = query.arg( QString::number( m_dbId ) );
        QStringList result = CollectionManager::instance()->sqlStorage()->query( query );


        int resultRows = result.count() / 4;

        for( int i = 0; i < resultRows; i++ )
        {
            QStringList row = result.mid( i*4, 4 );
            BookmarkGroup* mutableThis = const_cast<BookmarkGroup*>( this );
            m_childGroups << BookmarkGroupPtr( new BookmarkGroup( row, BookmarkGroupPtr( mutableThis ) ) );
        }

        m_hasFetchedChildGroups = true;

    }

    return m_childGroups;
}

BookmarkList BookmarkGroup::childBookmarks() const
{
    //DEBUG_BLOCK
    //debug() << "my name: " << m_name << " my pointer: " << this;
    if ( !m_hasFetchedChildPlaylists ) {
        QString query = "SELECT id, parent_id, name, url, description, custom FROM bookmarks where parent_id=%1 ORDER BY name;";
        query = query.arg( QString::number( m_dbId ) );
        QStringList result = CollectionManager::instance()->sqlStorage()->query( query );

        //debug() << "Result: " << result;
        int resultRows = result.count() / 6;

        for( int i = 0; i < resultRows; i++ )
        {
            QStringList row = result.mid( i*6, 6 );
            BookmarkGroup* mutableThis = const_cast<BookmarkGroup*>( this );
            m_childBookmarks << AmarokUrlPtr( new AmarokUrl( row, BookmarkGroupPtr( mutableThis ) ) );
        }
        m_hasFetchedChildPlaylists = true;
    }
    return m_childBookmarks;
}

int BookmarkGroup::id() const
{
    return m_dbId;
}

QString BookmarkGroup::name() const
{
    return m_name;
}

QString BookmarkGroup::description() const
{
    return m_description;
}

int BookmarkGroup::childCount() const
{
    //DEBUG_BLOCK
    return childGroups().count() + childBookmarks().count();
}

void BookmarkGroup::clear()
{
    //DEBUG_BLOCK
//m_childBookmarks, m_childGroups are KSharedPtrs, so we should be able to just clear the list
//and the playlistptrs will delete themselves
    m_childGroups.clear();
    m_childBookmarks.clear();

    m_hasFetchedChildGroups = false;
    m_hasFetchedChildPlaylists = false;
}



void BookmarkGroup::rename(const QString & name)
{
    m_name = name;
    save();
}

void BookmarkGroup::deleteChild( BookmarkViewItemPtr item )
{
    if ( typeid( * item ) == typeid( BookmarkGroup ) )
    {
        BookmarkGroupPtr group = BookmarkGroupPtr::staticCast( item );
        m_childGroups.removeAll( group );
    }
    else if ( typeid( * item ) == typeid( AmarokUrl ) )
    {
        AmarokUrlPtr bookmark = AmarokUrlPtr::staticCast( item );
        m_childBookmarks.removeAll( bookmark );
    }
}

void BookmarkGroup::removeFromDb()
{
    DEBUG_BLOCK

    foreach( BookmarkGroupPtr group, m_childGroups )
        group->removeFromDb();
    foreach( AmarokUrlPtr bookmark, m_childBookmarks )
        bookmark->removeFromDb();

    QString query = QString( "DELETE FROM bookmark_groups where id=%1;").arg( QString::number( m_dbId ) );
    debug() << "query: " << query;
    QStringList result = CollectionManager::instance()->sqlStorage()->query( query );
}

void BookmarkGroup::reparent( BookmarkGroupPtr parent )
{
    m_parent = parent;
    save();
}
