/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2009-2010 Jeff Mitchell <mitchell@kde.org>                             *
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

#include "SqlScanResultProcessor.h"

#define DEBUG_PREFIX "SqlScanResultProcessor"

#include "collectionscanner/Directory.h"
#include "collectionscanner/Album.h"
#include "collectionscanner/Track.h"
#include "collectionscanner/Playlist.h"
#include "core/support/Debug.h"
#include "core-impl/collections/db/MountPointManager.h"
#include "core-impl/collections/db/sql/SqlQueryMaker.h"
#include "playlistmanager/PlaylistManager.h"

SqlScanResultProcessor::SqlScanResultProcessor( Collections::SqlCollection *collection, QObject *parent )
    : ScanResultProcessor( parent ),
      m_collection( collection )
{
}

SqlScanResultProcessor::~SqlScanResultProcessor()
{
}

void
SqlScanResultProcessor::commit()
{
    DEBUG_BLOCK

    m_collection->sqlStorage()->clearLastErrors();
    urlsCacheInit();

    // -- call the base implementation
    ScanResultProcessor::commit();

    m_lastErrors.append( m_collection->sqlStorage()->getLastErrors() );
}

void
SqlScanResultProcessor::blockUpdates()
{
    m_collection->blockUpdatedSignal();
    m_collection->registry()->blockDatabaseUpdate();
}

void
SqlScanResultProcessor::unblockUpdates()
{
    m_collection->registry()->unblockDatabaseUpdate();
    m_collection->unblockUpdatedSignal();
}

void
SqlScanResultProcessor::commitDirectory( CollectionScanner::Directory *directory )
{
    QString path = directory->path();
    // a bit of paranoia:
    if( m_foundDirectories.contains( path ) )
        warning() << "commitDirectory(): duplicate directory path" << path << "in"
                  << "collectionscanner output. This shouldn't happen.";

    // getDirectory() updates the directory entry mtime:
    int dirId = m_collection->registry()->getDirectory( path, directory->mtime() );
    m_directoryIds.insert( directory, dirId );
    m_foundDirectories.insert( path, dirId );

    ScanResultProcessor::commitDirectory( directory );
}

void
SqlScanResultProcessor::commitAlbum( CollectionScanner::Album *album )
{
    debug() << "commitAlbum on"<<album->name()<< "artist"<<album->artist();

    // --- get or create the album
    Meta::SqlAlbumPtr metaAlbum;
    metaAlbum = Meta::SqlAlbumPtr::staticCast( m_collection->getAlbum( album->name(), album->artist() ) );
    if( !metaAlbum )
        return;
    m_albumIds.insert( album, metaAlbum->id() );

    // --- add all tracks
    foreach( CollectionScanner::Track *track, album->tracks() )
        commitTrack( track, album );

    // --- set the cover if we have one
    // we need to do this after the tracks are added in case of an embedded cover
    bool suppressAutoFetch = metaAlbum->suppressImageAutoFetch();
    metaAlbum->setSuppressImageAutoFetch( true );
    if( m_type == FullScan )
    {
        if( !album->cover().isEmpty() )
        {
            metaAlbum->removeImage();
            metaAlbum->setImage( album->cover() );
        }
    }
    else
    {
        if( !metaAlbum->hasImage() && !album->cover().isEmpty() )
            metaAlbum->setImage( album->cover() );
    }
    metaAlbum->setSuppressImageAutoFetch( suppressAutoFetch );
}

