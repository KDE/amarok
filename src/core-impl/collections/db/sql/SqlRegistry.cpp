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
#include "core/support/Debug.h"

#include "SqlRegistry.h"
#include "SqlCollection.h"
#include "../ScanManager.h"

#include <QMutableHashIterator>
#include <QMutexLocker>

SqlRegistry::SqlRegistry( Collections::SqlCollection* collection )
    : QObject( 0 )
    , m_collection( collection )
{
    setObjectName( "SqlRegistry" );

    // -- remove unneeded entries from the database.
    // we have to do this now before anyone can hold references
    // to those objects.
    DatabaseUpdater databaseUpdater( m_collection );
    databaseUpdater.deleteAllRedundant( "album" ); // what about cover images in database and disk cache?
    databaseUpdater.deleteAllRedundant( "artist" );
    databaseUpdater.deleteAllRedundant( "genre" );
    databaseUpdater.deleteAllRedundant( "composer" );
    databaseUpdater.deleteAllRedundant( "url" );
    databaseUpdater.deleteAllRedundant( "year" );

    m_timer = new QTimer( this );
    m_timer->setInterval( 30 * 1000 );  //try to clean up every 30 seconds, change if necessary
    m_timer->setSingleShot( false );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( emptyCache() ) );
    m_timer->start();
}

SqlRegistry::~SqlRegistry()
{
    //don't delete m_collection
}

// ------ directory
int
SqlRegistry::getDirectory( const QString &path, uint mtime )
{
    int dirId;
    int deviceId = m_collection->mountPointManager()->getIdForUrl( path );
    QString rdir = m_collection->mountPointManager()->getRelativePath( deviceId, path );

    SqlStorage *storage = m_collection->sqlStorage();

    // - find existing entry
    QString query = QString( "SELECT id, changedate FROM directories "
                             "WHERE  deviceid = %1 AND dir = '%2';" )
                        .arg( QString::number( deviceId ), storage->escape( rdir ) );
    QStringList res = storage->query( query );

    // - create new entry
    if( res.isEmpty() )
    {
        debug() << "SqlRegistry::getDirectory new Directory" << path;
        QString insert = QString( "INSERT INTO directories(deviceid,changedate,dir) "
                                  "VALUES (%1,%2,'%3');" )
                        .arg( QString::number( deviceId ), QString::number( mtime ),
                                storage->escape( rdir ) );
        dirId = storage->insert( insert, "directories" );
    }
    else
    {
        // update old one
        dirId = res[0].toUInt();
        uint oldMtime = res[1].toUInt();
        if( mtime != 0 && oldMtime != mtime )
        {
            QString update = QString( "UPDATE directories SET changedate = %1 "
                                      "WHERE id = %2;" )
                .arg( QString::number( mtime ), res[0] );
        debug() << "SqlRegistry::getDirectory update Directory"<<res[0]<<"from" << oldMtime << "to" << mtime;
        debug() << update;
            storage->query( update );
        }
    }
    return dirId;
}

// ------ track

Meta::TrackPtr
SqlRegistry::getTrack( int trackId )
{
    QMutexLocker locker( &m_trackMutex );

    QString query = "SELECT %1 FROM urls %2 "
        "WHERE tracks.id = %3;";
    query = query.arg( Meta::SqlTrack::getTrackReturnValues(),
                       Meta::SqlTrack::getTrackJoinConditions(),
                       QString::number( trackId ) );
    QStringList rowData = m_collection->sqlStorage()->query( query );
    if( rowData.isEmpty() )
        return Meta::TrackPtr();

    TrackId id( rowData[Meta::SqlTrack::returnIndex_urlDeviceId].toInt(),
                rowData[Meta::SqlTrack::returnIndex_urlRPath] );
    QString uid = rowData[Meta::SqlTrack::returnIndex_urlUid];

    if( m_trackMap.contains( id ) )
        return m_trackMap.value( id );
    else if( m_uidMap.contains( uid ) )
        return m_uidMap.value( uid );
    else
    {
        Meta::SqlTrack *sqlTrack = new Meta::SqlTrack( m_collection, rowData );
        Meta::TrackPtr trackPtr( sqlTrack );

        m_trackMap.insert( id, trackPtr );
        m_uidMap.insert( sqlTrack->uidUrl(), trackPtr );
        return trackPtr;
    }
}

