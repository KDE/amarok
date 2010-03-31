/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#define DEBUG_PREFIX "SqlRegistry"

#include "SqlRegistry.h"

#include "core/support/Debug.h"

#include "SqlCollection.h"

#include <QMutableHashIterator>
#include <QMutexLocker>

SqlRegistry::SqlRegistry( Collections::SqlCollection* collection )
    : QObject( 0 )
    , m_collection( collection )
    , m_storage( 0 )
{
    setObjectName( "SqlRegistry" );

    m_timer = new QTimer( this );
    m_timer->setInterval( 300 * 1000 );  //try to clean up every 300 seconds, change if necessary
    m_timer->setSingleShot( false );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( emptyCache() ) );
    m_timer->start();
}

SqlRegistry::~SqlRegistry()
{
    //don't delete m_collection
}


Meta::TrackPtr
SqlRegistry::getTrack( const QString &url )
{
    int deviceid = m_collection->mountPointManager()->getIdForUrl( url );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceid, url );
    TrackId id(deviceid, rpath);
    QMutexLocker locker( &m_trackMutex );
    QMutexLocker locker2( &m_uidMutex );
    if( m_trackMap.contains( id ) )
        return m_trackMap.value( id );
    else
    {
        Meta::TrackPtr track = Meta::SqlTrack::getTrack( deviceid, rpath, m_collection );
        if( track )
        {
            m_trackMap.insert( id, track );
            m_uidMap.insert( KSharedPtr<Meta::SqlTrack>::staticCast( track )->uidUrl(), track );
        }
        return track;
    }
}

Meta::TrackPtr
SqlRegistry::getTrack( const QStringList &rowData )
{
    TrackId id( rowData[0].toInt(), rowData[1] );
    QString uid = rowData[2];
    QMutexLocker locker( &m_trackMutex );
    QMutexLocker locker2( &m_uidMutex );
    if( m_trackMap.contains( id ) )
        return m_trackMap.value( id );
    else if( m_uidMap.contains( uid ) )
        return m_uidMap.value( uid );
    else
    {
        Meta::SqlTrack *sqlTrack =  new Meta::SqlTrack( m_collection, rowData );
        sqlTrack->setCapabilityDelegate( createTrackDelegate() );
        Meta::TrackPtr track( sqlTrack );
        if( track )
        {
            m_trackMap.insert( id, track );
            m_uidMap.insert( KSharedPtr<Meta::SqlTrack>::staticCast( track )->uidUrl(), track );

        }
        return track;
    }
}

void
SqlRegistry::updateCachedUrl( const QPair<QString, QString> &oldnew )
{
    QMutexLocker locker( &m_trackMutex );
    QMutexLocker locker2( &m_uidMutex );
    int deviceid = m_collection->mountPointManager()->getIdForUrl( oldnew.first );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceid, oldnew.first );
    TrackId id(deviceid, rpath);
    if( m_trackMap.contains( id ) )
    {
        Meta::TrackPtr track = m_trackMap[id];
        m_trackMap.remove( id );
        int newdeviceid = m_collection->mountPointManager()->getIdForUrl( oldnew.second );
        QString newrpath = m_collection->mountPointManager()->getRelativePath( newdeviceid, oldnew.second );
        TrackId newid( newdeviceid, newrpath );
        m_trackMap.insert( newid, track );
    }
}

void
SqlRegistry::updateCachedUid( const QString &oldUid, const QString &newUid )
{
    QMutexLocker locker( &m_trackMutex );
    QMutexLocker locker2( &m_uidMutex );
    if( m_uidMap.contains( oldUid ) )
    {
        Meta::TrackPtr track = m_uidMap[oldUid];
        m_uidMap.remove( oldUid );
        m_uidMap.insert( newUid, track );
    }
}

