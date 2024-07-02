/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "DatabaseUpdater.h"
#include "SqlRegistry_p.h"
#include "SqlCollection.h"
#include "core/support/Debug.h"
#include "core-impl/collections/db/MountPointManager.h"
#include "scanner/GenericScanManager.h"

#include <QMutableHashIterator>
#include <QMutexLocker>

SqlRegistry::SqlRegistry( Collections::SqlCollection* collection )
    : QObject( nullptr )
    , m_collection( collection )
    , m_blockDatabaseUpdateCount( 0 )
    , m_collectionChanged( false )
{
    DEBUG_BLOCK
    setObjectName( QStringLiteral("SqlRegistry") );

    // -- remove unneeded entries from the database.
    // we have to do this now before anyone can hold references
    // to those objects.
    DatabaseUpdater databaseUpdater( m_collection );

    // url entries without associated directory just stick around and cannot be processed
    // by SqlScanResultProcessor. Delete them before checking tracks
    databaseUpdater.deleteOrphanedByDirectory( QStringLiteral("urls") );

    // tracks with no associated url entry are useless, just a bunch of medatada with
    // nothing to associate them to; remove those first
    databaseUpdater.deleteOrphanedByUrl( QStringLiteral("tracks") );

    databaseUpdater.deleteAllRedundant( QStringLiteral("album") ); // what about cover images in database and disk cache?
    databaseUpdater.deleteAllRedundant( QStringLiteral("artist") );
    databaseUpdater.deleteAllRedundant( QStringLiteral("genre") );
    databaseUpdater.deleteAllRedundant( QStringLiteral("composer") );
    databaseUpdater.deleteAllRedundant( QStringLiteral("url") );
    databaseUpdater.deleteAllRedundant( QStringLiteral("year") );

    databaseUpdater.deleteOrphanedByUrl( QStringLiteral("lyrics") );
    databaseUpdater.deleteOrphanedByUrl( QStringLiteral("statistics") );
    databaseUpdater.deleteOrphanedByUrl( QStringLiteral("urls_labels") );

    m_timer = new QTimer( this );
    m_timer->setInterval( 30 * 1000 );  //try to clean up every 30 seconds, change if necessary
    m_timer->setSingleShot( false );
    connect( m_timer, &QTimer::timeout, this, &SqlRegistry::emptyCache );
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
    int deviceId = m_collection->mountPointManager()->getIdForUrl( QUrl::fromLocalFile(path) );
    QString rdir = m_collection->mountPointManager()->getRelativePath( deviceId, path );

    auto storage = m_collection->sqlStorage();

    // - find existing entry
    QString query = QStringLiteral( "SELECT id, changedate FROM directories "
                             "WHERE  deviceid = %1 AND dir = '%2';" )
                        .arg( QString::number( deviceId ), storage->escape( rdir ) );
    QStringList res = storage->query( query );

    // - create new entry
    if( res.isEmpty() )
    {
        debug() << "SqlRegistry::getDirectory(): new directory" << path;
        QString insert = QStringLiteral( "INSERT INTO directories(deviceid,changedate,dir) "
                                  "VALUES (%1,%2,'%3');" )
                        .arg( QString::number( deviceId ), QString::number( mtime ),
                                storage->escape( rdir ) );
        dirId = storage->insert( insert, QStringLiteral("directories") );
        m_collectionChanged = true;
    }
    else
    {
        // update old one
        dirId = res[0].toUInt();
        uint oldMtime = res[1].toUInt();
        if( oldMtime != mtime )
        {
            QString update = QStringLiteral( "UPDATE directories SET changedate = %1 "
                                      "WHERE id = %2;" )
                .arg( QString::number( mtime ), res[0] );
            debug() << "SqlRegistry::getDirectory(): update directory" << path << "(id" <<
                    res[0] << ") from" << oldMtime << "to" << mtime << "UNIX time";
            storage->query( update );
        }
    }
    return dirId;
}

// ------ track