Meta::TrackPtr
SqlRegistry::getTrack( const QString &path )
{
    int deviceId = m_collection->mountPointManager()->getIdForUrl( path );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, path );

    TrackId id(deviceId, rpath);
    if( m_trackMap.contains( id ) )
        return m_trackMap.value( id );
    else
    {
        // -- first a fast lookup if we really have such a track
        QString query = "SELECT id FROM urls "
            "WHERE urls.deviceid = %1 AND urls.rpath = '%2';";
        query = query.arg( QString::number( deviceId ),
                           m_collection->sqlStorage()->escape( rpath ) );
        QStringList result = m_collection->sqlStorage()->query( query );

        // -- now either create the track or get it from the database
        Meta::SqlTrack *sqlTrack;

        if( result.isEmpty() )
            return Meta::TrackPtr();

        Q_ASSERT( result.count() == 1 ); // path is unique

        query = "SELECT %1 FROM urls %2 "
            "WHERE urls.id = %3;";
        query = query.arg( Meta::SqlTrack::getTrackReturnValues(),
                           Meta::SqlTrack::getTrackJoinConditions(),
                           result[0] );
        result = m_collection->sqlStorage()->query( query );

        sqlTrack = new Meta::SqlTrack( m_collection, result );

        Meta::TrackPtr trackPtr( sqlTrack );
        m_trackMap.insert( id, trackPtr );
        m_uidMap.insert( sqlTrack->uidUrl(), trackPtr );
        return trackPtr;
    }
}


Meta::TrackPtr
SqlRegistry::getTrack( int deviceId, const QString &rpath, int directoryId, const QString &uidUrl )
{
    TrackId id(deviceId, rpath);
    QMutexLocker locker( &m_trackMutex );
    if( m_trackMap.contains( id ) )
        return m_trackMap.value( id );
    else
    {
        // -- first a fast lookup if we really have such a track
        QString query = "SELECT id FROM urls "
            "WHERE urls.deviceid = %1 AND urls.rpath = '%2';";
        query = query.arg( QString::number( deviceId ),
                           m_collection->sqlStorage()->escape( rpath ) );
        QStringList result = m_collection->sqlStorage()->query( query );

        // -- now either create the track or get it from the database
        Meta::SqlTrack *sqlTrack;

        if( result.isEmpty() )
        {
            // ok. we have to create a new track

            sqlTrack = new Meta::SqlTrack( m_collection, deviceId, rpath, directoryId, uidUrl );
        }
        else
        {
            Q_ASSERT( result.count() == 1 ); // path is unique

            query = "SELECT %1 FROM urls %2 "
                "WHERE urls.id = %3;";
            query = query.arg( Meta::SqlTrack::getTrackReturnValues(),
                               Meta::SqlTrack::getTrackJoinConditions(),
                               result[0] );
            result = m_collection->sqlStorage()->query( query );

            sqlTrack = new Meta::SqlTrack( m_collection, result );
        }

        Meta::TrackPtr trackPtr( sqlTrack );
        m_trackMap.insert( id, trackPtr );
        m_uidMap.insert( sqlTrack->uidUrl(), trackPtr );
        return trackPtr;
    }
}