Meta::TrackPtr
SqlRegistry::getTrackFromUid( const QString &uid )
{
    QMutexLocker locker( &m_trackMutex );
    QMutexLocker locker2( &m_uidMutex );
    if( m_uidMap.contains( uid ) )
        return m_uidMap.value( uid );
    else
    {

        Meta::TrackPtr track( Meta::SqlTrack::getTrackFromUid( uid, m_collection ) );
        if( track )
        {
            //we need to ensure that this track has a capability delegate or not much will work for tracks loaded from a playlist.
            Meta::SqlTrack * sqlTrack = dynamic_cast<Meta::SqlTrack *>( track.data() );
            if( sqlTrack )
                sqlTrack->setCapabilityDelegate( createTrackDelegate() );

            int deviceid = m_collection->mountPointManager()->getIdForUrl( track->playableUrl().path() );
            QString rpath = m_collection->mountPointManager()->getRelativePath( deviceid, track->playableUrl().path() );
            TrackId id(deviceid, rpath);
            m_trackMap.insert( id, track );
            m_uidMap.insert( uid, track );
        }
        return track;
    } 
}

bool
SqlRegistry::checkUidExists( const QString &uid )
{ 
    QMutexLocker locker( &m_uidMutex );
    if( m_uidMap.contains( uid ) )
        return true;
    return false;
}

Meta::ArtistPtr
SqlRegistry::getArtist( const QString &name, int id, bool refresh )
{
    QMutexLocker locker( &m_artistMutex );
    if( m_artistMap.contains( id ) && !refresh )
        return m_artistMap.value( id );
    else
    {
        if( id == -1 )
        {
            QString query = QString( "SELECT id FROM artists WHERE name = '%1';" ).arg( m_storage->escape( name ) );
            QStringList res = m_storage->query( query );
            if( res.isEmpty() )
            {
                QString insert = QString( "INSERT INTO artists( name ) VALUES ('%1');" ).arg( m_storage->escape( name ) );
                id = m_storage->insert( insert, "artists" );
            }
            else
            {
                id = res[0].toInt();
            }
        }

        if( m_artistMap.contains( id ) )
        {
            if( refresh )
                KSharedPtr<Meta::SqlArtist>::staticCast( m_artistMap.value( id ) )->updateData( m_collection, id, name );
            return m_artistMap.value( id );
        }

        Meta::SqlArtist *sqlArtist = new Meta::SqlArtist( m_collection, id, name );
        sqlArtist->setCapabilityDelegate( createArtistDelegate() );
        Meta::ArtistPtr artist( sqlArtist );
        m_artistMap.insert( id, artist );
        return artist;
    }
}

Meta::GenrePtr
SqlRegistry::getGenre( const QString &name, int id, bool refresh )
{
    QMutexLocker locker( &m_genreMutex );
    if( m_genreMap.contains( id ) && !refresh )
        return m_genreMap.value( id );
    else
    {
        if( id == -1 )
        {
            QString query = QString( "SELECT id FROM genres WHERE name = '%1';" ).arg( m_storage->escape( name ) );
            QStringList res = m_storage->query( query );
            if( res.isEmpty() )
            {
                QString insert = QString( "INSERT INTO genres( name ) VALUES ('%1');" ).arg( m_storage->escape( name ) );
                id = m_storage->insert( insert, "genres" );
            }
            else
            {
                id = res[0].toInt();
            }
        }

        if( m_genreMap.contains( id ) )
        {
            if( refresh )
                KSharedPtr<Meta::SqlGenre>::staticCast( m_genreMap.value( id ) )->updateData( m_collection, id, name );
            return m_genreMap.value( id );
        }

        Meta::GenrePtr genre( new Meta::SqlGenre( m_collection, id, name ) );
        m_genreMap.insert( id, genre );
        return genre;
    }
}