Meta::TrackPtr
SqlRegistry::getTrack( int urlId )
{
    QString query = QStringLiteral("SELECT %1 FROM urls %2 WHERE urls.id = %3");
    query = query.arg( Meta::SqlTrack::getTrackReturnValues(),
                       Meta::SqlTrack::getTrackJoinConditions(),
                       QString::number( urlId ) );
    QStringList rowData = m_collection->sqlStorage()->query( query );
    if( rowData.isEmpty() )
        return Meta::TrackPtr();

    TrackPath id( rowData[Meta::SqlTrack::returnIndex_urlDeviceId].toInt(),
                rowData[Meta::SqlTrack::returnIndex_urlRPath] );
    QString uid = rowData[Meta::SqlTrack::returnIndex_urlUid];

    QMutexLocker locker( &m_trackMutex );
    if( m_trackMap.contains( id ) )
    {
        Meta::SqlTrackPtr track = Meta::SqlTrackPtr::staticCast( m_trackMap[ id ] );
        // yes, it may happen that we get a different track in corner cases, see bug 323156
        if( track->urlId() == urlId )
            return Meta::TrackPtr::staticCast( track );
        warning() << Q_FUNC_INFO << "track with (deviceId, rpath)" << id << "found in"
                  << "m_trackMap, but it had different urlId (" << track->urlId() << ")"
                  << "than requested (" << urlId << "). This may happen in corner-cases.";
    }
    if( m_uidMap.contains( uid ) )
    {
        Meta::SqlTrackPtr track = Meta::SqlTrackPtr::staticCast( m_uidMap[ uid ] );
        // yes, it may happen that we get a different track in corner cases, see bug 323156
        if( track->urlId() == urlId )
            return Meta::TrackPtr::staticCast( track );
        warning() << Q_FUNC_INFO << "track with uid" << uid << "found in m_uidMap, but it"
                  << "had different urlId (" << track->urlId() << ") than requested ("
                  << urlId << "). This may happen in corner-cases.";
    }

    Meta::SqlTrack *sqlTrack = new Meta::SqlTrack( m_collection, rowData );
    Meta::TrackPtr trackPtr( sqlTrack );

    m_trackMap.insert( id, trackPtr );
    m_uidMap.insert( sqlTrack->uidUrl(), trackPtr );
    return trackPtr;
}

Meta::TrackPtr
SqlRegistry::getTrack( const QString &path )
{
    int deviceId = m_collection->mountPointManager()->getIdForUrl( QUrl::fromLocalFile(path) );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, path );
    TrackPath id( deviceId, rpath );

    QMutexLocker locker( &m_trackMutex );
    if( m_trackMap.contains( id ) )
        return m_trackMap.value( id );
    else
    {
        QString query;
        QStringList result;

        query = QStringLiteral("SELECT %1 FROM urls %2 "
            "WHERE urls.deviceid = %3 AND urls.rpath = '%4';");
        query = query.arg( Meta::SqlTrack::getTrackReturnValues(),
                           Meta::SqlTrack::getTrackJoinConditions(),
                           QString::number( deviceId ),
                           m_collection->sqlStorage()->escape( rpath ) );
        result = m_collection->sqlStorage()->query( query );
        if( result.isEmpty() )
            return Meta::TrackPtr();

        Meta::SqlTrack *sqlTrack = new Meta::SqlTrack( m_collection, result );

        Meta::TrackPtr trackPtr( sqlTrack );
        m_trackMap.insert( id, trackPtr );
        m_uidMap.insert( sqlTrack->uidUrl(), trackPtr );
        return trackPtr;
    }
}