void
SqlScanResultProcessor::commitTrack( CollectionScanner::Track *track,
                                     CollectionScanner::Album *srcAlbum )
{
    Q_ASSERT( track );
    Q_ASSERT( srcAlbum );

    Q_ASSERT( m_directoryIds.contains( track->directory() ) );
    int directoryId = m_directoryIds.value( track->directory() );
    Q_ASSERT( m_albumIds.contains( srcAlbum ) );
    int albumId = m_albumIds.value( srcAlbum );

    QString uid = track->uniqueid();
    if( uid.isEmpty() )
    {
        warning() << "commitTrack(): got track with empty unique id from the scanner,"
                  << "not adding it";
        m_lastErrors.append( QString( "Not adding track %1 because it has no unique id." ).
                             arg(track->path()) );
        return;
    }
    uid = m_collection->generateUidUrl( uid );

    int deviceId = m_collection->mountPointManager()->getIdForUrl( track->path() );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, track->path() );

    if( m_foundTracks.contains( uid ) )
    {
        const UrlEntry old = m_urlsCache.value( m_uidCache.value( uid ) );
        const char *pattern = I18N_NOOP( "Duplicates found, the second file will be ignored:\n%1\n%2" );

        // we want translated version for GUI and non-translated for debug log
        warning() << "commitTrack():" << QString( pattern ).arg( old.path, track->path() );
        m_lastErrors.append( i18n( pattern, old.path, track->path() ) );
        return;
    }

    Meta::SqlTrackPtr metaTrack;
    UrlEntry entry;
    // find an existing track by uid
    if( m_uidCache.contains( uid ) )
    {
        // uid is sadly not unique. Try to find the best url id.
        int urlId = findBestUrlId( uid, track->path() );
        Q_ASSERT( urlId > 0 );
        Q_ASSERT( m_urlsCache.contains( urlId ) );
        entry = m_urlsCache.value( urlId );
        entry.path = track->path();
        entry.directoryId = directoryId;

        metaTrack = Meta::SqlTrackPtr::staticCast( m_collection->registry()->getTrack( urlId ) );
        Q_ASSERT( metaTrack->urlId() == entry.id );
    }
    // find an existing track by path
    else if( m_pathCache.contains( track->path() ) )
    {
        int urlId = m_pathCache.value( track->path() );
        Q_ASSERT( m_urlsCache.contains( urlId ) );
        entry = m_urlsCache.value( urlId );
        entry.uid = uid;
        entry.directoryId = directoryId;

        metaTrack = Meta::SqlTrackPtr::staticCast( m_collection->registry()->getTrack( urlId ) );
        Q_ASSERT( metaTrack->urlId() == entry.id );
    }
    // create a new one
    else
    {
        static int autoDecrementId = -1;
        entry.id = autoDecrementId--;
        entry.path = track->path();
        entry.uid = uid;
        entry.directoryId = directoryId;

        metaTrack = Meta::SqlTrackPtr::staticCast( m_collection->getTrack( deviceId, rpath, directoryId, uid ) );
    }

    if( !metaTrack )
    {
        QString text = QString( "Something went wrong when importing track %1, metaTrack "
                "is null while it shouldn't be." ).arg( track->path() );
        warning() << "commitTrack():" << text.toLocal8Bit().data();
        m_lastErrors.append( text );
        return;
    }
    urlsCacheInsert( entry ); // removes the previous entry (by id) first if necessary
    m_foundTracks.insert( uid, entry.id );

    // TODO: we need to check the modified date of the file agains the last updated of the file
    // to figure out if the track information was updated from outside Amarok.
    // In such a case we would fully reread all the information as if in a FullScan

    // -- set the values
    metaTrack->setWriteFile( false ); // no need to write the tags back
    metaTrack->beginUpdate();

    metaTrack->setUidUrl( uid );
    metaTrack->setUrl( deviceId, rpath, directoryId );

    if( m_type == FullScan ||
        !track->title().isEmpty() )
        metaTrack->setTitle( track->title() );

    if( m_type == FullScan ||
        albumId != -1 )
        metaTrack->setAlbum( albumId );

    if( m_type == FullScan ||
        !track->artist().isEmpty() )
        metaTrack->setArtist( track->artist() );

    if( m_type == FullScan ||
        !track->composer().isEmpty() )
        metaTrack->setComposer( track->composer() );

    if( m_type == FullScan ||
        track->year() >= 0 )
        metaTrack->setYear( (track->year() >= 0) ? track->year() : 0 );

    if( m_type == FullScan ||
        !track->genre().isEmpty() )
        metaTrack->setGenre( track->genre() );

    metaTrack->setType( track->filetype() );

    if( m_type == FullScan ||
        !track->bpm() >= 0 )
        metaTrack->setBpm( track->bpm() );

    if( m_type == FullScan ||
        !track->comment().isEmpty() )
        metaTrack->setComment( track->comment() );

    if( (m_type == FullScan || metaTrack->score() == 0) &&
        track->score() >= 0 )
        metaTrack->setScore( track->score() );

    if( (m_type == FullScan || metaTrack->rating() == 0.0) &&
        track->rating() >= 0 )
        metaTrack->setRating( track->rating() );

    if( (m_type == FullScan || metaTrack->length() == 0) &&
        track->length() >= 0 )
        metaTrack->setLength( track->length() );

    // the filesize is updated every time after the
    // file is changed. Doesn't make sense to set it.

    if( (m_type == FullScan || !metaTrack->modifyDate().isValid()) &&
        track->modified().isValid() )
        metaTrack->setModifyDate( track->modified() );

    if( (m_type == FullScan || metaTrack->sampleRate() == 0) &&
        track->samplerate() >= 0 )
        metaTrack->setSampleRate( track->samplerate() );

    if( (m_type == FullScan || metaTrack->bitrate() == 0) &&
        track->bitrate() >= 0 )
        metaTrack->setBitrate( track->bitrate() );

    if( (m_type == FullScan || metaTrack->trackNumber() == 0) &&
        track->track() >= 0 )
        metaTrack->setTrackNumber( track->track() );

    if( (m_type == FullScan || metaTrack->discNumber() == 0) &&
        track->disc() >= 0 )
        metaTrack->setDiscNumber( track->disc() );

    if( m_type == FullScan && track->playcount() >= metaTrack->playCount() )
        metaTrack->setPlayCount( track->playcount() );


    Meta::ReplayGainTag modes[] = { Meta::ReplayGain_Track_Gain,
        Meta::ReplayGain_Track_Peak,
        Meta::ReplayGain_Album_Gain,
        Meta::ReplayGain_Album_Peak };

    for( int i=0; i<4; i++ )
    {
        if( track->replayGain( modes[i] ) != 0.0 )
            metaTrack->setReplayGain( modes[i], track->replayGain( modes[i] ) );
    }

    metaTrack->endUpdate();
    metaTrack->setWriteFile( true );
}