Meta::TrackPtr
SqlRegistry::getTrack( int trackId, const QStringList &rowData )
{
    Q_ASSERT( trackId == rowData[Meta::SqlTrack::returnIndex_trackId].toInt() );
    Q_UNUSED( trackId );

    TrackId id( rowData[Meta::SqlTrack::returnIndex_urlDeviceId].toInt(),
                rowData[Meta::SqlTrack::returnIndex_urlRPath] );
    QString uid = rowData[Meta::SqlTrack::returnIndex_urlUid];

    QMutexLocker locker( &m_trackMutex );
    if( m_trackMap.contains( id ) )
        return m_trackMap.value( id );
    else if( m_uidMap.contains( uid ) )
        return m_uidMap.value( uid );
    else
    {
        Meta::SqlTrack *sqlTrack =  new Meta::SqlTrack( m_collection, rowData );
        Meta::TrackPtr track( sqlTrack );

        m_trackMap.insert( id, track );
        m_uidMap.insert( KSharedPtr<Meta::SqlTrack>::staticCast( track )->uidUrl(), track );
        return track;
    }
}

bool
SqlRegistry::updateCachedUrl( const QString &oldUrl, const QString &newUrl )
{
    QMutexLocker locker( &m_trackMutex );
    int deviceId = m_collection->mountPointManager()->getIdForUrl( oldUrl );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, oldUrl );
    TrackId oldId(deviceId, rpath);

    int newdeviceId = m_collection->mountPointManager()->getIdForUrl( newUrl );
    QString newRpath = m_collection->mountPointManager()->getRelativePath( newdeviceId, newUrl );
    TrackId newId( newdeviceId, newRpath );

    if( m_trackMap.contains( newId ) )
        warning() << "updating path to an already existing path.";
    else if( !m_trackMap.contains( oldId ) )
        warning() << "updating path from a non existing path.";
    else
    {
        Meta::TrackPtr track = m_trackMap.take( oldId );
        m_trackMap.insert( newId, track );
        return true;
    }
    return false;
}

bool
SqlRegistry::updateCachedUid( const QString &oldUid, const QString &newUid )
{
    QMutexLocker locker( &m_trackMutex );
    if( m_uidMap.contains( newUid ) )
        warning() << "updating uid to an already existing uid.";
    else if( !m_uidMap.contains( oldUid ) )
        warning() << "updating uid from a non existing uid.";
    else
    {
        Meta::TrackPtr track = m_uidMap.take(oldUid);
        m_uidMap.insert( newUid, track );
        return true;
    }
    return false;
}

Meta::TrackPtr
SqlRegistry::getTrackFromUid( const QString &uid )
{
    QMutexLocker locker( &m_trackMutex );
    if( m_uidMap.contains( uid ) )
        return m_uidMap.value( uid );
    else
    {
        // -- first check for the unique id (is faster)
        QString query = "SELECT (id) FROM urls WHERE urls.uniqueid = '%3';";
        query = query.arg( m_collection->sqlStorage()->escape( uid ) );
        QStringList result = m_collection->sqlStorage()->query( query );
        if( result.isEmpty() )
            return Meta::TrackPtr();
        if( result.count() > 1 )
            warning() << "More than one unique track id.";

        // -- then get all the track info
        query = "SELECT %1 FROM urls %2 "
            "WHERE urls.id = %3;";
        query = query.arg( Meta::SqlTrack::getTrackReturnValues(),
                           Meta::SqlTrack::getTrackJoinConditions(),
                           result[0] );
        result = m_collection->sqlStorage()->query( query );
        if( result.isEmpty() )
            return Meta::TrackPtr();

        Meta::SqlTrack *sqlTrack = new Meta::SqlTrack( m_collection, result );
        Meta::TrackPtr trackPtr( sqlTrack );

        int deviceid = m_collection->mountPointManager()->getIdForUrl( trackPtr->playableUrl().path() );
        QString rpath = m_collection->mountPointManager()->getRelativePath( deviceid, trackPtr->playableUrl().path() );
        TrackId id(deviceid, rpath);
        m_trackMap.insert( id, trackPtr );
        m_uidMap.insert( uid, trackPtr );
        return trackPtr;
    }
}