Meta::TrackPtr
SqlRegistry::getTrack( int deviceId, const QString &rpath, int directoryId, const QString &uidUrl )
{
    TrackPath id( deviceId, rpath );

    QMutexLocker locker( &m_trackMutex );
    if( m_trackMap.contains( id ) )
        return m_trackMap.value( id );
    else
    {
        QString query;
        QStringList result;
        Meta::SqlTrack *sqlTrack = nullptr;

        // -- get it from the database
        query = QStringLiteral("SELECT %1 FROM urls %2 "
            "WHERE urls.deviceid = %3 AND urls.rpath = '%4';");
        query = query.arg( Meta::SqlTrack::getTrackReturnValues(),
                           Meta::SqlTrack::getTrackJoinConditions(),
                           QString::number( deviceId ),
                           m_collection->sqlStorage()->escape( rpath ) );
        result = m_collection->sqlStorage()->query( query );

        if( !result.isEmpty() )
            sqlTrack = new Meta::SqlTrack( m_collection, result );

        // -- we have to create a new track
        if( !sqlTrack )
            sqlTrack = new Meta::SqlTrack( m_collection, deviceId, rpath, directoryId, uidUrl );

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

    TrackPath path( rowData[Meta::SqlTrack::returnIndex_urlDeviceId].toInt(),
                    rowData[Meta::SqlTrack::returnIndex_urlRPath] );
    QString uid = rowData[Meta::SqlTrack::returnIndex_urlUid];

    QMutexLocker locker( &m_trackMutex );
    if( m_trackMap.contains( path ) )
        return m_trackMap.value( path );
    else if( m_uidMap.contains( uid ) )
        return m_uidMap.value( uid );
    else
    {
        Meta::SqlTrack *sqlTrack =  new Meta::SqlTrack( m_collection, rowData );
        Meta::TrackPtr track( sqlTrack );

        m_trackMap.insert( path, track );
        m_uidMap.insert( AmarokSharedPointer<Meta::SqlTrack>::staticCast( track )->uidUrl(), track );
        return track;
    }
}

bool
SqlRegistry::updateCachedUrl( const QString &oldPath, const QString &newPath )
{
    int deviceId = m_collection->mountPointManager()->getIdForUrl( QUrl::fromLocalFile(oldPath) );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, oldPath );
    TrackPath oldId( deviceId, rpath );

    int newdeviceId = m_collection->mountPointManager()->getIdForUrl( QUrl::fromLocalFile(newPath) );
    QString newRpath = m_collection->mountPointManager()->getRelativePath( newdeviceId, newPath );
    TrackPath newId( newdeviceId, newRpath );

    QMutexLocker locker( &m_trackMutex );
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
    // TODO: improve uid handling
    if( m_uidMap.contains( newUid ) )
        warning() << "updating uid to an already existing uid.";
    else if( !oldUid.isEmpty() && !m_uidMap.contains( oldUid ) )
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
    {
        QString query;
        QStringList result;

        // -- get all the track info
        query = QStringLiteral("SELECT %1 FROM urls %2 "
            "WHERE urls.uniqueid = '%3';");
        query = query.arg( Meta::SqlTrack::getTrackReturnValues(),
                           Meta::SqlTrack::getTrackJoinConditions(),
                           m_collection->sqlStorage()->escape( uid ) );
        result = m_collection->sqlStorage()->query( query );
        if( result.isEmpty() )
            return Meta::TrackPtr();

        Meta::SqlTrack *sqlTrack = new Meta::SqlTrack( m_collection, result );
        Meta::TrackPtr trackPtr( sqlTrack );

        int deviceid = m_collection->mountPointManager()->getIdForUrl( trackPtr->playableUrl() );
        QString rpath = m_collection->mountPointManager()->getRelativePath( deviceid, trackPtr->playableUrl().path() );
        TrackPath id(deviceid, rpath);
        m_trackMap.insert( id, trackPtr );
        m_uidMap.insert( uid, trackPtr );
        return trackPtr;
    }
}

void
SqlRegistry::removeTrack( int urlId, const QString &uid )
{
    // delete all entries linked to the url, including track
    QStringList tables = QStringList() << QStringLiteral("tracks") << QStringLiteral("lyrics") << QStringLiteral("statistics") << QStringLiteral("urls_labels");
    for( const QString &table : tables )
    {
        QString query = QStringLiteral( "DELETE FROM %1 WHERE url=%2" ).arg( table ).arg( urlId );
        m_collection->sqlStorage()->query( query );
    }

    // delete url entry from database; we used to keep it and keep its statistics, but
    // DatabaseUpdater::deleteAllRedundant( url ) removes the url entry on the next Amarok
    // startup, plus we don't know how long we should keep the entry, so just delete
    // everything. ScanResultProcessor should be witty enough not to delete tracks that
    // have been moved to another directory and/or device, even if it is currently
    // unavailable.
    QString query = QStringLiteral( "DELETE FROM urls WHERE id=%1" ).arg( urlId );
    m_collection->sqlStorage()->query( query );

    // --- delete the track from memory
    QMutexLocker locker( &m_trackMutex );
    if( m_uidMap.contains( uid ) )
    {
        // -- remove from hashes
        Meta::TrackPtr track = m_uidMap.take( uid );
        Meta::SqlTrack *sqlTrack = static_cast<Meta::SqlTrack*>( track.data() );

        int deviceId = m_collection->mountPointManager()->getIdForUrl( sqlTrack->playableUrl() );
        QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, sqlTrack->playableUrl().path() );
        TrackPath id(deviceId, rpath);
        m_trackMap.remove( id );
    }
}