void
SqlScanResultProcessor::deleteDeletedDirectories()
{
    DEBUG_BLOCK
    SqlStorage *storage = m_collection->sqlStorage();

    // -- get a list of all mounted device ids
    QList<int> idList = m_collection->mountPointManager()->getMountedDeviceIds();
    QString deviceIds;
    foreach( int id, idList )
    {
        if ( !deviceIds.isEmpty() )
            deviceIds += ',';
        deviceIds += QString::number( id );
    }

    // -- get all (mounted) directories
    QString query = QString( "SELECT id, deviceid, dir FROM directories "
                             "WHERE deviceid IN (%1)" ).arg( deviceIds );
    QStringList res = storage->query( query );

    // -- check if the have been found during the scan
    for( int i = 0; i < res.count(); )
    {
        int dirId = res.at( i++ ).toInt();
        int deviceId = res.at( i++ ).toInt();
        QString dir = res.at( i++ );

        /* we need to match directories by their (absolute) path, otherwise following
         * scenario triggers statistics loss (bug 298275):
         *
         * 1. user relocates collection to different filesystem, but clones path structure
         *    or toggles MassStorageDeviceHandler enabled in Config -> plugins.
         * 2. collectionscanner knows nothings about directory ids, so it doesn't detect
         *    any track changes and emits a bunch of skipped (unchanged) dirs with no
         *    tracks.
         * 3. SqlRegistry::getDirectory() called there from returns different directory id
         *    then in past.
         * 4. deleteDeletedDirectories() is called, and if it operates on directory ids,
         *    it happily removes _all_ directories, taking tracks with it.
         * 5. Tracks disappear from the UI until full rescan, stats, lyrics, labels are
         *    lost forever.
         */
        QString path = m_collection->mountPointManager()->getAbsolutePath( deviceId, dir );
        bool deleteThisDir = false;
        if( !m_foundDirectories.contains( path ) )
            deleteThisDir = true;
        else if( m_foundDirectories.value( path ) != dirId )
        {
            int newDirId = m_foundDirectories.value( path );
            // as a safety measure, we don't delete the old dir if relocation fails
            deleteThisDir = relocateTracksToNewDirectory( dirId, newDirId );
        }

        if( deleteThisDir )
        {
            deleteDeletedTracks( dirId );
            query = QString( "DELETE FROM directories WHERE id = %1;" ).arg( dirId );
            storage->query( query );
        }
    }
}

