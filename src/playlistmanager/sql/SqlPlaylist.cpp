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

#include "SqlPlaylist.h"

#include "CollectionManager.h"
#include "Debug.h"
#include "meta/stream/Stream.h"
#include "SqlStorage.h"
#include "playlistmanager/PlaylistManager.h"
#include "playlistmanager/sql/SqlPlaylistGroup.h"
#include "playlistmanager/sql/SqlUserPlaylistProvider.h"

#include <typeinfo>

Meta::SqlPlaylist::SqlPlaylist( const QString & name, const Meta::TrackList
        &tracks, Meta::SqlPlaylistGroupPtr parent, PlaylistProvider *provider,
        const QString &urlId )
    : m_dbId( -1 )
    , m_parent( parent )
    , m_tracks( tracks )
    , m_provider( provider)
    , m_name( name )
    , m_description( QString() )
    , m_urlId( urlId )
    , m_tracksLoaded( true )
{
    saveToDb();
}

Meta::SqlPlaylist::SqlPlaylist( const QStringList & resultRow,
                                Meta::SqlPlaylistGroupPtr parent,
                                PlaylistProvider *provider )
    : m_parent( parent )
    , m_provider( provider)
    , m_tracksLoaded( false )
{
    m_dbId = resultRow[0].toInt();
    m_name = resultRow[2];
    m_description = resultRow[3];
    m_urlId = resultRow[4];

    //loadTracks();

    //debug() << m_name << " created with pointer " << this << " and parent " << this->parent();
}


Meta::SqlPlaylist::~SqlPlaylist()
{
}

Meta::SqlPlaylistGroupPtr
Meta::SqlPlaylist::parent() const
{
    return m_parent;
}

QStringList
Meta::SqlPlaylist::groups()
{
    QStringList groups;
    if( m_parent )
        groups << m_parent->name();
    return groups;
}

void
Meta::SqlPlaylist::setGroups( const QStringList &groups )
{
    DEBUG_BLOCK
    debug() << groups;
    //HACK: fix this to use m_provider;
    SqlUserPlaylistProvider *provider = dynamic_cast<SqlUserPlaylistProvider *>(The::playlistManager()->defaultUserPlaylists());
    if( !provider )
    {
        debug() << "ERROR: could not cast the default UserPlaylistProvider" << __FILE__ << __LINE__;
        return;
    }

    if( groups.isEmpty() )
        m_parent = Meta::SqlPlaylistGroupPtr();
    else
        m_parent = provider->group( groups.first() );

    saveToDb();
}

bool
Meta::SqlPlaylist::saveToDb( bool tracks )
{
    DEBUG_BLOCK

    int parentId = -1;
    if( m_parent )
        parentId = m_parent->id();

    SqlStorage * sql =  CollectionManager::instance()->sqlStorage();

    //figure out if we have a urlId and if this id is already in the db, if so, update it instead of creating a new one.
    if( !m_urlId.isEmpty() )
    {
        debug() << "Checking " << m_urlId << " against db";

        //check if urlId exists
        QString query = "SELECT id from playlists WHERE urlid='%1'";
        query = query.arg( sql->escape( m_urlId ) );
        QStringList result = sql->query( query );

        if( !result.isEmpty() )
        {
            //set this id to the already existing one
            m_dbId =  result.at( 0 ).toInt();
            debug() << "Got existing playlist with id " << m_dbId;
        }
    }

    if( m_dbId != -1 )
    {
        //update existing
        QString query = "UPDATE playlists SET parent_id=%1, name='%2', description='%3' WHERE id=%4;";
        query = query.arg( QString::number( parentId ) )
                .arg( sql->escape( m_name ) )
                .arg( sql->escape( m_description ) )
                .arg( QString::number( m_dbId ) );
        CollectionManager::instance()->sqlStorage()->query( query );

        if( tracks )
        {
            //delete existing tracks and insert all
            debug() << "Updating existing playlist";
            query = "DELETE FROM playlist_tracks where playlist_id=%1;";
            query = query.arg( QString::number( m_dbId ) );
            CollectionManager::instance()->sqlStorage()->query( query );
            saveTracks();
        }
    }
    else
    {
        //insert new
        debug() << "Creating new playlist";
        QString query = "INSERT INTO playlists ( parent_id, name, description, urlid ) VALUES ( %1, '%2', '%3', '%4' );";
        query = query.arg( QString::number( parentId ) )
                .arg( sql->escape( m_name ) )
                .arg( sql->escape( m_description ) )
                .arg( sql->escape( m_urlId ) );
        m_dbId = CollectionManager::instance()->sqlStorage()->insert( query, NULL );
        if ( tracks )
            saveTracks();
    }

    //HACK! if this has just been added from the collection scanner, the list is full of "dirty" tracks that might not all have been
    //properly trackForUrl'ed, so clear the track list so we reload if we ever need them!
    if( !m_urlId.isEmpty() )
    {
        m_tracks.clear();
        m_tracksLoaded = false;
    }

    //clean the cache
    if( m_parent )
        m_parent->clear();

    return true;
}