// -------- artist

Meta::ArtistPtr
SqlRegistry::getArtist( const QString &oName )
{
    QMutexLocker locker( &m_artistMutex );

    QString name = oName.left( DatabaseUpdater::textColumnLength() );
    if( m_artistMap.contains( name ) )
        return m_artistMap.value( name );

    int id;

    QString query = QStringLiteral( "SELECT id FROM artists WHERE name = '%1';" ).arg( m_collection->sqlStorage()->escape( name ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QStringLiteral( "INSERT INTO artists( name ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( name ) );
        id = m_collection->sqlStorage()->insert( insert, QStringLiteral("artists") );
        m_collectionChanged = true;
    }
    else
    {
        id = res[0].toInt();
    }

    if( !id )
        return Meta::ArtistPtr();

    Meta::ArtistPtr artist( new Meta::SqlArtist( m_collection, id, name ) );
    m_artistMap.insert( name, artist );
    m_artistIdMap.insert( id, artist );
    return artist;
}

Meta::ArtistPtr
SqlRegistry::getArtist( int id )
{
    QMutexLocker locker( &m_artistMutex );

    if( m_artistIdMap.contains( id ) )
        return m_artistIdMap.value( id );

    QString query = QStringLiteral( "SELECT name FROM artists WHERE id = %1;" ).arg( id );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
        return Meta::ArtistPtr();

    QString name = res[0];
    Meta::ArtistPtr artist( new Meta::SqlArtist( m_collection, id, name ) );
    m_artistMap.insert( name, artist );
    m_artistIdMap.insert( id, artist );
    return artist;
}

Meta::ArtistPtr
SqlRegistry::getArtist( int id, const QString &name )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_artistMutex );

    if( m_artistMap.contains( name ) )
        return m_artistMap.value( name );

    Meta::ArtistPtr artist( new Meta::SqlArtist( m_collection, id, name ) );
    m_artistMap.insert( name, artist );
    m_artistIdMap.insert( id, artist );
    return artist;
}

// -------- genre

Meta::GenrePtr
SqlRegistry::getGenre( const QString &oName )
{
    QMutexLocker locker( &m_genreMutex );

    QString name = oName.left( DatabaseUpdater::textColumnLength() );
    if( m_genreMap.contains( name ) )
        return m_genreMap.value( name );

    int id;

    QString query = QStringLiteral( "SELECT id FROM genres WHERE name = '%1';" ).arg( m_collection->sqlStorage()->escape( name ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QStringLiteral( "INSERT INTO genres( name ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( name ) );
        id = m_collection->sqlStorage()->insert( insert, QStringLiteral("genres") );
        m_collectionChanged = true;
    }
    else
    {
        id = res[0].toInt();
    }

    if( !id )
        return Meta::GenrePtr();

    Meta::GenrePtr genre( new Meta::SqlGenre( m_collection, id, name ) );
    m_genreMap.insert( name, genre );
    return genre;
}

Meta::GenrePtr
SqlRegistry::getGenre( int id )
{
    QMutexLocker locker( &m_genreMutex );

    QString query = QStringLiteral( "SELECT name FROM genres WHERE id = '%1';" ).arg( id );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
        return Meta::GenrePtr();

    QString name = res[0];
    Meta::GenrePtr genre( new Meta::SqlGenre( m_collection, id, name ) );
    m_genreMap.insert( name, genre );
    return genre;
}

Meta::GenrePtr
SqlRegistry::getGenre( int id, const QString &name )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_genreMutex );

    if( m_genreMap.contains( name ) )
        return m_genreMap.value( name );

    Meta::GenrePtr genre( new Meta::SqlGenre( m_collection, id, name ) );
    m_genreMap.insert( name, genre );
    return genre;
}

// -------- composer