void
SqlScanResultProcessor::deleteDeletedTracks( CollectionScanner::Directory *directory )
{
    Q_ASSERT( m_directoryIds.contains( directory ) );
    int directoryId = m_directoryIds.value( directory );
    deleteDeletedTracks( directoryId );
}

void
SqlScanResultProcessor::deleteDeletedTracks( int directoryId )
{
    // -- find all tracks
    QList<int> urlIds = m_directoryCache.values( directoryId );

    // -- check if the tracks have been found during the scan
    foreach( int urlId, urlIds )
    {
        Q_ASSERT( m_urlsCache.contains( urlId ) );
        const UrlEntry &entry = m_urlsCache[ urlId ];
        Q_ASSERT( entry.directoryId == directoryId );
        // we need to match both uid and url id, because uid is not unique
        if( !m_foundTracks.contains( entry.uid, entry.id ) )
        {
            removeTrack( entry );
            urlsCacheRemove( entry );
        }
    }
}

int
SqlScanResultProcessor::findBestUrlId( const QString &uid, const QString &path )
{
    QList<int> urlIds = m_uidCache.values( uid );
    if( urlIds.isEmpty() )
        return -1;
    if( urlIds.size() == 1 )
        return urlIds.at( 0 ); // normal operation

    foreach( int testedUrlId, urlIds )
    {
        Q_ASSERT( m_urlsCache.contains( testedUrlId ) );
        if( m_urlsCache[ testedUrlId ].path == path )
            return testedUrlId;
    }

    warning() << "multiple url entries with uid" << uid << "found in the database, but"
              << "none with current path" << path << "Choosing blindly the last one out"
              << "of url id candidates" << urlIds;
    return urlIds.last();
}

bool
SqlScanResultProcessor::relocateTracksToNewDirectory( int oldDirId, int newDirId )
{
    QList<int> urlIds = m_directoryCache.values( oldDirId );
    if( urlIds.isEmpty() )
        return true; // nothing to do

    MountPointManager *manager = m_collection->mountPointManager();
    SqlRegistry *reg = m_collection->registry();
    SqlStorage *storage = m_collection->sqlStorage();

    // sanity checking, not strictly needed, but imagine new device appearing in the
    // middle of the scan, so rather prevent db corruption:
    QStringList res = storage->query( QString( "SELECT deviceid FROM directories "
                                               "WHERE id = %1" ).arg( newDirId ) );
    if( res.count() != 1 )
    {
        warning() << "relocateTracksToNewDirectory(): no or multiple entries when"
                  << "quering directory with id" << newDirId;
        return false;
    }
    int newDirDeviceId = res.at( 0 ).toInt();

    foreach( int urlId, urlIds )
    {
        Q_ASSERT( m_urlsCache.contains( urlId ) );
        UrlEntry entry = m_urlsCache.value( urlId );
        Meta::SqlTrackPtr track = Meta::SqlTrackPtr::staticCast( reg->getTrack( urlId ) );
        Q_ASSERT( track );

        // not strictly needed, but we want to sanity check it to prevent corrupt db
        int deviceId = manager->getIdForUrl( entry.path );
        if( newDirDeviceId != deviceId )
        {
            warning() << "relocateTracksToNewDirectory(): device id from newDirId ("
                      << res.at( 0 ).toInt() << ") and device id from mountPointManager ("
                      << deviceId << ") don't match!";
            return false;
        }
        QString rpath = manager->getRelativePath( deviceId, entry.path );

        track->setUrl( deviceId, rpath, newDirId );
        entry.directoryId = newDirId;
        urlsCacheInsert( entry ); // removes the previous entry (by id) first
    }
    return true;
}