void
SqlRegistry::deleteTrack( int trackId )
{
    QMutexLocker locker( &m_trackMutex );

    // -- get the uid
    QString query = "SELECT %1 FROM urls %2 "
        "WHERE tracks.id = %3;";
    query = query.arg( "urls.id, urls.uniqueid",
                       Meta::SqlTrack::getTrackJoinConditions(),
                       QString::number( trackId ) );

    QStringList result = m_collection->sqlStorage()->query( query );
    if( result.isEmpty() )
    {
        warning() << "Didn't find the track to delete.";
        return;
    }
    else if( result.count() > 2 )
        warning() << "More than one unique track id.";

    int urlId = result.at(0).toInt();
    QString uid = result.at(1);

    // --- delete the track from memory and database
    if( m_uidMap.contains( uid ) )
    {
        // -- remove from hashes
        Meta::TrackPtr track = m_uidMap.take( uid );
        Meta::SqlTrack *sqlTrack = static_cast<Meta::SqlTrack*>( track.data() );

        int deviceId = m_collection->mountPointManager()->getIdForUrl( sqlTrack->playableUrl().path() );
        QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, sqlTrack->playableUrl().path() );
        TrackId id(deviceId, rpath);
        m_trackMap.remove( id );

        locker.unlock(); // prevent deadlock
        sqlTrack->remove();
        return;
    }

    // --- delete the track only from database
    else
    {
        QString query;

        // -- remove from db
        query = QString( "DELETE FROM tracks where url = %1;" ).arg( urlId );
        m_collection->sqlStorage()->query( query );
        query = QString( "DELETE FROM urls WHERE id = '%1';").arg( urlId );
        m_collection->sqlStorage()->query( query );
    }
}

// -------- artist

Meta::ArtistPtr
SqlRegistry::getArtist( const QString &name )
{
    QMutexLocker locker( &m_artistMutex );

    int id;

    QString query = QString( "SELECT id FROM artists WHERE name = '%1';" ).arg( m_collection->sqlStorage()->escape( name ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO artists( name ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( name ) );
        id = m_collection->sqlStorage()->insert( insert, "artists" );
    }
    else
    {
        id = res[0].toInt();
        if( m_artistMap.contains( id ) )
            return m_artistMap.value( id );
    }

    Meta::ArtistPtr artist( new Meta::SqlArtist( m_collection, id, name ) );
    m_artistMap.insert( id, artist );
    return artist;
}

Meta::ArtistPtr
SqlRegistry::getArtist( int id )
{
    QMutexLocker locker( &m_artistMutex );

    if( m_artistMap.contains( id ) )
        return m_artistMap.value( id );

    QString query = QString( "SELECT name FROM artists WHERE id = %1;" ).arg( id );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
        return Meta::ArtistPtr();

    QString name = res[0];
    Meta::ArtistPtr artist( new Meta::SqlArtist( m_collection, id, name ) );
    m_artistMap.insert( id, artist );
    return artist;
}

Meta::ArtistPtr
SqlRegistry::getArtist( int id, const QString &name )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_artistMutex );

    if( m_artistMap.contains( id ) )
        return m_artistMap.value( id );

    Meta::ArtistPtr artist( new Meta::SqlArtist( m_collection, id, name ) );
    m_artistMap.insert( id, artist );
    return artist;
}

// -------- genre