Meta::ComposerPtr
SqlRegistry::getComposer( const QString &name, int id, bool refresh )
{
    QMutexLocker locker( &m_composerMutex );
    if( m_composerMap.contains( id ) && !refresh )
        return m_composerMap.value( id );
    else
    {
        if( id == -1 )
        {
            QString query = QString( "SELECT id FROM composers WHERE name = '%1';" ).arg( m_storage->escape( name ) );
            QStringList res = m_storage->query( query );
            if( res.isEmpty() )
            {
                QString insert = QString( "INSERT INTO composers( name ) VALUES ('%1');" ).arg( m_storage->escape( name ) );
                id = m_storage->insert( insert, "composers" );
            }
            else
            {
                id = res[0].toInt();
            }
        }

        if( m_composerMap.contains( id ) )
        {
            if( refresh )
                KSharedPtr<Meta::SqlComposer>::staticCast( m_composerMap.value( id ) )->updateData( m_collection, id, name );
            return m_composerMap.value( id );
        }

        Meta::ComposerPtr composer( new Meta::SqlComposer( m_collection, id, name ) );
        m_composerMap.insert( id, composer );
        return composer;
    }
}

Meta::YearPtr
SqlRegistry::getYear( const QString &name, int id, bool refresh )
{
    QMutexLocker locker( &m_yearMutex );
    if( m_yearMap.contains( id ) && !refresh )
        return m_yearMap.value( id );
    else
    {
        if( id == -1 )
        {
            QString query = QString( "SELECT id FROM years WHERE name = '%1';" ).arg( m_storage->escape( name ) );
            QStringList res = m_storage->query( query );
            if( res.isEmpty() )
            {
                QString insert = QString( "INSERT INTO years( name ) VALUES ('%1');" ).arg( m_storage->escape( name ) );
                id = m_storage->insert( insert, "years" );
            }
            else
            {
                id = res[0].toInt();
            }
        }

        if( m_yearMap.contains( id ) )
        {
            if( refresh )
                KSharedPtr<Meta::SqlYear>::staticCast( m_yearMap.value( id ) )->updateData( m_collection, id, name );
            return m_yearMap.value( id );
        }

        Meta::YearPtr year( new Meta::SqlYear( m_collection, id, name ) );
        m_yearMap.insert( id, year );
        return year;
    }
}

Meta::AlbumPtr
SqlRegistry::getAlbum( const QString &name, int id, int artist, bool refresh )
{
    QMutexLocker locker( &m_albumMutex );
    if( m_albumMap.contains( id ) && !refresh )
        return m_albumMap.value( id );
    else
    {
        if( id == -1 )
        {
            QString query = QString( "SELECT id FROM albums WHERE name = '%1' AND " ).arg( m_storage->escape( name ) );
            if( artist >= 1)
            {
                query += QString( "artist = %1" ).arg( artist );
            }
            else
            {
                query += QString( "(artist = %1 OR artist IS NULL)" ).arg( artist );
            }
            QStringList res = m_storage->query( query );
            if( res.isEmpty() )
            {
                QString insert = QString( "INSERT INTO albums( name,artist ) VALUES ('%1',%2);" ).arg( m_storage->escape( name ), QString::number( artist ) );
                id = m_storage->insert( insert, "albums" );
            }
            else
            {
                id = res[0].toInt();
            }
        }

        if( m_albumMap.contains( id ) )
        {
            if( refresh )
                KSharedPtr<Meta::SqlAlbum>::staticCast( m_albumMap.value( id ) )->updateData( m_collection, id, name, artist );
            return m_albumMap.value( id );
        }

        Meta::SqlAlbum *sqlAlbum = new Meta::SqlAlbum( m_collection, id, name, artist );
        sqlAlbum->setCapabilityDelegate( createAlbumDelegate() );
        Meta::AlbumPtr album( sqlAlbum );
        m_albumMap.insert( id, album );
        return album;
    }
}

