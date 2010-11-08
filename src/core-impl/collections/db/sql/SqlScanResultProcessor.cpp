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

#include "playlistmanager/PlaylistManager.h"

#include "core/support/Debug.h"

// include files from the collection scanner utility
#include <collectionscanner/Directory.h>
#include <collectionscanner/Album.h>
#include <collectionscanner/Track.h>
#include <collectionscanner/Playlist.h>

SqlScanResultProcessor::SqlScanResultProcessor( Collections::DatabaseCollection *collection, ScanType type, QObject *parent )
    : ScanResultProcessor( collection, type, parent ),
      m_sqlCollection( static_cast<Collections::SqlCollection*>( collection ) )
{
}

SqlScanResultProcessor::~SqlScanResultProcessor()
{
}

void
SqlScanResultProcessor::commitAlbum( const CollectionScanner::Album *album, int directoryId )
{
    debug() << "SRP::commitAlbum on"<<album->name()<< "artist"<<album->artist()<<"compilation"<<album->isCompilation();

    // --- get or create the album
    int albumId = -1;
    KSharedPtr<Meta::SqlAlbum> metaAlbum;
    if( album->isCompilation() )
        metaAlbum = KSharedPtr<Meta::SqlAlbum>::staticCast( m_sqlCollection->getAlbum( album->name(), QString() ) );
    else
        metaAlbum = KSharedPtr<Meta::SqlAlbum>::staticCast( m_sqlCollection->getAlbum( album->name(), album->artist() ) );
    albumId = metaAlbum->id();

    // --- add all tracks
    foreach( const CollectionScanner::Track &track, album->tracks() )
        commitTrack( &track, directoryId, albumId );

    // --- set the cover if we have one
    // we need to do this after the tracks are added in case of an embedded cover
    if( metaAlbum )
    {
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
    }
}

void
SqlScanResultProcessor::commitTrack( const CollectionScanner::Track *track, int directoryId, int albumId )
{
    Q_ASSERT( directoryId );
    Q_ASSERT( albumId ); // no track without album

    if( track->uniqueid().isEmpty() )
    {
        warning() << "got track with no unique id from the scanner, not adding";
        return;
    }
    debug() << "SRP::commitTrack on " << track->path() << "for album" << albumId;

    int deviceId = m_sqlCollection->mountPointManager()->getIdForUrl( track->path() );
    QString rpath = m_sqlCollection->mountPointManager()->getRelativePath( deviceId, track->path() );

    KSharedPtr<Meta::SqlTrack> metaTrack;
    metaTrack = KSharedPtr<Meta::SqlTrack>::staticCast( m_sqlCollection->getTrackFromUid( track->uniqueid() ) );

    if( metaTrack )
    {
        metaTrack->setWriteFile( false ); // no need to write the tags back
        metaTrack->beginMetaDataUpdate();

        // check if there is an older track at the same position.
        KSharedPtr<Meta::SqlTrack> otherTrack;
        otherTrack = KSharedPtr<Meta::SqlTrack>::staticCast( m_sqlCollection->trackForUrl( track->path() ) );
        if( otherTrack && otherTrack != metaTrack )
            m_sqlCollection->registry()->deleteTrack( otherTrack->id() );

        metaTrack->setUrl( deviceId, rpath, directoryId );
    }
    else
    {
        metaTrack = KSharedPtr<Meta::SqlTrack>::staticCast( m_sqlCollection->getTrack( deviceId, rpath, directoryId, track->uniqueid() ) );

        metaTrack->setWriteFile( false ); // no need to write the tags back
        metaTrack->beginMetaDataUpdate();

        metaTrack->setUidUrl( track->uniqueid() );
    }

    // TODO: we need to check the modified date of the file agains the last updated of the file
    // to figure out if the track information was updated from outside Amarok.
    // In such a case we would fully reread all the information as if in a FullScan

    // -- set the values

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
        metaTrack->setYear( track->year() );

    if( m_type == FullScan ||
        !track->genre().isEmpty() )
        metaTrack->setGenre( track->genre() );

    // the filetype is not set or in the database.
    // Meta::SqlTrack uses the file extension.

    if( m_type == FullScan ||
        !track->bpm() >= 0 )
        metaTrack->setBpm( track->bpm() );

    if( m_type == FullScan ||
        !track->comment().isEmpty() )
        metaTrack->setComment( track->comment() );

    if( (m_type == FullScan || metaTrack->score() == 0) &&
        track->score() >= 0 )
        metaTrack->setScore( track->score() * 100.0 );

    if( (m_type == FullScan || metaTrack->rating() == 0.0) &&
        track->rating() >= 0 )
        metaTrack->setRating( track->rating() * 10.0 );

    if( (m_type == FullScan || metaTrack->length() == 0) &&
        track->length() >= 0 )
        metaTrack->setLength( track->length() );

    // the filesize is updated every time after the
    // file is changed. Doesn't make sense to set it.

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

    if( m_type == FullScan &&
        track->playcount() >= 0 )
        metaTrack->setPlayCount( track->playcount() );


    Meta::ReplayGainTag modes[] = { Meta::ReplayGain_Track_Gain,
        Meta::ReplayGain_Track_Peak,
        Meta::ReplayGain_Album_Gain,
        Meta::ReplayGain_Album_Peak };

    for( int i=0; i<4; i++ )
        if( !track->replayGain( modes[i] ) != 0 )
            metaTrack->setReplayGain( modes[i], track->replayGain( modes[i] ) );

    metaTrack->endMetaDataUpdate();
    metaTrack->setWriteFile( true );

    m_foundTracks.insert( metaTrack->id() );
}



void
SqlScanResultProcessor::deleteDeletedDirectories()
{
    SqlStorage *storage = m_sqlCollection->sqlStorage();

    // -- get all directories
    QString query = QString( "SELECT id FROM directories;" );
    QStringList res = storage->query( query );

    // -- check if the have been found during the scan
    for( int i = 0; i < res.count(); )
    {
        int dirId = res.at(i++).toInt();
        if( !m_foundDirectories.contains( dirId ) )
        {
            debug() << "deleteDeletedDirectories" << dirId;

            deleteDeletedTracks( dirId );
            query = QString( "DELETE FROM directories WHERE id = %1;" ).arg( dirId );
            storage->query( query );
        }
    }
}

void
SqlScanResultProcessor::deleteDeletedTracks( int dirId )
{
    SqlStorage *storage = m_sqlCollection->sqlStorage();

    // -- find all tracks
    QString query = QString( "SELECT tracks.id, urls.deviceid, urls.rpath "
                             "FROM urls INNER JOIN tracks ON urls.id = tracks.url "
                             "WHERE  urls.directory = %1;" ).arg( dirId );
    QStringList res = storage->query( query );

    // -- check if the tracks have been found during the scan
    for( int i = 0; i < res.count(); )
    {
        int trackId = res.at(i++).toInt();
        int deviceId = res.at(i++).toInt();
        QString rpath = res.at(i++);

        if( !m_foundTracks.contains( trackId ) )
        {
            QString url = m_sqlCollection->mountPointManager()->getAbsolutePath( deviceId, rpath );
            debug() << "deleteDeletedTracks" << url <<"id"<< trackId;
            m_sqlCollection->registry()->deleteTrack( trackId );
        }
    }
}


#include "SqlScanResultProcessor.moc"