Meta::GenrePtr
SqlRegistry::getGenre( const QString &name )
{
    QMutexLocker locker( &m_genreMutex );

    int id;

    QString query = QString( "SELECT id FROM genres WHERE name = '%1';" ).arg( m_collection->sqlStorage()->escape( name ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO genres( name ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( name ) );
        id = m_collection->sqlStorage()->insert( insert, "genres" );
    }
    else
    {
        id = res[0].toInt();
        if( m_genreMap.contains( id ) )
            return m_genreMap.value( id );
    }

    Meta::GenrePtr genre( new Meta::SqlGenre( m_collection, id, name ) );
    m_genreMap.insert( id, genre );
    return genre;
}

Meta::GenrePtr
SqlRegistry::getGenre( int id )
{
    QMutexLocker locker( &m_genreMutex );

    if( m_genreMap.contains( id ) )
        return m_genreMap.value( id );

    QString query = QString( "SELECT name FROM genres WHERE id = '%1';" ).arg( id );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
        return Meta::GenrePtr();

    QString name = res[0];
    Meta::GenrePtr genre( new Meta::SqlGenre( m_collection, id, name ) );
    m_genreMap.insert( id, genre );
    return genre;
}

Meta::GenrePtr
SqlRegistry::getGenre( int id, const QString &name )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_genreMutex );

    if( m_genreMap.contains( id ) )
        return m_genreMap.value( id );

    Meta::GenrePtr genre( new Meta::SqlGenre( m_collection, id, name ) );
    m_genreMap.insert( id, genre );
    return genre;
}

// -------- composer

Meta::ComposerPtr
SqlRegistry::getComposer( const QString &name )
{
    QMutexLocker locker( &m_composerMutex );

    int id;

    QString query = QString( "SELECT id FROM composers WHERE name = '%1';" ).arg( m_collection->sqlStorage()->escape( name ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO composers( name ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( name ) );
        id = m_collection->sqlStorage()->insert( insert, "composers" );
    }
    else
    {
        id = res[0].toInt();
        if( m_composerMap.contains( id ) )
            return m_composerMap.value( id );
    }

    Meta::ComposerPtr composer( new Meta::SqlComposer( m_collection, id, name ) );
    m_composerMap.insert( id, composer );
    return composer;
}

Meta::ComposerPtr
SqlRegistry::getComposer( int id )
{
    if( id <= 0 )
        return Meta::ComposerPtr();

    QMutexLocker locker( &m_composerMutex );

    if( m_composerMap.contains( id ) )
        return m_composerMap.value( id );

    QString query = QString( "SELECT name FROM composers WHERE id = '%1';" ).arg( id );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
        return Meta::ComposerPtr();

    QString name = res[0];
    Meta::ComposerPtr composer( new Meta::SqlComposer( m_collection, id, name ) );
    m_composerMap.insert( id, composer );
    return composer;
}

Meta::ComposerPtr
SqlRegistry::getComposer( int id, const QString &name )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_composerMutex );

    if( m_composerMap.contains( id ) )
        return m_composerMap.value( id );

    Meta::ComposerPtr composer( new Meta::SqlComposer( m_collection, id, name ) );
    m_composerMap.insert( id, composer );
    return composer;
}

// -------- year

Meta::YearPtr
SqlRegistry::getYear( int year, int yearId )
{
    QMutexLocker locker( &m_yearMutex );

    // don't know the id yet
    if( yearId <= 0 )
    {
        QString query = QString( "SELECT id FROM years WHERE name = '%1';" ).arg( QString::number( year ) );
        QStringList res = m_collection->sqlStorage()->query( query );
        if( res.isEmpty() )
        {
            QString insert = QString( "INSERT INTO years( name ) VALUES ('%1');" ).arg( QString::number( year ) );
            yearId = m_collection->sqlStorage()->insert( insert, "years" );
        }
        else
        {
            yearId = res[0].toInt();
        }
    }
    if( m_yearMap.contains( yearId ) )
        return m_yearMap.value( yearId );

    Meta::YearPtr yearPtr( new Meta::SqlYear( m_collection, yearId, year ) );
    m_yearMap.insert( yearId, yearPtr );
    return yearPtr;
}

// -------- album