void
SqlRegistry::emptyCache()
{
    bool hasTrack, hasAlbum, hasArtist, hasYear, hasGenre, hasComposer, hasUid;
    hasTrack = hasAlbum = hasArtist = hasYear = hasGenre = hasComposer = hasUid = false;

    //try to avoid possible deadlocks by aborting when we can't get all locks
    if ( ( hasTrack = m_trackMutex.tryLock() )
         && ( hasAlbum = m_albumMutex.tryLock() )
         && ( hasArtist = m_artistMutex.tryLock() )
         && ( hasYear = m_yearMutex.tryLock() )
         && ( hasGenre = m_genreMutex.tryLock() )
         && ( hasComposer = m_composerMutex.tryLock() )
         && ( hasUid = m_uidMutex.tryLock() ) )
    {
        //this very simple garbage collector doesn't handle cyclic object graphs
        //so care has to be taken to make sure that we are not dealing with a cyclic graph
        //by invalidating the tracks cache on all objects
        #define foreachInvalidateCache( Type, RealType, x ) \
        for( QMutableHashIterator<int,Type > iter(x); iter.hasNext(); ) \
            RealType::staticCast( iter.next().value() )->invalidateCache()

        foreachInvalidateCache( Meta::AlbumPtr, KSharedPtr<Meta::SqlAlbum>, m_albumMap );
        foreachInvalidateCache( Meta::ArtistPtr, KSharedPtr<Meta::SqlArtist>, m_artistMap );
        foreachInvalidateCache( Meta::GenrePtr, KSharedPtr<Meta::SqlGenre>, m_genreMap );
        foreachInvalidateCache( Meta::ComposerPtr, KSharedPtr<Meta::SqlComposer>, m_composerMap );
        foreachInvalidateCache( Meta::YearPtr, KSharedPtr<Meta::SqlYear>, m_yearMap );

        //elem.count() == 2 is correct because elem is one pointer to the object
        //and the other is stored in the hash map (except for m_trackMap, where
        //another refence is stored in m_uidMap
        #define foreachCollectGarbage( Key, Type, RefCount, x ) \
        for( QMutableHashIterator<Key,Type > iter(x); iter.hasNext(); ) \
        { \
            Type elem = iter.next().value(); \
            if( elem.count() == RefCount ) \
                iter.remove(); \
        }

        foreachCollectGarbage( TrackId, Meta::TrackPtr, 3, m_trackMap )
        foreachCollectGarbage( QString, Meta::TrackPtr, 2, m_uidMap )
        //run before artist so that album artist pointers can be garbage collected
        foreachCollectGarbage( int, Meta::AlbumPtr, 2, m_albumMap )
        foreachCollectGarbage( int, Meta::ArtistPtr, 2, m_artistMap )
        foreachCollectGarbage( int, Meta::GenrePtr, 2, m_genreMap )
        foreachCollectGarbage( int, Meta::ComposerPtr, 2, m_composerMap )
        foreachCollectGarbage( int, Meta::YearPtr, 2, m_yearMap )
    }

    //make sure to unlock all necessary locks
    //important: calling unlock() on an unlocked mutex gives an undefined result
    //unlocking a mutex locked by another thread results in an error, so be careful
    if( hasTrack ) m_trackMutex.unlock();
    if( hasAlbum ) m_albumMutex.unlock();
    if( hasArtist ) m_artistMutex.unlock();
    if( hasYear ) m_yearMutex.unlock();
    if( hasGenre ) m_genreMutex.unlock();
    if( hasComposer ) m_composerMutex.unlock();
    if( hasUid ) m_uidMutex.unlock();
}

Capabilities::AlbumCapabilityDelegate*
SqlRegistry::createAlbumDelegate() const
{
    return 0;
}

Capabilities::ArtistCapabilityDelegate*
SqlRegistry::createArtistDelegate() const
{
    return 0;
}

Capabilities::TrackCapabilityDelegate*
SqlRegistry::createTrackDelegate() const
{
    return 0;
}

#include "SqlRegistry.moc"