Meta::ComposerPtr
SqlRegistry::getComposer( const QString &oName )
{
    QMutexLocker locker( &m_composerMutex );

    QString name = oName.left( DatabaseUpdater::textColumnLength() );
    if( m_composerMap.contains( name ) )
        return m_composerMap.value( name );

    int id;

    QString query = QStringLiteral( "SELECT id FROM composers WHERE name = '%1';" ).arg( m_collection->sqlStorage()->escape( name ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QStringLiteral( "INSERT INTO composers( name ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( name ) );
        id = m_collection->sqlStorage()->insert( insert, QStringLiteral("composers") );
        m_collectionChanged = true;
    }
    else
    {
        id = res[0].toInt();
    }

    if( !id )
        return Meta::ComposerPtr();

    Meta::ComposerPtr composer( new Meta::SqlComposer( m_collection, id, name ) );
    m_composerMap.insert( name, composer );
    return composer;
}

Meta::ComposerPtr
SqlRegistry::getComposer( int id )
{
    if( id <= 0 )
        return Meta::ComposerPtr();

    QMutexLocker locker( &m_composerMutex );

    QString query = QStringLiteral( "SELECT name FROM composers WHERE id = '%1';" ).arg( id );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
        return Meta::ComposerPtr();

    QString name = res[0];
    Meta::ComposerPtr composer( new Meta::SqlComposer( m_collection, id, name ) );
    m_composerMap.insert( name, composer );
    return composer;
}

Meta::ComposerPtr
SqlRegistry::getComposer( int id, const QString &name )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_composerMutex );

    if( m_composerMap.contains( name ) )
        return m_composerMap.value( name );

    Meta::ComposerPtr composer( new Meta::SqlComposer( m_collection, id, name ) );
    m_composerMap.insert( name, composer );
    return composer;
}

// -------- year

Meta::YearPtr
SqlRegistry::getYear( int year, int yearId )
{
    QMutexLocker locker( &m_yearMutex );

    if( m_yearMap.contains( year ) )
        return m_yearMap.value( year );

    // don't know the id yet
    if( yearId <= 0 )
    {
        QString query = QStringLiteral( "SELECT id FROM years WHERE name = '%1';" ).arg( QString::number( year ) );
        QStringList res = m_collection->sqlStorage()->query( query );
        if( res.isEmpty() )
        {
            QString insert = QStringLiteral( "INSERT INTO years( name ) VALUES ('%1');" ).arg( QString::number( year ) );
            yearId = m_collection->sqlStorage()->insert( insert, QStringLiteral("years") );
            m_collectionChanged = true;
        }
        else
        {
            yearId = res[0].toInt();
        }
    }

    if( !yearId )
        return Meta::YearPtr();

    Meta::YearPtr yearPtr( new Meta::SqlYear( m_collection, yearId, year ) );
    m_yearMap.insert( year, yearPtr );
    return yearPtr;
}

// -------- album

Meta::AlbumPtr
SqlRegistry::getAlbum( const QString &oName, const QString &oArtist )
{
    // we allow albums with empty name but nonempty artist, see bug 272471
    QString name = oName.left( DatabaseUpdater::textColumnLength() );
    QString albumArtist = oArtist.left( DatabaseUpdater::textColumnLength() );
    AlbumKey key( name, albumArtist );

    QMutexLocker locker( &m_albumMutex );
    if( m_albumMap.contains( key ) )
        return m_albumMap.value( key );

    int albumId = -1;
    int artistId = -1;

    QString query = QStringLiteral( "SELECT id FROM albums WHERE name = '%1' AND " ).arg( m_collection->sqlStorage()->escape( name ) );

    if( albumArtist.isEmpty() )
    {
        query += QStringLiteral( "artist IS NULL" );
    }
    else
    {
        Meta::ArtistPtr artistPtr = getArtist( albumArtist );
        if( !artistPtr )
            return Meta::AlbumPtr();
        Meta::SqlArtist *sqlArtist = static_cast<Meta::SqlArtist*>(artistPtr.data());
        artistId = sqlArtist->id();

        query += QStringLiteral( "artist=%1" ).arg( artistId );
    }

    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        // ok. have to create a new album
        QString insert = QStringLiteral( "INSERT INTO albums( name, artist ) VALUES ('%1',%2);" ).
            arg( m_collection->sqlStorage()->escape( name ),
                 artistId > 0 ? QString::number( artistId ) : QStringLiteral("NULL") );
        albumId = m_collection->sqlStorage()->insert( insert, QStringLiteral("albums") );
        m_collectionChanged = true; // we just added a new album
    }
    else
    {
        albumId = res[0].toInt();
    }

    if( !albumId )
        return Meta::AlbumPtr();

    Meta::SqlAlbum *sqlAlbum = new Meta::SqlAlbum( m_collection, albumId, name, artistId );
    Meta::AlbumPtr album( sqlAlbum );
    m_albumMap.insert( key, album );
    m_albumIdMap.insert( albumId, album );
    locker.unlock(); // prevent deadlock
    return album;
}