Meta::AlbumPtr
SqlRegistry::getAlbum( const QString &name, const QString &artist )
{
    QString albumArtist( artist );
    if( name.isEmpty() ) // the empty album is special. all singles are collected here
        albumArtist.clear();

    QMutexLocker locker( &m_albumMutex );
    int albumId = -1;
    int artistId = -1;

    QString query = QString( "SELECT id FROM albums WHERE name = '%1' AND " ).arg( m_collection->sqlStorage()->escape( name ) );

    if( albumArtist.isEmpty() )
    {
        query += QString( "artist IS NULL" );
    }
    else
    {
        Meta::ArtistPtr artistPtr = getArtist( albumArtist );
        Meta::SqlArtist *sqlArtist = static_cast<Meta::SqlArtist*>(artistPtr.data());
        artistId = sqlArtist->id();

        query += QString( "artist=%1" ).arg( artistId );
    }

    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        // ok. have to create a new album
        QString insert = QString( "INSERT INTO albums( name, artist ) VALUES ('%1',%2);" ).
            arg( m_collection->sqlStorage()->escape( name ),
                 artistId > 0 ? QString::number( artistId ) : "NULL" );
        albumId = m_collection->sqlStorage()->insert( insert, "albums" );
    }
    else
    {
        albumId = res[0].toInt();
        if( m_albumMap.contains( albumId ) )
            return m_albumMap.value( albumId );
    }

    Meta::SqlAlbum *sqlAlbum = new Meta::SqlAlbum( m_collection, albumId, name, artistId );
    Meta::AlbumPtr album( sqlAlbum );
    m_albumMap.insert( albumId, album );
    return album;
}

Meta::AlbumPtr
SqlRegistry::getAlbum( int id )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_albumMutex );

    if( m_albumMap.contains( id ) )
        return m_albumMap.value( id );

    QString query = QString( "SELECT name, artist FROM albums WHERE id = %1" ).arg( id );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
        return Meta::AlbumPtr(); // someone messed up

    QString name = res[0];
    int artistId = res[1].toInt();

    Meta::SqlAlbum *sqlAlbum = new Meta::SqlAlbum( m_collection, id, name, artistId );
    Meta::AlbumPtr album( sqlAlbum );
    m_albumMap.insert( id, album );
    return album;
}

Meta::AlbumPtr
SqlRegistry::getAlbum( int id, const QString &name, int artistId )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_albumMutex );

    if( m_albumMap.contains( id ) )
        return m_albumMap.value( id );

    Meta::SqlAlbum *sqlAlbum = new Meta::SqlAlbum( m_collection, id, name, artistId );
    Meta::AlbumPtr album( sqlAlbum );
    m_albumMap.insert( id, album );
    return album;
}

// ------------ label

Meta::LabelPtr
SqlRegistry::getLabel( const QString &label )
{
    QMutexLocker locker( &m_labelMutex );

    int id;

    QString query = QString( "SELECT id FROM labels WHERE label = '%1';" ).arg( m_collection->sqlStorage()->escape( label ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO labels( label ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( label ) );
        id = m_collection->sqlStorage()->insert( insert, "albums" );
    }
    else
    {
        id = res[0].toInt();
        if( m_labelMap.contains( id ) )
            return m_labelMap.value( id );
    }

    Meta::LabelPtr labelPtr( new Meta::SqlLabel( m_collection, id, label ) );
    m_labelMap.insert( id, labelPtr );
    return labelPtr;
}

Meta::LabelPtr
SqlRegistry::getLabel( int id )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_labelMutex );

    if( m_labelMap.contains( id ) )
        return m_labelMap.value( id );

    QString query = QString( "SELECT label FROM labels WHERE id = '%1';" ).arg( id );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
        return Meta::LabelPtr();

    QString label = res[0];
    Meta::LabelPtr labelPtr( new Meta::SqlLabel( m_collection, id, label ) );
    m_labelMap.insert( id, labelPtr );
    return labelPtr;
}

Meta::LabelPtr
SqlRegistry::getLabel( int id, const QString &label )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_labelMutex );

    if( m_labelMap.contains( id ) )
        return m_labelMap.value( id );

    Meta::LabelPtr labelPtr( new Meta::SqlLabel( m_collection, id, label ) );
    m_labelMap.insert( id, labelPtr );
    return labelPtr;
}

