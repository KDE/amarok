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

#define DEBUG_PREFIX "SqlScanResultProcessor"

#include "SqlScanResultProcessor.h"

#include "core/support/Debug.h"
#include "SqlQueryMaker.h"
#include "playlistmanager/PlaylistManager.h"

// include files from the collection scanner utility
#include <collectionscanner/Directory.h>
#include <collectionscanner/Album.h>
#include <collectionscanner/Track.h>
#include <collectionscanner/Playlist.h>

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

    // -- fill the registry cache with all the tracks
    // count the non skipped directories to find out if we should buffer all tracks before committing.
    int nonSkippedDirectories = 0;
    foreach( const CollectionScanner::Directory* dir, m_directories )
        if( !dir->isSkipped() )
        {
            debug() << "in commit, dir not skipped" << dir->path();
            nonSkippedDirectories++;
        }

    /*
    if( nonSkippedDirectories > 0 && (m_directories.count() / nonSkippedDirectories < 3) )
    {
        debug() << "in commit, buffering all tracks";
        // ok. enough directories changed. Use the query manager to read
        // all the tracks into the cache in one go.
        // that saves us a lot of single database queries later
        Collections::SqlQueryMaker *qm = static_cast< Collections::SqlQueryMaker* >( m_collection->queryMaker() );
        qm->setQueryType( Collections::QueryMaker::Track );
        qm->setBlocking( true );
        qm->run();
        qm->tracks( m_collection->collectionId() );
        delete qm;
    }
    */

    cacheUrlsInit();

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
    // --- updated the directory entry
    int dirId = m_collection->registry()->getDirectory( directory->path(), directory->mtime() );
    m_directoryIds.insert( directory, dirId );
    m_foundDirectories.insert( dirId, directory );

    ScanResultProcessor::commitDirectory( directory );
}

void
SqlScanResultProcessor::commitAlbum( CollectionScanner::Album *album )
{
    // debug() << "SRP::commitAlbum on"<<album->name()<< "artist"<<album->artist();

    // --- get or create the album
    KSharedPtr<Meta::SqlAlbum> metaAlbum;
    metaAlbum = KSharedPtr<Meta::SqlAlbum>::staticCast( m_collection->getAlbum( album->name(), album->artist() ) );
    m_albumIds.insert( album, metaAlbum->id() );

    // --- add all tracks
    foreach( CollectionScanner::Track *track, album->tracks() )
        commitTrack( track, album );

    // --- set the cover if we have one
    // we need to do this after the tracks are added in case of an embedded cover
    if( metaAlbum )
    {
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
}

void
SqlScanResultProcessor::commitTrack( CollectionScanner::Track *track,
                                     CollectionScanner::Album *srcAlbum )
{
    Q_ASSERT( srcAlbum );

    int directoryId = m_directoryIds.value( track->directory() );
    int albumId = m_albumIds.value( srcAlbum );

    QString uid = track->uniqueid();
    if( uid.isEmpty() )
    {
        warning() << "got track with no unique id from the scanner, not adding";
        m_lastErrors.append( QString("Not adding track %1 because it has no unique id").
                             arg(track->path()) );
        return;
    }
    uid = m_collection->uidUrlProtocol() + "://" + uid;

    int deviceId = m_collection->mountPointManager()->getIdForUrl( track->path() );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, track->path() );

    if( m_foundTracks.contains( uid ) )
    {
        warning() << "track"<<track->path()<<"with uid"<<uid<<"already committed. There seems to be a duplicate uid.";
        m_lastErrors.append( QString("Track %1 with uid %2 already committed. There seems to be a duplicate uid.").
                                     arg(track->path(), uid) );
        return;
    }
    m_foundTracks.insert( uid );

    // --- find an existing track by uid
    KSharedPtr<Meta::SqlTrack> metaTrack;
    if( m_urlsCacheUid.contains( uid ) )
    {
        UrlEntry entry = m_urlsCache.value( m_urlsCacheUid.value( uid ) );
        cacheUrlsRemove( entry.id ); // remove the old cache entry
        entry.path = track->path();
        entry.directoryId = directoryId;
        cacheUrlsInsert( entry );

        // check if there is an older track at the same position.
        if( m_urlsCachePath.contains( track->path() ) )
        {
            const UrlEntry &otherEntry = m_urlsCache.value( m_urlsCachePath.value( track->path() ) );
            if( entry.id != otherEntry.id )
            {
                removeTrack( otherEntry.id, otherEntry.uid );
                cacheUrlsRemove( otherEntry.id );
            }
        }
        metaTrack = KSharedPtr<Meta::SqlTrack>::staticCast( m_collection->trackForUrl( uid ) );
    }
    // --- find an existing track by path or create a new one
    if( !metaTrack )
    {
        UrlEntry entry;
        entry.id = -1;
        entry.path = track->path();
        if( m_urlsCachePath.contains( track->path() ) )
        {
            UrlEntry entry = m_urlsCache.value( m_urlsCachePath.value( track->path() ) );
            cacheUrlsRemove( entry.id ); // remove the old cache entry
        }
        entry.uid = uid;
        entry.directoryId = directoryId;
        cacheUrlsInsert( entry ); // and insert it again (or new)

        metaTrack = KSharedPtr<Meta::SqlTrack>::staticCast( m_collection->getTrack( deviceId, rpath, directoryId, uid ) );
    }

    if( !metaTrack )
    {
        warning() << "Something went wrong when importing track"<<track->path();
        return;
    }

    // TODO: we need to check the modified date of the file agains the last updated of the file
    // to figure out if the track information was updated from outside Amarok.
    // In such a case we would fully reread all the information as if in a FullScan

    // -- set the values
    metaTrack->setWriteFile( false ); // no need to write the tags back
    metaTrack->beginMetaDataUpdate();

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

    metaTrack->endMetaDataUpdate();
    metaTrack->setWriteFile( true );
}