Meta::AlbumPtr
SqlRegistry::getAlbum( int albumId )
{
    Q_ASSERT( albumId > 0 ); // must be a valid id

    {
        // we want locker only for this block because we call another getAlbum() below
        QMutexLocker locker( &m_albumMutex );
        if( m_albumIdMap.contains( albumId ) )
            return m_albumIdMap.value( albumId );
    }

    QString query = QStringLiteral( "SELECT name, artist FROM albums WHERE id = %1" ).arg( albumId );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
        return Meta::AlbumPtr(); // someone messed up

    QString name = res[0];
    int artistId = res[1].toInt();
    return getAlbum( albumId, name, artistId );
}

Meta::AlbumPtr
SqlRegistry::getAlbum( int albumId, const QString &name, int artistId )
{
    Q_ASSERT( albumId > 0 ); // must be a valid id

    QMutexLocker locker( &m_albumMutex );
    if( m_albumIdMap.contains( albumId ) )
        return m_albumIdMap.value( albumId );

    Meta::ArtistPtr artist = getArtist( artistId );
    AlbumKey key(name, artist ? artist->name() : QString() );
    if( m_albumMap.contains( key ) )
        return m_albumMap.value( key );

    Meta::SqlAlbum *sqlAlbum = new Meta::SqlAlbum( m_collection, albumId, name, artistId );
    Meta::AlbumPtr album( sqlAlbum );
    m_albumMap.insert( key, album );
    m_albumIdMap.insert( albumId, album );
    return album;
}

// ------------ label

Meta::LabelPtr
SqlRegistry::getLabel( const QString &oLabel )
{
    QMutexLocker locker( &m_labelMutex );
    QString label = oLabel.left( DatabaseUpdater::textColumnLength() );
    if( m_labelMap.contains( label ) )
        return m_labelMap.value( label );

    int id;

    QString query = QStringLiteral( "SELECT id FROM labels WHERE label = '%1';" ).arg( m_collection->sqlStorage()->escape( label ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QStringLiteral( "INSERT INTO labels( label ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( label ) );
        id = m_collection->sqlStorage()->insert( insert, QStringLiteral("albums") );
    }
    else
    {
        id = res[0].toInt();
    }

    if( !id )
        return Meta::LabelPtr();

    Meta::LabelPtr labelPtr( new Meta::SqlLabel( m_collection, id, label ) );
    m_labelMap.insert( label, labelPtr );
    return labelPtr;
}

Meta::LabelPtr
SqlRegistry::getLabel( int id )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_labelMutex );

    QString query = QStringLiteral( "SELECT label FROM labels WHERE id = '%1';" ).arg( id );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
        return Meta::LabelPtr();

    QString label = res[0];
    Meta::LabelPtr labelPtr( new Meta::SqlLabel( m_collection, id, label ) );
    m_labelMap.insert( label, labelPtr );
    return labelPtr;
}

Meta::LabelPtr
SqlRegistry::getLabel( int id, const QString &label )
{
    Q_ASSERT( id > 0 ); // must be a valid id
    QMutexLocker locker( &m_labelMutex );

    if( m_labelMap.contains( label ) )
        return m_labelMap.value( label );

    Meta::LabelPtr labelPtr( new Meta::SqlLabel( m_collection, id, label ) );
    m_labelMap.insert( label, labelPtr );
    return labelPtr;
}



// ---------------- generic database management --------------

void
SqlRegistry::blockDatabaseUpdate()
{
    QMutexLocker locker( &m_blockMutex );
    m_blockDatabaseUpdateCount++;
}

