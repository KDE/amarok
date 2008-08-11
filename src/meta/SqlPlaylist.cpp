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

#include "SqlPlaylist.h"

#include "CollectionManager.h"
#include "Debug.h"
#include "SqlStorage.h"

#include "SqlPlaylistGroup.h"

using namespace Meta;

SqlPlaylist::SqlPlaylist( const QString & name, const Meta::TrackList & tracks, SqlPlaylistGroupPtr parent )
    : SqlPlaylistViewItem()
    , Playlist()
    , m_dbId( -1 )
    , m_parent( parent )
    , m_tracks( tracks )
    , m_name( name )
    , m_description( QString() )
    , m_tracksLoaded( true )
{
    saveToDb();
}

SqlPlaylist::SqlPlaylist( const QStringList & resultRow, SqlPlaylistGroupPtr parent )
    : SqlPlaylistViewItem()
    , Playlist()
    , m_parent( parent )
    , m_tracksLoaded( false )
{
    m_dbId = resultRow[0].toInt();
    m_name = resultRow[2];
    m_description = resultRow[3];

    //loadTracks();

    debug() << m_name << " created with pointer " << this << " and parent " << this->parent();
}


SqlPlaylist::~SqlPlaylist()
{
}

bool SqlPlaylist::saveToDb( bool tracks )
{
    int parentId = -1;
    if ( m_parent )
        parentId = m_parent->id();

    SqlStorage * sql =  CollectionManager::instance()->sqlStorage();

    if( m_dbId != -1 )
    {
        //update existing
        QString query = "UPDATE playlists SET parent_id=%1, name='%2', description='%3' WHERE id=%4;";
        query = query.arg( QString::number( parentId ) ).arg( sql->escape( m_name ) ).arg( sql->escape( m_description ) ).arg( QString::number( m_dbId ) );
        CollectionManager::instance()->sqlStorage()->query( query );

        if( tracks )
        {
            //delete existing tracks and insert all
            query = "DELETE FROM TABLE playlist_tracks where playlist_id=%1;";
            query = query.arg( QString::number( m_dbId ) );
            CollectionManager::instance()->sqlStorage()->query( query );
            saveTracks();
        }
    }
    else
    {
        //insert new
        QString query = "INSERT INTO playlists ( parent_id, name, description ) VALUES ( %1, '%2', '%3' );";
        query = query.arg( QString::number( parentId ) ).arg( sql->escape( m_name ) ).arg( sql->escape( m_description ) );
        m_dbId = CollectionManager::instance()->sqlStorage()->insert( query, NULL );
        if ( tracks )
            saveTracks();
    }
    return true;
}

void SqlPlaylist::saveTracks()
{
    int trackNum = 1;
    SqlStorage * sql =  CollectionManager::instance()->sqlStorage();

    foreach( Meta::TrackPtr trackPtr, m_tracks )
    {
        QString query = "INSERT INTO playlist_tracks ( playlist_id, track_num, url, title, album, artist, length, uniqueid ) VALUES ( %1, %2, '%3', '%4', '%5', '%6', %7, '%8' );";
        query = query.arg( QString::number( m_dbId ), QString::number( trackNum ), sql->escape( trackPtr->uidUrl() ),
                            sql->escape( trackPtr->prettyName() ), sql->escape( trackPtr->album()->prettyName() ),
                            sql->escape( trackPtr->artist()->prettyName() ), QString::number( trackPtr->length() ),
                            sql->escape( trackPtr->uidUrl() ) );
        sql->insert( query, NULL );

        trackNum++;
    }
}

TrackList SqlPlaylist::tracks()
{
    DEBUG_BLOCK
    if ( !m_tracksLoaded )
        loadTracks();

    debug() << "track count: " << m_tracks.count();
    return m_tracks;
}

void SqlPlaylist::loadTracks()
{
    DEBUG_BLOCK
    QString query = "SELECT playlist_id, track_num, url, title, album, artist, length FROM playlist_tracks WHERE playlist_id=%1 ORDER BY track_num";
    query = query.arg( QString::number( m_dbId ) );

    QStringList result = CollectionManager::instance()->sqlStorage()->query( query );

    int resultRows = result.count() / 7;

    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*7, 7 );
        KUrl url = KUrl( row[2] );
        debug() << "url: " << url.url();

        Meta::TrackPtr trackPtr = CollectionManager::instance()->trackForUrl( url );

        if ( trackPtr ) {
            m_tracks << trackPtr;
            debug() << "added track: " << trackPtr->name();
        }
    }

    m_tracksLoaded = true;
}


void Meta::SqlPlaylist::rename(const QString & name)
{
    m_name = name;
    saveToDb( false ); //no need to resave all tracks
}

void Meta::SqlPlaylist::removeFromDb()
{
    DEBUG_BLOCK
    QString query = "DELETE FROM playlist_tracks WHERE playlist_id=%1";
    query = query.arg( QString::number( m_dbId ) );
    CollectionManager::instance()->sqlStorage()->query( query );

    query = "DELETE FROM playlists WHERE id=%1";
    query = query.arg( QString::number( m_dbId ) );
    CollectionManager::instance()->sqlStorage()->query( query );
}

int Meta::SqlPlaylist::id()
{
    return m_dbId;
}


void Meta::SqlPlaylist::reparent( SqlPlaylistGroupPtr parent )
{
    m_parent = parent;
    saveToDb( false );
}