void
SqlRegistry::emptyCache()
{
    if( m_collection->scanManager() && m_collection->scanManager()->isRunning() )
        return; // don't clean the cache if a scan is done

    bool hasTrack, hasAlbum, hasArtist, hasYear, hasGenre, hasComposer, hasLabel;
    hasTrack = hasAlbum = hasArtist = hasYear = hasGenre = hasComposer = hasLabel = false;

    //try to avoid possible deadlocks by aborting when we can't get all locks
    if ( ( hasTrack = m_trackMutex.tryLock() )
         && ( hasAlbum = m_albumMutex.tryLock() )
         && ( hasArtist = m_artistMutex.tryLock() )
         && ( hasYear = m_yearMutex.tryLock() )
         && ( hasGenre = m_genreMutex.tryLock() )
         && ( hasComposer = m_composerMutex.tryLock() )
         && ( hasLabel = m_labelMutex.tryLock() ) )
    {
        debug() << "SqlRegistry::emptyCache is running";

        QString query = QString( "SELECT COUNT(*) FROM albums;" );
        QStringList res = m_collection->sqlStorage()->query( query );
        debug() << "    albums:" << m_albumMap.count() << "of" << res << "cached.";

        query = QString( "SELECT COUNT(*) FROM tracks;" );
        res = m_collection->sqlStorage()->query( query );
        debug() << "     tracks:" << m_trackMap.count() << "of" << res << "cached.";

        query = QString( "SELECT COUNT(*) FROM artists;" );
        res = m_collection->sqlStorage()->query( query );
        debug() << "    artists:" << m_artistMap.count() << "of" << res << "cached.";

        query = QString( "SELECT COUNT(*) FROM genres;" );
        res = m_collection->sqlStorage()->query( query );
        debug() << "     genres:" << m_genreMap.count() << "of" << res << "cached.";

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
        foreachInvalidateCache( Meta::LabelPtr, KSharedPtr<Meta::SqlLabel>, m_labelMap );
        #undef foreachInvalidateCache

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

        foreachCollectGarbage( TrackId, Meta::TrackPtr, 3, m_trackMap );
        foreachCollectGarbage( QString, Meta::TrackPtr, 2, m_uidMap );
        //run before artist so that album artist pointers can be garbage collected
        foreachCollectGarbage( int, Meta::AlbumPtr, 2, m_albumMap );
        foreachCollectGarbage( int, Meta::ArtistPtr, 2, m_artistMap );
        foreachCollectGarbage( int, Meta::GenrePtr, 2, m_genreMap );
        foreachCollectGarbage( int, Meta::ComposerPtr, 2, m_composerMap );
        foreachCollectGarbage( int, Meta::YearPtr, 2, m_yearMap );
        foreachCollectGarbage( int, Meta::LabelPtr, 2, m_labelMap );
        #undef foreachCollectGarbage

        debug() << "--- after run: ---";

        query = QString( "SELECT COUNT(*) FROM albums;" );
        res = m_collection->sqlStorage()->query( query );
        debug() << "    albums:" << m_albumMap.count() << "of" << res << "cached.";

        query = QString( "SELECT COUNT(*) FROM tracks;" );
        res = m_collection->sqlStorage()->query( query );
        debug() << "     tracks:" << m_trackMap.count() << "of" << res << "cached.";

        query = QString( "SELECT COUNT(*) FROM artists;" );
        res = m_collection->sqlStorage()->query( query );
        debug() << "    artists:" << m_artistMap.count() << "of" << res << "cached.";

        query = QString( "SELECT COUNT(*) FROM genres;" );
        res = m_collection->sqlStorage()->query( query );
        debug() << "     genres:" << m_genreMap.count() << "of" << res << "cached.";
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
    if( hasLabel ) m_labelMutex.unlock();
}

#include "SqlRegistry.moc"

