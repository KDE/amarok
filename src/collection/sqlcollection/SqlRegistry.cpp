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

#include "Debug.h"

#include "SqlCollection.h"

#include <QMutableHashIterator>
#include <QMutexLocker>

using namespace Meta;

SqlRegistry::SqlRegistry( SqlCollection* collection )
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


TrackPtr
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
        TrackPtr track = SqlTrack::getTrack( deviceid, rpath, m_collection );
        if( track )
        {
            m_trackMap.insert( id, track );
            m_uidMap.insert( KSharedPtr<SqlTrack>::staticCast( track )->uidUrl(), track );
        }
        return track;
    }
}

TrackPtr
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
        SqlTrack *sqlTrack =  new SqlTrack( m_collection, rowData );
        sqlTrack->setCapabilityDelegate( createTrackDelegate() );
        TrackPtr track( sqlTrack );
        if( track )
        {
            m_trackMap.insert( id, track );
            m_uidMap.insert( KSharedPtr<SqlTrack>::staticCast( track )->uidUrl(), track );

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
        TrackPtr track = m_trackMap[id];
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
        TrackPtr track = m_uidMap[oldUid];
        m_uidMap.remove( oldUid );
        m_uidMap.insert( newUid, track );
    }
}

TrackPtr
SqlRegistry::getTrackFromUid( const QString &uid )
{
    QMutexLocker locker( &m_trackMutex );
    QMutexLocker locker2( &m_uidMutex );
    if( m_uidMap.contains( uid ) )
        return m_uidMap.value( uid );
    else
    {
        TrackPtr track( SqlTrack::getTrackFromUid( uid, m_collection ) );
        if( track )
        {
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

ArtistPtr
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
                KSharedPtr<SqlArtist>::staticCast( m_artistMap.value( id ) )->updateData( m_collection, id, name );
            return m_artistMap.value( id );
        }

        SqlArtist *sqlArtist = new SqlArtist( m_collection, id, name );
        sqlArtist->setCapabilityDelegate( createArtistDelegate() );
        ArtistPtr artist( sqlArtist );
        m_artistMap.insert( id, artist );
        return artist;
    }
}

GenrePtr
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
                KSharedPtr<SqlGenre>::staticCast( m_genreMap.value( id ) )->updateData( m_collection, id, name );
            return m_genreMap.value( id );
        }

        GenrePtr genre( new SqlGenre( m_collection, id, name ) );
        m_genreMap.insert( id, genre );
        return genre;
    }
}

ComposerPtr
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
                KSharedPtr<SqlComposer>::staticCast( m_composerMap.value( id ) )->updateData( m_collection, id, name );
            return m_composerMap.value( id );
        }

        ComposerPtr composer( new SqlComposer( m_collection, id, name ) );
        m_composerMap.insert( id, composer );
        return composer;
    }
}

YearPtr
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
                KSharedPtr<SqlYear>::staticCast( m_yearMap.value( id ) )->updateData( m_collection, id, name );
            return m_yearMap.value( id );
        }

        YearPtr year( new SqlYear( m_collection, id, name ) );
        m_yearMap.insert( id, year );
        return year;
    }
}

AlbumPtr
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
                KSharedPtr<SqlAlbum>::staticCast( m_albumMap.value( id ) )->updateData( m_collection, id, name, artist );
            return m_albumMap.value( id );
        }

        SqlAlbum *sqlAlbum = new SqlAlbum( m_collection, id, name, artist );
        sqlAlbum->setCapabilityDelegate( createAlbumDelegate() );
        AlbumPtr album( sqlAlbum );
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

        foreachInvalidateCache( AlbumPtr, KSharedPtr<SqlAlbum>, m_albumMap );
        foreachInvalidateCache( ArtistPtr, KSharedPtr<SqlArtist>, m_artistMap );
        foreachInvalidateCache( GenrePtr, KSharedPtr<SqlGenre>, m_genreMap );
        foreachInvalidateCache( ComposerPtr, KSharedPtr<SqlComposer>, m_composerMap );
        foreachInvalidateCache( YearPtr, KSharedPtr<SqlYear>, m_yearMap );

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

        foreachCollectGarbage( TrackId, TrackPtr, 3, m_trackMap )
        foreachCollectGarbage( QString, TrackPtr, 2, m_uidMap )
        //run before artist so that album artist pointers can be garbage collected
        foreachCollectGarbage( int, AlbumPtr, 2, m_albumMap )
        foreachCollectGarbage( int, ArtistPtr, 2, m_artistMap )
        foreachCollectGarbage( int, GenrePtr, 2, m_genreMap )
        foreachCollectGarbage( int, ComposerPtr, 2, m_composerMap )
        foreachCollectGarbage( int, YearPtr, 2, m_yearMap )
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

AlbumCapabilityDelegate*
SqlRegistry::createAlbumDelegate() const
{
    return 0;
}

ArtistCapabilityDelegate*
SqlRegistry::createArtistDelegate() const
{
    return 0;
}

TrackCapabilityDelegate*
SqlRegistry::createTrackDelegate() const
{
    return 0;
}

#include "SqlRegistry.moc"