void
SqlScanResultProcessor::removeTrack( const UrlEntry &entry )
{
    debug() << "removeTrack(" << entry << ")";
    if( !m_lastErrors.isEmpty() )
    {
        warning() << "removeTrack(): there were errors, skipping destructive operations";
        return;
    }

    SqlRegistry *reg = m_collection->registry();
    // we must get the track by id, uid is not unique
    Meta::SqlTrackPtr track = Meta::SqlTrackPtr::staticCast( reg->getTrack( entry.id ) );
    Q_ASSERT( track->urlId() == entry.id );
    track->remove();
}

void
SqlScanResultProcessor::urlsCacheInit()
{
    DEBUG_BLOCK
    SqlStorage *storage = m_collection->sqlStorage();

    QString query = QString( "SELECT id, deviceid, rpath, directory, uniqueid FROM urls;");
    QStringList res = storage->query( query );

    for( int i = 0; i < res.count(); )
    {
        int id = res.at(i++).toInt();
        int deviceId = res.at(i++).toInt();
        QString rpath = res.at(i++);
        int directoryId = res.at(i++).toInt();
        QString uid = res.at(i++);

        QString path;
        if( deviceId )
            path = m_collection->mountPointManager()->getAbsolutePath( deviceId, rpath );
        else
            path = rpath;

        UrlEntry entry;
        entry.id = id;
        entry.path = path;
        entry.directoryId = directoryId;
        entry.uid = uid;

        if( !directoryId )
        {
            warning() << "Found urls entry without directory. A phantom track. Removing" << path;
            removeTrack( entry );
            continue;
        }

        urlsCacheInsert( entry );
    }
}

void
SqlScanResultProcessor::urlsCacheInsert( const UrlEntry &entry )
{
    // this case is normal operation
    if( m_urlsCache.contains( entry.id ) )
        urlsCacheRemove( m_urlsCache[ entry.id ] );

    // following shoudn't normally happen:
    if( m_pathCache.contains( entry.path ) )
    {
        int oldId = m_pathCache.value( entry.path );
        Q_ASSERT( m_urlsCache.contains( oldId ) );
        const UrlEntry &old = m_urlsCache[ oldId ];
        warning() << "urlsCacheInsert(): found duplicate in path. old" << old
                  << "will be hidden by the new one in the cache:" << entry;
    }

    // this will signify error in this class:
    Q_ASSERT( !m_uidCache.contains( entry.uid, entry.id ) );
    Q_ASSERT( !m_directoryCache.contains( entry.directoryId, entry.id ) );

    m_urlsCache.insert( entry.id, entry );
    m_uidCache.insert( entry.uid, entry.id );
    m_pathCache.insert( entry.path, entry.id );
    m_directoryCache.insert( entry.directoryId, entry.id );
}

void
SqlScanResultProcessor::urlsCacheRemove( const UrlEntry &entry )
{
    if( !m_urlsCache.contains( entry.id ) )
        return;

    m_uidCache.remove( entry.uid, entry.id );
    m_pathCache.remove( entry.path );
    m_directoryCache.remove( entry.directoryId, entry.id );
    m_urlsCache.remove( entry.id );
}

QDebug
operator<<( QDebug dbg, const SqlScanResultProcessor::UrlEntry &entry )
{
     dbg.nospace() << "Entry(id=" << entry.id << ", path=" << entry.path << ", dirId="
                   << entry.directoryId << ", uid=" << entry.uid << ")";
     return dbg.space();
}
