/* This file is part of the KDE project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define DEBUG_PREFIX "SqlRegistry"

#include "sqlregistry.h"

#include "debug.h"

#include "mountpointmanager.h"

#include <QHashIterator>
#include <QMutableHashIterator>
#include <QMutexLocker>

SqlRegistry::SqlRegistry( SqlCollection* collection )
    : QObject( 0 )
    , m_collection( collection )
{
    setObjectName( "SqlRegistry" );

    m_timer = new QTimer( this );
    m_timer->setInterval( 60000 );  //try to clean up every 60 seconds, change if necessary
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
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );
    TrackId id(deviceid, rpath);
    QMutexLocker locker( &m_trackMutex );
    if( m_trackMap.contains( id ) )
        return m_trackMap.value( id );
    else
    {
        TrackPtr track = SqlTrack::getTrack( deviceid, rpath, m_collection );
        if( track )
            m_trackMap.insert( id, track );
        return track;
    }
}

TrackPtr
SqlRegistry::getTrack( const QStringList &rowData )
{
    TrackId id( rowData[0].toInt(), rowData[1] );
    QMutexLocker locker( &m_trackMutex );
    if( m_trackMap.contains( id ) )
        return m_trackMap.value( id );
    else
    {
        TrackPtr track( new SqlTrack( m_collection, rowData ) );
        m_trackMap.insert( id, track );
        return track;
    }
}

ArtistPtr
SqlRegistry::getArtist( const QString &name, int id )
{
    QMutexLocker locker( &m_artistMutex );
    if( m_artistMap.contains( name ) )
        return m_artistMap.value( name );
    else
    {
        ArtistPtr artist( new SqlArtist( m_collection, id, name ) );
        m_artistMap.insert( name, artist );
        return artist;
    }
}

GenrePtr
SqlRegistry::getGenre( const QString &name, int id )
{
    QMutexLocker locker( &m_genreMutex );
    if( m_genreMap.contains( name ) )
        return m_genreMap.value( name );
    else
    {
        GenrePtr genre( new SqlGenre( m_collection, id, name ) );
        m_genreMap.insert( name, genre );
        return genre;
    }
}

ComposerPtr
SqlRegistry::getComposer( const QString &name, int id )
{
    QMutexLocker locker( &m_composerMutex );
    if( m_composerMap.contains( name ) )
        return m_composerMap.value( name );
    else
    {
        ComposerPtr composer( new SqlComposer( m_collection, id, name ) );
        m_composerMap.insert( name, composer );
        return composer;
    }
}

YearPtr
SqlRegistry::getYear( const QString &name, int id )
{
    QMutexLocker locker( &m_yearMutex );
    if( m_yearMap.contains( name ) )
        return m_yearMap.value( name );
    else
    {
        YearPtr year( new SqlYear( m_collection, id, name ) );
        m_yearMap.insert( name, year );
        return year;
    }
}

AlbumPtr
SqlRegistry::getAlbum( const QString &name, int id )
{
    QMutexLocker locker( &m_albumMutex );
    if( m_albumMap.contains( name ) )
        return m_albumMap.value( name );
    else
    {
        AlbumPtr album( new SqlAlbum( m_collection, id, name ) );
        m_albumMap.insert( name, album );
        return album;
    }
}

void
SqlRegistry::emptyCache()
{
    DEBUG_BLOCK
    bool hasTrack, hasAlbum, hasArtist, hasYear, hasGenre, hasComposer;
    hasTrack = hasAlbum = hasArtist = hasYear = hasGenre = hasComposer = false;

    //try to avoid possible deadlocks by aborting when we can't get all locks
    if ( ( hasTrack = m_trackMutex.tryLock() )
         && ( hasAlbum = m_albumMutex.tryLock() )
         && ( hasArtist = m_artistMutex.tryLock() )
         && ( hasYear = m_yearMutex.tryLock() )
         && ( hasGenre = m_genreMutex.tryLock() )
         && ( hasComposer = m_composerMutex.tryLock() ) )
    {
        //this very simple garbage collector doesn't handle cyclic object graphs
        //so care has to be taken to make sure that we are not dealing with a cyclic graph
        //by invalidating the tracks cache on all objects
        #define foreachInvalidateCache( Type, RealType, x ) \
        for( QMutableHashIterator<QString,Type > iter(x); iter.hasNext(); ) \
            RealType::staticCast( iter.next().value() )->invalidateCache()

        foreachInvalidateCache( AlbumPtr, KSharedPtr<SqlAlbum>, m_albumMap );
        foreachInvalidateCache( ArtistPtr, KSharedPtr<SqlArtist>, m_artistMap );
        foreachInvalidateCache( GenrePtr, KSharedPtr<SqlGenre>, m_genreMap );
        foreachInvalidateCache( ComposerPtr, KSharedPtr<SqlComposer>, m_composerMap );
        foreachInvalidateCache( YearPtr, KSharedPtr<SqlYear>, m_yearMap );

        //elem.count() == 2 is correct because elem is one pointer to the object
        //and the other is stored in the hash map
        #define foreachCollectGarbage( Key, Type, x ) \
        for( QMutableHashIterator<Key,Type > iter(x); iter.hasNext(); ) \
        { \
            Type elem = iter.next().value(); \
            if( elem.count() == 2 ) \
                iter.remove(); \
        }

        foreachCollectGarbage( TrackId, TrackPtr, m_trackMap )
        //run before artist so that album artist pointers can be garbage collected
        foreachCollectGarbage( QString, AlbumPtr, m_albumMap )
        foreachCollectGarbage( QString, ArtistPtr, m_artistMap )
        foreachCollectGarbage( QString, GenrePtr, m_genreMap )
        foreachCollectGarbage( QString, ComposerPtr, m_composerMap )
        foreachCollectGarbage( QString, YearPtr, m_yearMap )
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
}

#include "sqlregistry.moc"