void
SqlRegistry::unblockDatabaseUpdate()
{
    {
        QMutexLocker locker( &m_blockMutex );
        Q_ASSERT( m_blockDatabaseUpdateCount > 0 );
        m_blockDatabaseUpdateCount--;
    }

    // update the database
    commitDirtyTracks();
}

void
SqlRegistry::commitDirtyTracks()
{
    QMutexLocker locker( &m_blockMutex );

    if( m_blockDatabaseUpdateCount > 0 )
        return;

    QList< Meta::SqlYearPtr > dirtyYears = m_dirtyYears.values();
    QList< Meta::SqlGenrePtr > dirtyGenres = m_dirtyGenres.values();
    QList< Meta::SqlAlbumPtr > dirtyAlbums = m_dirtyAlbums.values();
    QList< Meta::SqlTrackPtr > dirtyTracks = m_dirtyTracks.values();
    QList< Meta::SqlArtistPtr > dirtyArtists = m_dirtyArtists.values();
    QList< Meta::SqlComposerPtr > dirtyComposers = m_dirtyComposers.values();

    m_dirtyYears.clear();
    m_dirtyGenres.clear();
    m_dirtyAlbums.clear();
    m_dirtyTracks.clear();
    m_dirtyArtists.clear();
    m_dirtyComposers.clear();
    locker.unlock(); // need to unlock before notifying the observers

    // -- commit all the dirty tracks
    TrackUrlsTableCommitter().commit( dirtyTracks );
    TrackTracksTableCommitter().commit( dirtyTracks );
    TrackStatisticsTableCommitter().commit( dirtyTracks );

    // -- notify all observers
    for( Meta::SqlYearPtr year : dirtyYears )
    {
        // this means that a new year was added to track or an old removed (or both),
        // Collection docs says we need to Q_EMIT updated() in this case. Ditto below.
        m_collectionChanged = true;
        year->invalidateCache();
        year->notifyObservers();
    }
    for( Meta::SqlGenrePtr genre : dirtyGenres )
    {
        m_collectionChanged = true;
        genre->invalidateCache();
        genre->notifyObservers();
    }
    for( Meta::SqlAlbumPtr album : dirtyAlbums )
    {
        m_collectionChanged = true;
        album->invalidateCache();
        album->notifyObservers();
    }
    for( Meta::SqlTrackPtr track : dirtyTracks )
    {
        // if only track changes, no need to Q_EMIT updated() from here
        track->notifyObservers();
    }
    for( Meta::SqlArtistPtr artist : dirtyArtists )
    {
        m_collectionChanged = true;
        artist->invalidateCache();
        artist->notifyObservers();
    }
    for( Meta::SqlComposerPtr composer : dirtyComposers )
    {
        m_collectionChanged = true;
        composer->invalidateCache();
        composer->notifyObservers();
    }
    if( m_collectionChanged )
        m_collection->collectionUpdated();
    m_collectionChanged = false;
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
        #define mapCached( Cache, Key, Map, Res ) \
        Cache[Key] = qMakePair( Map.count(), Res.join(QLatin1String(" ")).toInt() );

        QMap<QString, QPair<int, int> > cachedBefore;

        QString query = QStringLiteral( "SELECT COUNT(*) FROM albums;" );
        QStringList res = m_collection->sqlStorage()->query( query );
        mapCached( cachedBefore, QStringLiteral("albums"), m_albumMap, res );

        query = QStringLiteral( "SELECT COUNT(*) FROM tracks;" );
        res = m_collection->sqlStorage()->query( query );
        mapCached( cachedBefore, QStringLiteral("tracks"), m_trackMap, res );

        query = QStringLiteral( "SELECT COUNT(*) FROM artists;" );
        res = m_collection->sqlStorage()->query( query );
        mapCached( cachedBefore, QStringLiteral("artists"), m_artistMap, res );

        query = QStringLiteral( "SELECT COUNT(*) FROM genres;" );
        res = m_collection->sqlStorage()->query( query );
        mapCached( cachedBefore, QStringLiteral("genres"), m_genreMap, res );

        //this very simple garbage collector doesn't handle cyclic object graphs
        //so care has to be taken to make sure that we are not dealing with a cyclic graph
        //by invalidating the tracks cache on all objects
        #define foreachInvalidateCache( Key, Type, RealType, x ) \
        for( QMutableHashIterator<Key,Type > iter(x); iter.hasNext(); ) \
            RealType::staticCast( iter.next().value() )->invalidateCache()

        foreachInvalidateCache( AlbumKey, Meta::AlbumPtr, AmarokSharedPointer<Meta::SqlAlbum>, m_albumMap );
        foreachInvalidateCache( QString, Meta::ArtistPtr, AmarokSharedPointer<Meta::SqlArtist>, m_artistMap );
        foreachInvalidateCache( QString, Meta::GenrePtr, AmarokSharedPointer<Meta::SqlGenre>, m_genreMap );
        foreachInvalidateCache( QString, Meta::ComposerPtr, AmarokSharedPointer<Meta::SqlComposer>, m_composerMap );
        foreachInvalidateCache( int, Meta::YearPtr, AmarokSharedPointer<Meta::SqlYear>, m_yearMap );
        foreachInvalidateCache( QString, Meta::LabelPtr, AmarokSharedPointer<Meta::SqlLabel>, m_labelMap );
        #undef foreachInvalidateCache

        // elem.count() == 2 is correct because elem is one pointer to the object
        // and the other is stored in the hash map (except for m_trackMap, m_albumMap
        // and m_artistMap , where another reference is stored in m_uidMap, m_albumIdMap
        // and m_artistIdMap
        #define foreachCollectGarbage( Key, Type, RefCount, x ) \
        for( QMutableHashIterator<Key,Type > iter(x); iter.hasNext(); ) \
        { \
            Type elem = iter.next().value(); \
            if( elem.count() == RefCount ) \
                iter.remove(); \
        }

        foreachCollectGarbage( TrackPath, Meta::TrackPtr, 3, m_trackMap );
        foreachCollectGarbage( QString, Meta::TrackPtr, 2, m_uidMap );
        // run before artist so that album artist pointers can be garbage collected
        foreachCollectGarbage( AlbumKey, Meta::AlbumPtr, 3, m_albumMap );
        foreachCollectGarbage( int, Meta::AlbumPtr, 2, m_albumIdMap );
        foreachCollectGarbage( QString, Meta::ArtistPtr, 3, m_artistMap );
        foreachCollectGarbage( int, Meta::ArtistPtr, 2, m_artistIdMap );
        foreachCollectGarbage( QString, Meta::GenrePtr, 2, m_genreMap );
        foreachCollectGarbage( QString, Meta::ComposerPtr, 2, m_composerMap );
        foreachCollectGarbage( int, Meta::YearPtr, 2, m_yearMap );
        foreachCollectGarbage( QString, Meta::LabelPtr, 2, m_labelMap );
        #undef foreachCollectGarbage

        QMap<QString, QPair<int, int> > cachedAfter;

        query = QStringLiteral( "SELECT COUNT(*) FROM albums;" );
        res = m_collection->sqlStorage()->query( query );
        mapCached( cachedAfter, QStringLiteral("albums"), m_albumMap, res );

        query = QStringLiteral( "SELECT COUNT(*) FROM tracks;" );
        res = m_collection->sqlStorage()->query( query );
        mapCached( cachedAfter, QStringLiteral("tracks"), m_trackMap, res );

        query = QStringLiteral( "SELECT COUNT(*) FROM artists;" );
        res = m_collection->sqlStorage()->query( query );
        mapCached( cachedAfter, QStringLiteral("artists"), m_artistMap, res );

        query = QStringLiteral( "SELECT COUNT(*) FROM genres;" );
        res = m_collection->sqlStorage()->query( query );
        mapCached( cachedAfter, QStringLiteral("genres"), m_genreMap, res );
        #undef mapCached

        if( cachedBefore != cachedAfter )
        {
            QMapIterator<QString, QPair<int, int> > i(cachedAfter), iLast(cachedBefore);
            while( i.hasNext() && iLast.hasNext() )
            {
                i.next();
                iLast.next();
                int count = i.value().first;
                int total = i.value().second;
                QString diff = QString::number( count - iLast.value().first );
                QString text = QStringLiteral( "%1 (%2) of %3 cached" ).arg( count ).arg( diff ).arg( total );
                debug() << QStringLiteral( "%1: %2" ).arg( i.key(), 8 ).arg( text ).toLocal8Bit().constData();
            }
        }
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