void
SqlScanResultProcessor::deleteDeletedDirectories()
{
    SqlStorage *storage = m_collection->sqlStorage();

    // -- get a list of all mounted device ids
    QList<int> idList = m_collection->mountPointManager()->getMountedDeviceIds();
    QString deviceIds;
    foreach( int id, idList )
    {
        if ( !deviceIds.isEmpty() ) deviceIds += ',';
        deviceIds += QString::number( id );
    }

    // -- get all (mounted) directories
    QString query = QString( "SELECT id FROM directories "
                             "WHERE deviceid IN (%1);").arg( deviceIds );

    QStringList res = storage->query( query );

    // -- check if the have been found during the scan
    for( int i = 0; i < res.count(); )
    {
        int dirId = res.at(i++).toInt();
        if( !m_foundDirectories.contains( dirId ) )
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
    int directoryId = m_directoryIds.value( directory );
    deleteDeletedTracks( directoryId );
}

void
SqlScanResultProcessor::deleteDeletedTracks( int directoryId )
{
    // -- find all tracks
    QList<int> urlIds = m_urlsCacheDirectory.values( directoryId );

    // -- check if the tracks have been found during the scan
    foreach( int urlId, urlIds )
    {
        QString uid = m_urlsCache.value( urlId ).uid;

        if( !m_foundTracks.contains( uid ) )
        {
            removeTrack( urlId, uid );
            cacheUrlsRemove( urlId );
        }
    }
}

void
SqlScanResultProcessor::removeTrack( int urlId, const QString uid )
{
    debug() << "deleteTrack" << uid <<"url id"<< urlId;
    if( m_collection->registry()->m_uidMap.contains( uid ) )
        static_cast<Meta::SqlTrack*>(const_cast<Meta::Track*>(m_collection->registry()->m_uidMap.value( uid ).data()))->remove();
    else
        m_collection->registry()->removeTrack( urlId, uid );
}


void
SqlScanResultProcessor::cacheUrlsInit()
{
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

        if( !directoryId && !rpath.isEmpty() )
        {
            warning() << "Found urls entry without directory. A phantom track. Removing"<<rpath;
            removeTrack( id, uid );
        }

        QString path = m_collection->mountPointManager()->getAbsolutePath( deviceId, rpath );

        UrlEntry entry;
        entry.id = id;
        entry.path = path;
        entry.directoryId = directoryId;
        entry.uid = uid;

        cacheUrlsInsert( entry );
    }
}

void
SqlScanResultProcessor::cacheUrlsInsert( UrlEntry entry )
{
    if( !m_urlsCache.contains( entry.id ) )
        cacheUrlsRemove( entry.id );

    if( !entry.path.isEmpty() && m_urlsCachePath.contains( entry.path ) ) {
        // no idea how this can happen, but we clean it up
        debug() << "Duplicate path in database:"<<entry.path;
        removeTrack( entry.id, entry.uid ); // this will not delete the statistics
        entry.path.clear();
    }

    m_urlsCache.insert( entry.id, entry );
    m_urlsCacheUid.insert( entry.uid, entry.id );
    m_urlsCachePath.insert( entry.path, entry.id );
    m_urlsCacheDirectory.insert( entry.directoryId, entry.id );
}

void
SqlScanResultProcessor::cacheUrlsRemove( int id )
{
    if( !m_urlsCache.contains( id ) )
        return;

    const UrlEntry &entry = m_urlsCache.value( id );
    m_urlsCacheUid.remove( entry.uid );
    m_urlsCachePath.remove( entry.path );
    m_urlsCacheDirectory.remove( entry.directoryId, id );
    m_urlsCache.remove( id );
}

#include "SqlScanResultProcessor.moc"