void
Meta::SqlPlaylist::saveTracks()
{
    int trackNum = 1;
    SqlStorage * sql =  CollectionManager::instance()->sqlStorage();

    foreach( Meta::TrackPtr trackPtr, m_tracks )
    {
        if ( trackPtr && trackPtr->album() && trackPtr->artist() )
        {
            QString query = "INSERT INTO playlist_tracks ( playlist_id, track_num, url, title, album, artist, length, uniqueid ) VALUES ( %1, %2, '%3', '%4', '%5', '%6', %7, '%8' );";
            query = query.arg( QString::number( m_dbId ), QString::number( trackNum ),
                        sql->escape( trackPtr->playableUrl().url() ),
                        sql->escape( trackPtr->prettyName() ),
                        sql->escape( trackPtr->album()->prettyName() ),
                        sql->escape( trackPtr->artist()->prettyName() ),
                        QString::number( trackPtr->length() ),
                        sql->escape( trackPtr->uidUrl() ) );
            sql->insert( query, NULL );

            trackNum++;
        }
    }
}

Meta::TrackList
Meta::SqlPlaylist::tracks()
{
    //DEBUG_BLOCK
    if ( !m_tracksLoaded )
        loadTracks();

    //debug() << "track count: " << m_tracks.count();
    return m_tracks;
}

void
Meta::SqlPlaylist::addTrack( Meta::TrackPtr track, int position )
{
    DEBUG_BLOCK
    int insertAt = (position == -1) ? m_tracks.count() : position;
    m_tracks.insert( insertAt, track );
    saveToDb( true );
    notifyObserversTrackAdded( track, position );
}

void
Meta::SqlPlaylist::removeTrack( int position )
{
    DEBUG_BLOCK
    debug() << "position: " << position;
    if( position < 0 || position >= m_tracks.size() )
        return;
    m_tracks.removeAt( position );
    saveToDb( true );
    notifyObserversTrackRemoved( position );
}

void
Meta::SqlPlaylist::loadTracks()
{
    QString query = "SELECT playlist_id, track_num, url, title, album, artist, length FROM playlist_tracks WHERE playlist_id=%1 ORDER BY track_num";
    query = query.arg( QString::number( m_dbId ) );

    QStringList result = CollectionManager::instance()->sqlStorage()->query( query );

    int resultRows = result.count() / 7;

    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*7, 7 );
        KUrl url = KUrl( row[2] );
        //debug() << "url: " << url.url();

        Meta::TrackPtr trackPtr = CollectionManager::instance()->trackForUrl( url );

        if ( trackPtr ) {

            if ( typeid( * trackPtr.data() ) == typeid( MetaStream::Track ) )  {

                debug() << "got stream from trackForUrl, setting album to " << row[4];

                MetaStream::Track * streamTrack = dynamic_cast<MetaStream::Track *> ( trackPtr.data() );

                if ( streamTrack ) {
                    streamTrack->setTitle( row[3] );
                    streamTrack->setAlbum( row[4] );
                    streamTrack->setArtist( row[5] );
                }

            }

            m_tracks << trackPtr;
            //debug() << "added track: " << trackPtr->name();

        }
    }

    m_tracksLoaded = true;
}

void
Meta::SqlPlaylist::setName( const QString & name )
{
    m_name = name;
    saveToDb( false ); //no need to resave all tracks
}

void
Meta::SqlPlaylist::removeFromDb()
{
    DEBUG_BLOCK
    QString query = "DELETE FROM playlist_tracks WHERE playlist_id=%1";
    query = query.arg( QString::number( m_dbId ) );
    CollectionManager::instance()->sqlStorage()->query( query );

    query = "DELETE FROM playlists WHERE id=%1";
    query = query.arg( QString::number( m_dbId ) );
    CollectionManager::instance()->sqlStorage()->query( query );
}

int
Meta::SqlPlaylist::id()
{
    return m_dbId;
}
