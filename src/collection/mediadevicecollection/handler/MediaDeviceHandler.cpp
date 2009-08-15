/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MediaDeviceHandler.h"

#include "MediaDeviceCollection.h"

#include "MediaDeviceHandlerCapability.h"

#include "playlist/MediaDeviceUserPlaylistProvider.h"
#include "playlistmanager/PlaylistManager.h"

#include "playlist/MediaDevicePlaylist.h"

#include <KMessageBox>
#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/JobCollection.h>

using namespace Meta;

bool
MetaHandlerCapability::hasCapabilityInterface( Handler::Capability::Type type ) const
{
    Q_UNUSED( type );
    return false;
}

Handler::Capability*
MetaHandlerCapability::createCapabilityInterface( Handler::Capability::Type type )
{
    Q_UNUSED( type );
    return 0;
}

MediaDeviceHandler::MediaDeviceHandler( QObject *parent )
    : QObject( parent )
    , m_memColl( qobject_cast<MediaDeviceCollection*>(parent) )
    , m_provider( 0 )
    , m_isCopying( false )
    , m_isDeleting( false )
    , m_pc( 0 )
    , m_rcb( 0 )
    , m_crc( 0 )
    , m_rc( 0 )
    , m_wcb( 0 )
    , m_wc( 0 )
{
    DEBUG_BLOCK

    connect( m_memColl, SIGNAL( deletingCollection() ),
             this,      SLOT( slotDeletingHandler() ), Qt::QueuedConnection );

    connect( this, SIGNAL( incrementProgress() ),
             The::statusBar(), SLOT( incrementProgress() ), Qt::QueuedConnection );
    connect( this, SIGNAL(endProgressOperation(const QObject*)),
             The::statusBar(), SLOT(endProgressOperation(const QObject*)));
    connect( this, SIGNAL( databaseWritten(bool)),
             this, SLOT( slotDatabaseWritten(bool)), Qt::QueuedConnection );
}

MediaDeviceHandler::~MediaDeviceHandler()
{
    DEBUG_BLOCK
    delete m_provider;
}

void
MediaDeviceHandler::slotDeletingHandler()
{
    DEBUG_BLOCK
    The::playlistManager()->removeProvider( m_provider );
}

void
MediaDeviceHandler::getBasicMediaDeviceTrackInfo( const Meta::MediaDeviceTrackPtr &srcTrack, Meta::MediaDeviceTrackPtr destTrack )
{
    /* 1-liner info retrieval */
    destTrack->setTitle( m_rc->libGetTitle( srcTrack ) );
    destTrack->setLength( m_rc->libGetLength( srcTrack ) );
    destTrack->setTrackNumber( m_rc->libGetTrackNumber( srcTrack ) );
    destTrack->setComment( m_rc->libGetComment( srcTrack ) );
    destTrack->setDiscNumber( m_rc->libGetDiscNumber( srcTrack ) );
    destTrack->setBitrate( m_rc->libGetBitrate( srcTrack ) );
    destTrack->setSamplerate( m_rc->libGetSamplerate( srcTrack ) );
    destTrack->setBpm( m_rc->libGetBpm( srcTrack ) );
    destTrack->setFileSize( m_rc->libGetFileSize( srcTrack ) );
    destTrack->setPlayCount( m_rc->libGetPlayCount( srcTrack ) );
    destTrack->setLastPlayed( m_rc->libGetLastPlayed( srcTrack ) );
    destTrack->setRating( m_rc->libGetRating( srcTrack ) );

    destTrack->setPlayableUrl( m_rc->libGetPlayableUrl( srcTrack ) );

    destTrack->setType( m_rc->libGetType( srcTrack ) );
}

void
MediaDeviceHandler::getBasicMediaDeviceTrackInfo( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr destTrack )
{
    /* 1-liner info retrieval */
    destTrack->setTitle( srcTrack->name() );
    destTrack->setLength( srcTrack->length() );
    destTrack->setTrackNumber( srcTrack->trackNumber() );
    destTrack->setComment( srcTrack->comment() );
    destTrack->setDiscNumber( srcTrack->discNumber() );
    destTrack->setBitrate( srcTrack->bitrate() );
    destTrack->setSamplerate( srcTrack->sampleRate() );
    destTrack->setFileSize( srcTrack->filesize() );
    destTrack->setPlayCount( srcTrack->lastPlayed() );
    destTrack->setLastPlayed( srcTrack->lastPlayed() );
    destTrack->setRating( srcTrack->rating() );

    destTrack->setPlayableUrl( srcTrack->playableUrl() );

    destTrack->setType( srcTrack->type() );
}

void
MediaDeviceHandler::setBasicMediaDeviceTrackInfo( const Meta::TrackPtr& srcTrack, MediaDeviceTrackPtr destTrack )
{
    DEBUG_BLOCK
    setupWriteCapability();

    if( !m_wc )
        return;

    m_wc->libSetTitle( destTrack, srcTrack->name() );
    if ( srcTrack->album() )
        m_wc->libSetAlbum( destTrack, srcTrack->album()->name() ); Debug::stamp();
    if ( srcTrack->artist() )
        m_wc->libSetArtist( destTrack, srcTrack->artist()->name() ); Debug::stamp();
    if ( srcTrack->composer() )
        m_wc->libSetComposer( destTrack, srcTrack->composer()->name() ); Debug::stamp();
    if ( srcTrack->genre() )
        m_wc->libSetGenre( destTrack, srcTrack->genre()->name() ); Debug::stamp();
    if ( srcTrack->year() )
        m_wc->libSetYear( destTrack, srcTrack->year()->name() ); Debug::stamp();
    m_wc->libSetLength( destTrack, srcTrack->length() ); Debug::stamp();
    m_wc->libSetTrackNumber( destTrack, srcTrack->trackNumber() ); Debug::stamp();
    m_wc->libSetComment( destTrack, srcTrack->comment() ); Debug::stamp();
    m_wc->libSetDiscNumber( destTrack, srcTrack->discNumber() ); Debug::stamp();
    m_wc->libSetBitrate( destTrack, srcTrack->bitrate() ); Debug::stamp();
    m_wc->libSetSamplerate( destTrack, srcTrack->sampleRate() ); Debug::stamp();
    //libSetBpm( destTrack, srcTrack->bpm() );
    m_wc->libSetFileSize( destTrack, srcTrack->filesize() ); Debug::stamp();
    m_wc->libSetPlayCount( destTrack, srcTrack->playCount() ); Debug::stamp();
    m_wc->libSetLastPlayed( destTrack, srcTrack->lastPlayed() ); Debug::stamp();
    m_wc->libSetRating( destTrack, srcTrack->rating() ); Debug::stamp();
    m_wc->libSetType( destTrack, srcTrack->type() ); Debug::stamp();
    //libSetPlayableUrl( destTrack, srcTrack );

    //if( srcTrack->album()->hasImage() )
    //    libSetCoverArt( destTrack, srcTrack->album()->image() );
}

void
MediaDeviceHandler::addMediaDeviceTrackToCollection(Meta::MediaDeviceTrackPtr& track)
{
    DEBUG_BLOCK

    setupReadCapability();

    if( !m_rcb )
        return;

    TrackMap trackMap = m_memColl->trackMap();
    ArtistMap artistMap = m_memColl->artistMap();
    AlbumMap albumMap = m_memColl->albumMap();
    GenreMap genreMap = m_memColl->genreMap();
    ComposerMap composerMap = m_memColl->composerMap();
    YearMap yearMap = m_memColl->yearMap();

    debug() << "1";

    /* 1-liner info retrieval */

    //Meta::TrackPtr srcTrack = Meta::TrackPtr::staticCast( track );

    //getBasicMediaDeviceTrackInfo( srcTrack, track );

    /* map-related info retrieval */
    setupArtistMap( track, artistMap );
    setupAlbumMap( track, albumMap );
    setupGenreMap( track, genreMap );
    setupComposerMap( track, composerMap );
    setupYearMap( track, yearMap );

    /* trackmap also soon to be subordinated */
    trackMap.insert( track->uidUrl(), TrackPtr::staticCast( track ) );

    m_titlemap.insert( track->name(), TrackPtr::staticCast( track ) );

    // Finally, assign the created maps to the collection
    m_memColl->acquireWriteLock();
    m_memColl->setTrackMap( trackMap );
    m_memColl->setArtistMap( artistMap );
    m_memColl->setAlbumMap( albumMap );
    m_memColl->setGenreMap( genreMap );
    m_memColl->setComposerMap( composerMap );
    m_memColl->setYearMap( yearMap );
    m_memColl->releaseLock();
}

void
MediaDeviceHandler::removeMediaDeviceTrackFromCollection( Meta::MediaDeviceTrackPtr &track )
{
    TrackMap trackMap = m_memColl->trackMap();
    ArtistMap artistMap = m_memColl->artistMap();
    AlbumMap albumMap = m_memColl->albumMap();
    GenreMap genreMap = m_memColl->genreMap();
    ComposerMap composerMap = m_memColl->composerMap();
    YearMap yearMap = m_memColl->yearMap();

    Meta::MediaDeviceArtistPtr artist = Meta::MediaDeviceArtistPtr::dynamicCast( track->artist() );
    Meta::MediaDeviceAlbumPtr album = Meta::MediaDeviceAlbumPtr::dynamicCast( track->album() );
    Meta::MediaDeviceGenrePtr genre = Meta::MediaDeviceGenrePtr::dynamicCast( track->genre() );
    Meta::MediaDeviceComposerPtr composer = Meta::MediaDeviceComposerPtr::dynamicCast( track->composer() );
    Meta::MediaDeviceYearPtr year = Meta::MediaDeviceYearPtr::dynamicCast( track->year() );

    // remove track from metadata's tracklists
    artist->remTrack( track );
    album->remTrack( track );
    genre->remTrack( track );
    composer->remTrack( track );
    year->remTrack( track );

    // if empty, get rid of metadata in general

    if( artist->tracks().isEmpty() )
    {
        artistMap.remove( artist->name() );
        m_memColl->acquireWriteLock();
        m_memColl->setArtistMap( artistMap );
        m_memColl->releaseLock();
    }
    if( album->tracks().isEmpty() )
    {
        albumMap.remove( album->name() );
        m_memColl->acquireWriteLock();
        m_memColl->setAlbumMap( albumMap );
        m_memColl->releaseLock();
    }
    if( genre->tracks().isEmpty() )
    {
        genreMap.remove( genre->name() );
        m_memColl->acquireWriteLock();
        m_memColl->setGenreMap( genreMap );
        m_memColl->releaseLock();
    }
    if( composer->tracks().isEmpty() )
    {
        composerMap.remove( composer->name() );
        m_memColl->acquireWriteLock();
        m_memColl->setComposerMap( composerMap );
        m_memColl->releaseLock();
    }
    if( year->tracks().isEmpty() )
    {
        yearMap.remove( year->name() );
        m_memColl->acquireWriteLock();
        m_memColl->setYearMap( yearMap );
        m_memColl->releaseLock();
    }

    // remove from trackmap
    trackMap.remove( track->name() );
    m_memColl->acquireWriteLock();
    m_memColl->setTrackMap( trackMap );
    m_memColl->releaseLock();
}

void
MediaDeviceHandler::getCopyableUrls(const Meta::TrackList &tracks)
{
    QMap<Meta::TrackPtr, KUrl> urls;
    foreach( Meta::TrackPtr track, tracks )
    {
        if( track->isPlayable() )
            urls.insert( track, track->playableUrl() );
    }

    emit gotCopyableUrls( urls );
}

void
MediaDeviceHandler::copyTrackListToDevice(const Meta::TrackList tracklist)
{
    QString copyErrorCaption = i18n( "Copying Tracks Failed" );

    if ( m_isCopying )
    {
        KMessageBox::error( 0, i18n( "Tracks not copied: the device is already being copied to" ), copyErrorCaption );
        return;
    }

    DEBUG_BLOCK

    setupWriteCapability();

    if( !m_wcb )
        return;

    m_isCopying = true;

    bool isDupe;
    bool hasDupe;
    QString format;
    TrackMap trackMap = m_memColl->trackMap();

    Meta::TrackList tempTrackList;

    m_copyFailed = false;

    hasDupe = false;

    m_tracksFailed.clear();

    // Clear Transfer queue
    m_tracksToCopy.clear();

    // Check for same tags, don't copy if same tags
    // Also check for compatible format
    foreach( Meta::TrackPtr track, tracklist )
    {
        // Check for compatible formats
        format = track->type();

        if( !m_wcb->supportedFormats().contains( format ) )
        {
             const QString error = i18n("Unsupported format: %1", format);
             m_tracksFailed.insert( track, error );
             continue;
        }

        tempTrackList = m_titlemap.values( track->name() );

        /* If no song with same title, already not a dupe */

        if( tempTrackList.isEmpty() )
        {
            debug() << "No tracks with same title, track not a dupe";
            m_tracksToCopy.append( track );
            continue;
        }

        /* Songs with same title present, check other tags */
        isDupe = false;

        foreach( Meta::TrackPtr tempTrack, tempTrackList )
        {
            if( ( tempTrack->artist()->name() != track->artist()->name() )
                || ( tempTrack->album()->name() != track->album()->name() )
                || ( tempTrack->genre()->name() != track->genre()->name() )
                || ( tempTrack->composer()->name() != track->composer()->name() )
                || ( tempTrack->year()->name() != track->year()->name() ) )
            {
                debug() << "Same title, but other tags differ, not a dupe";
                //debug() << "Source track:   " << "Artist: " << track->artist()->name() << " Album: " << track->album()->name() << " Genre: " << track->genre()->name() <<
                //" Composer: " << track->composer()->name() << " Year: " << track->year()->name();
                //debug() << "Candidate dupe: " << "Artist: " << tempTrack->artist()->name() << " Album: " << tempTrack->album()->name() << " Genre: " << tempTrack->genre()->name() <<
                //" Composer: " << tempTrack->composer()->name() << " Year: " << tempTrack->year()->name();
                continue;
            }

            // Track is already on there, break
            isDupe = true;
            hasDupe = true;
            break;
        }

        if( !isDupe )
            m_tracksToCopy.append( track );
        else
        {
            debug() << "Track " << track->name() << " is a dupe!";

            QString error = i18n("Already on device");
            m_tracksFailed.insert( track, error );
        }
    }

    // NOTE: see comment at top of copyTrackListToDevice
    if( hasDupe )
        m_copyFailed = true;

    /* List ready, begin copying */

    // Do not bother copying 0 tracks
    // This could happen if all tracks to copy are dupes

    if( m_tracksToCopy.size() == 0 )
    {
        KMessageBox::error( 0, i18n( "Tracks not copied: the device already has these tracks" ), copyErrorCaption );
        m_isCopying = false;
        emit copyTracksDone( false );
        return;
    }

    // Check for available space, in bytes, after the copy

    int transfersize = 0;

    foreach( Meta::TrackPtr track, m_tracksToCopy )
        transfersize += track->filesize();

    // NOTE: if the device will not have more than 5 MB to spare, abort the copy
    if( !( (freeSpace() - transfersize) > 1024*1024*5 ) )
    {
        debug() << "Free space: " << freeSpace();
        debug() << "Space would've been after copy: " << (freeSpace() - transfersize);
        KMessageBox::error( 0, i18n( "Tracks not copied: the device has insufficient space" ), copyErrorCaption );
        m_isCopying = false;
        emit copyTracksDone( false );
        return;
    }
    debug() << "Copying " << m_tracksToCopy.size() << " tracks";

    // Set up progress bar

    m_statusbar = The::statusBar()->newProgressOperation( this, i18n( "Transferring Tracks to Device" ) );

    m_statusbar->setMaximum( m_tracksToCopy.size() );

    // prepare to copy

    m_wcb->prepareToCopy();

    m_numTracksToCopy = m_tracksToCopy.count();
    m_tracksCopying.clear();
    m_trackSrcDst.clear();

    // begin copying tracks to device

    if( !m_copyingthreadsafe )
        copyNextTrackToDevice();
    else
        enqueueNextCopyThread();
}

void
MediaDeviceHandler::copyNextTrackToDevice()
{
    Meta::TrackPtr track;

    // If there are more tracks to copy, copy the next one
    if ( !m_tracksToCopy.isEmpty() )
    {
        // Pop the track off the front of the list
        track = m_tracksToCopy.first();
        m_tracksToCopy.removeFirst();

        // Copy the track
        privateCopyTrackToDevice( track );
    }
}


bool
MediaDeviceHandler::privateCopyTrackToDevice( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    bool success = false;

    // Create new destTrack that will go into the device collection, based on source track

    Meta::MediaDeviceTrackPtr destTrack ( new Meta::MediaDeviceTrack( m_memColl ) );

    // find path to copy to

    m_wcb->findPathToCopy( track, destTrack );

    if( !isOrganizable() )
    {
        // Create a track struct, associate it to destTrack

        m_wc->libCreateTrack( destTrack );

        // Fill the track struct of the destTrack with info from the track parameter as source

        setBasicMediaDeviceTrackInfo( track, destTrack );

        // set up the play url

        m_wc->libSetPlayableUrl( destTrack, track );

    }

    // Fill metadata of destTrack too with the same info

    getBasicMediaDeviceTrackInfo( track, destTrack );

    m_trackSrcDst[ track ] = destTrack; // associate source with destination, for finalizing copy

    // Copy the file to the device

    success = m_wcb->libCopyTrack( track, destTrack );

    return success;
}

/// @param track is the source track from which we are copying
void
MediaDeviceHandler::slotFinalizeTrackCopy( const Meta::TrackPtr & track )
{
    DEBUG_BLOCK
    //m_tracksCopying.removeOne( track );

    Meta::MediaDeviceTrackPtr destTrack = m_trackSrcDst[ track ];

    if( !isOrganizable() )
    {
        // Add the track struct into the database, if the library needs to

        m_wc->addTrackInDB( destTrack );

        // Inform subclass that a track has been added to the db

        m_wc->databaseChanged();
    }

    // Add the new Meta::MediaDeviceTrackPtr into the device collection

    // add track to collection
    addMediaDeviceTrackToCollection( destTrack );

    emit incrementProgress();

    m_numTracksToCopy--;

    debug() << "Tracks left to copy after this one is now done: " << m_numTracksToCopy;

    if( m_numTracksToCopy == 0 )
    {
        if( m_tracksFailed.size() > 0 )
        {
            The::statusBar()->shortMessage( i18np( "%1 track failed to copy to the device",
                                                   "%1 tracks failed to copy to the device", m_tracksFailed.size() ) );
        }
        // clear maps/hashes used

        m_tracksCopying.clear();
        m_trackSrcDst.clear();
        m_tracksFailed.clear();
        m_tracksToCopy.clear();

        // copying done

        m_isCopying = false;
        emit copyTracksDone( true );
    }
}

void
MediaDeviceHandler::slotCopyTrackFailed( const Meta::TrackPtr & track )
{
    DEBUG_BLOCK
    emit incrementProgress();

    m_numTracksToCopy--;

    QString error = i18n( "The track failed to copy to the device" );

    m_tracksFailed.insert( track, error );
}

void
MediaDeviceHandler::removeTrackListFromDevice( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK

    QString removeError = i18n( "Tracks not deleted:" );
    QString removeErrorCaption = i18n( "Deleting Tracks Failed" );

    if ( m_isDeleting )
    {
        KMessageBox::error( 0, i18np( "%1 Track is already being deleted from the device",
                                      "%1 Tracks are already being deleted from the device", removeError ), removeErrorCaption );
        return;
    }

    setupWriteCapability();

    if( !m_wcb )
        return;

    m_isDeleting = true;

    // Init the list of tracks to be deleted

    m_tracksToDelete = tracks;

    // Set up statusbar for deletion operation

    m_statusbar = The::statusBar()->newProgressOperation( this, i18n( "Removing Tracks from Device" ) );

    m_statusbar->setMaximum( tracks.size() );

    m_wcb->prepareToDelete();

    m_numTracksToRemove = m_tracksToDelete.count();

    removeNextTrackFromDevice();
}

void
MediaDeviceHandler::removeNextTrackFromDevice()
{
    DEBUG_BLOCK
    Meta::TrackPtr track;
    // If there are more tracks to remove, remove the next one
    if( !m_tracksToDelete.isEmpty() )
    {
        // Pop the track off the front of the list

        track = m_tracksToDelete.first();
        m_tracksToDelete.removeFirst();

        // Remove the track

        privateRemoveTrackFromDevice( track );
    }
}

void
MediaDeviceHandler::privateRemoveTrackFromDevice( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    Meta::MediaDeviceTrackPtr devicetrack = Meta::MediaDeviceTrackPtr::staticCast( track );

    // Remove the physical file from the device, perhaps using a libcall, or KIO

    m_wcb->libDeleteTrackFile( devicetrack );


}

void
MediaDeviceHandler::slotFinalizeTrackRemove( const Meta::TrackPtr & track )
{
    DEBUG_BLOCK
    Meta::MediaDeviceTrackPtr devicetrack = Meta::MediaDeviceTrackPtr::staticCast( track );

    if( !isOrganizable() )
    {
        // Remove the track struct from the db, references to it

        m_wc->removeTrackFromDB( devicetrack );

        // delete the struct associated with this track

        m_wc->libDeleteTrack( devicetrack );

        // Inform subclass that a track has been removed from

        m_wc->databaseChanged();
    }

    // remove from titlemap

    m_titlemap.remove( track->name(), track );

    // remove from collection

    removeMediaDeviceTrackFromCollection( devicetrack );

    emit incrementProgress();

    m_numTracksToRemove--;

    if( m_numTracksToRemove == 0 )
    {
        /*
        if( m_tracksFailed.size() > 0 )
        {
            The::statusBar()->shortMessage( i18n( "%1 tracks failed to copy to the device", m_tracksFailed.size() ) );
        }
        */
        m_wcb->endTrackRemove();
        debug() << "Done removing tracks";
        m_isDeleting = false;
        emit removeTracksDone();
    }
}

void
MediaDeviceHandler::slotDatabaseWritten( bool success )
{
    DEBUG_BLOCK
    Q_UNUSED( success )

    emit endProgressOperation( this );

    m_memColl->collectionUpdated();
}


void
MediaDeviceHandler::setupArtistMap( Meta::MediaDeviceTrackPtr track, ArtistMap& artistMap )
{
    DEBUG_BLOCK
    QString artist( m_rcb->libGetArtist( track ) );
    MediaDeviceArtistPtr artistPtr;

    if ( artistMap.contains( artist ) )
        artistPtr = MediaDeviceArtistPtr::staticCast( artistMap.value( artist ) );
    else
    {
        artistPtr = MediaDeviceArtistPtr( new MediaDeviceArtist( artist ) );
        artistMap.insert( artist, ArtistPtr::staticCast( artistPtr ) );
    }

    artistPtr->addTrack( track );
    track->setArtist( artistPtr );
}

void
MediaDeviceHandler::setupAlbumMap( Meta::MediaDeviceTrackPtr track, AlbumMap& albumMap )
{
    QString album( m_rcb->libGetAlbum( track ) );
    MediaDeviceAlbumPtr albumPtr;

    if ( albumMap.contains( album ) )
        albumPtr = MediaDeviceAlbumPtr::staticCast( albumMap.value( album ) );
    else
    {
        albumPtr = MediaDeviceAlbumPtr( new MediaDeviceAlbum( m_memColl, album ) );
        albumMap.insert( album, AlbumPtr::staticCast( albumPtr ) );
    }

    albumPtr->addTrack( track );
    track->setAlbum( albumPtr );
}

void
MediaDeviceHandler::setupGenreMap( Meta::MediaDeviceTrackPtr track, GenreMap& genreMap )
{
    QString genre = m_rcb->libGetGenre( track );
    MediaDeviceGenrePtr genrePtr;

    if ( genreMap.contains( genre ) )
        genrePtr = MediaDeviceGenrePtr::staticCast( genreMap.value( genre ) );

    else
    {
        genrePtr = MediaDeviceGenrePtr( new MediaDeviceGenre( genre ) );
        genreMap.insert( genre, GenrePtr::staticCast( genrePtr ) );
    }

    genrePtr->addTrack( track );
    track->setGenre( genrePtr );
}

void
MediaDeviceHandler::setupComposerMap( Meta::MediaDeviceTrackPtr track, ComposerMap& composerMap )
{
    QString composer ( m_rcb->libGetComposer( track ) );
    MediaDeviceComposerPtr composerPtr;

    if ( composerMap.contains( composer ) )
        composerPtr = MediaDeviceComposerPtr::staticCast( composerMap.value( composer ) );
    else
    {
        composerPtr = MediaDeviceComposerPtr( new MediaDeviceComposer( composer ) );
        composerMap.insert( composer, ComposerPtr::staticCast( composerPtr ) );
    }

    composerPtr->addTrack( track );
    track->setComposer( composerPtr );
}

void
MediaDeviceHandler::setupYearMap( Meta::MediaDeviceTrackPtr track, YearMap& yearMap )
{
    QString year;
    year = year.setNum( m_rcb->libGetYear( track ) );
    MediaDeviceYearPtr yearPtr;
    if ( yearMap.contains( year ) )
        yearPtr = MediaDeviceYearPtr::staticCast( yearMap.value( year ) );
    else
    {
        yearPtr = MediaDeviceYearPtr( new MediaDeviceYear( year ) );
        yearMap.insert( year, YearPtr::staticCast( yearPtr ) );
    }
    yearPtr->addTrack( track );
    track->setYear( yearPtr );
}

void
MediaDeviceHandler::parseTracks()
{
    DEBUG_BLOCK

    setupReadCapability();
    if ( !m_rcb )
        return;

    TrackMap trackMap;
    ArtistMap artistMap;
    AlbumMap albumMap;
    GenreMap genreMap;
    ComposerMap composerMap;
    YearMap yearMap;

    /* iterate through tracklist and add to appropriate map */
    for( m_rcb->prepareToParseTracks(); !m_rcb->isEndOfParseTracksList(); m_rcb->prepareToParseNextTrack() )
    {
        /// Fetch next track to parse

        m_rcb->nextTrackToParse();

        MediaDeviceTrackPtr track( new MediaDeviceTrack( m_memColl ) );

        m_rcb->setAssociateTrack( track );

        if( m_rc != 0 )
        {
            getBasicMediaDeviceTrackInfo( track, track );
        }

        else if( m_crc != 0 )
        {
            Meta::TrackPtr srcTrack = m_crc->sourceTrack();
            getBasicMediaDeviceTrackInfo( srcTrack, track );
        }

        /* map-related info retrieval */
        setupArtistMap( track, artistMap );
        setupAlbumMap( track, albumMap );
        setupGenreMap( track, genreMap );
        setupComposerMap( track, composerMap );
        setupYearMap( track, yearMap );

        /* TrackMap stuff to be subordinated later */
        trackMap.insert( track->uidUrl(), TrackPtr::staticCast( track ) );

        // TODO: setup titlemap for copy/deleting
        m_titlemap.insert( track->name(), TrackPtr::staticCast( track ) );

        // TODO: abstract playlist parsing

        // Subscribe to Track for metadata updates
        subscribeTo( Meta::TrackPtr::staticCast( track ) );
    }


    if( !m_pc && this->hasCapabilityInterface( Handler::Capability::Playlist ) )
        m_pc = this->create<Handler::PlaylistCapability>();

    if( m_pc )
    {
        // Register the playlist provider with the playlistmanager

        // register a playlist provider for this type of device
        m_provider = new MediaDeviceUserPlaylistProvider();

        // Begin parsing the playlists
        Meta::MediaDevicePlaylistList playlists;

        for( m_pc->prepareToParsePlaylists(); !m_pc->isEndOfParsePlaylistsList(); m_pc->prepareToParseNextPlaylist() )
        {
            m_pc->nextPlaylistToParse();

            if( m_pc->shouldNotParseNextPlaylist() )
                continue;

            // Create a new track list

            Meta::TrackList tracklist;

            for ( m_pc->prepareToParsePlaylistTracks(); !(m_pc->isEndOfParsePlaylist()); m_pc->prepareToParseNextPlaylistTrack() )
            {
                m_pc->nextPlaylistTrackToParse();
                // Grab the track associated with the next struct
                Meta::TrackPtr track = Meta::TrackPtr::staticCast( m_pc->libGetTrackPtrForTrackStruct() );
                // if successful, add it into the list at the end.
                // it is assumed that the list has some presorted order
                // and this is left to the library

                if ( track )
                    tracklist << track;
            }

            // Make a playlist out of this tracklist
            Meta::MediaDevicePlaylistPtr playlist( new Meta::MediaDevicePlaylist( m_pc->libGetPlaylistName(), tracklist ) );

            m_pc->setAssociatePlaylist( playlist );

            // Insert the new playlist into the list of playlists
            //playlists << playlist;

            // Inform the provider of the new playlist
            m_provider->addPlaylist( playlist );
        }

        // When the provider saves a playlist, the handler should save it internally
        connect( m_provider, SIGNAL( playlistSaved( const Meta::MediaDevicePlaylistPtr &, const QString& ) ),
                 SLOT( savePlaylist( const Meta::MediaDevicePlaylistPtr &, const QString& ) ) );
        connect( m_provider, SIGNAL( playlistRenamed( const Meta::MediaDevicePlaylistPtr &) ),
                 SLOT( renamePlaylist( const Meta::MediaDevicePlaylistPtr & ) ) );
        connect( m_provider, SIGNAL( playlistsDeleted( const Meta::MediaDevicePlaylistList & ) ),
                 SLOT( deletePlaylists( const Meta::MediaDevicePlaylistList &  ) ) );

        The::playlistManager()->addProvider(  m_provider,  m_provider->category() );
        m_provider->sendUpdated();

    }

    // Finally, assign the created maps to the collection
    m_memColl->acquireWriteLock();
    m_memColl->setTrackMap( trackMap );
    m_memColl->setArtistMap( artistMap );
    m_memColl->setAlbumMap( albumMap );
    m_memColl->setGenreMap( genreMap );
    m_memColl->setComposerMap( composerMap );
    m_memColl->setYearMap( yearMap );
    m_memColl->releaseLock();

    m_memColl->collectionUpdated();
}

void
MediaDeviceHandler::slotCopyNextTrackFailed( ThreadWeaver::Job* job, const Meta::TrackPtr& track )
{
    Q_UNUSED( job );
    enqueueNextCopyThread();
    m_copyFailed = true;
    slotCopyTrackFailed( track );
}

void
MediaDeviceHandler::slotCopyNextTrackDone( ThreadWeaver::Job* job, const Meta::TrackPtr& track )
{
    Q_UNUSED( track )
    enqueueNextCopyThread();
    if ( job->success() )
        slotFinalizeTrackCopy( track );
    else
    {
        m_copyFailed = true;
        slotCopyTrackFailed( track );
    }
}

void
MediaDeviceHandler::enqueueNextCopyThread()
{
    Meta::TrackPtr track;

    // If there are more tracks to copy, copy the next one
    if( !m_tracksToCopy.isEmpty() )
    {
        // Pop the track off the front of the list

        track = m_tracksToCopy.first();
        m_tracksToCopy.removeFirst();

        // Copy the track
        ThreadWeaver::Weaver::instance()->enqueue(  new CopyWorkerThread( track,  this ) );
    }
}

void
MediaDeviceHandler::slotCopyTrackJobsDone( ThreadWeaver::Job* job )
{
    Q_UNUSED( job )
    // TODO: some error checking showing tracks that could not be copied

    // Finish the progress bar
    emit incrementProgress();
    emit endProgressOperation( this );

    // Inform CollectionLocation that copying is done
    m_isCopying = false;
    emit copyTracksDone( true );
}

float
MediaDeviceHandler::freeSpace() const
{
    if ( m_rcb )
        return ( m_rcb->totalCapacity() - m_rcb->usedCapacity() );
    else
        return 0.0;
}

float
MediaDeviceHandler::usedcapacity() const
{
    if ( m_rcb )
        return m_rcb->usedCapacity();
    else
        return 0.0;
}

float
MediaDeviceHandler::totalcapacity() const
{
    if ( m_rcb )
        return m_rcb->totalCapacity();
    else
        return 0.0;
}

void
MediaDeviceHandler::setDestinations( const QMap<Meta::TrackPtr, QString> &destinations )
{
    m_destinations.clear();
    m_destinations = destinations;
}

UserPlaylistProvider*
MediaDeviceHandler::provider()
{
    DEBUG_BLOCK
    return (qobject_cast<UserPlaylistProvider *>( m_provider ) );
}

void
MediaDeviceHandler::savePlaylist( const Meta::MediaDevicePlaylistPtr &playlist, const QString& name )
{
    DEBUG_BLOCK
    if( !m_pc )
    {
        if( this->hasCapabilityInterface( Handler::Capability::Playlist ) )
        {
            m_pc = this->create<Handler::PlaylistCapability>();
            if( !m_pc )
            {
                debug() << "Handler does not have MediaDeviceHandler::PlaylistCapability.";
            }
        }
    }

    if( m_pc )
    {
        m_pc->savePlaylist( playlist, name );
        writeDatabase();
    }
}

void
MediaDeviceHandler::renamePlaylist( const Meta::MediaDevicePlaylistPtr &playlist )
{
    DEBUG_BLOCK
    if( !m_pc )
    {
        if( this->hasCapabilityInterface( Handler::Capability::Playlist ) )
        {
            m_pc = this->create<Handler::PlaylistCapability>();
            if( !m_pc )
            {
                debug() << "Handler does not have MediaDeviceHandler::PlaylistCapability.";
            }
        }
    }

    if( m_pc )
    {
        debug() << "Renaming playlist";
        m_pc->renamePlaylist( playlist );
        writeDatabase();
    }
}

void
MediaDeviceHandler::deletePlaylists( const Meta::MediaDevicePlaylistList &playlistlist )
{
    DEBUG_BLOCK
    if( !m_pc )
    {
        if( this->hasCapabilityInterface( Handler::Capability::Playlist ) )
        {
            m_pc = this->create<Handler::PlaylistCapability>();
            if( !m_pc )
            {
                debug() << "Handler does not have MediaDeviceHandler::PlaylistCapability.";
            }
        }
    }

    if( m_pc )
    {
        debug() << "Deleting playlists";
        foreach( Meta::MediaDevicePlaylistPtr playlist, playlistlist )
        {
            m_pc->deletePlaylist( playlist );
        }

        writeDatabase();
    }
}

void
MediaDeviceHandler::setupReadCapability()
{
    DEBUG_BLOCK
    MediaDeviceHandler *handler = const_cast<MediaDeviceHandler*> ( this );
    if( !m_rcb )
    {
        debug() << "RCB does not exist";
        if( handler->hasCapabilityInterface( Handler::Capability::Readable ) )
        {
            debug() << "Has read capability interface";
            m_rcb = handler->create<Handler::ReadCapabilityBase>();
            m_rc = 0;
            m_crc = 0;
            if( m_rcb->inherits( "Handler::ReadCapability" ) )
            {
                debug() << "Making read capability";
                m_rc = qobject_cast<Handler::ReadCapability *>( m_rcb );
            }
            else if( m_rcb->inherits( "Handler::CustomReadCapability" ) )
            {
                debug() << "making custom read capability";
                m_crc = qobject_cast<Handler::CustomReadCapability *>( m_rcb );
            }
            debug() << "Created rc";
            if( !m_rcb )
            {
                debug() << "Handler does not have MediaDeviceHandler::ReadCapability. Aborting.";
            }
        }
    }
}

void
MediaDeviceHandler::setupWriteCapability()
{
    DEBUG_BLOCK
    if( !m_wcb )
    {
        debug() << "WCB does not exist";
        if( this->hasCapabilityInterface( Handler::Capability::Writable ) )
        {
            m_wcb = this->create<Handler::WriteCapabilityBase>();
            m_wc = 0;
            if( m_wcb->inherits( "Handler::WriteCapability" ) )
            {
                debug() << "Making write capability";
                m_wc = qobject_cast<Handler::WriteCapability *>( m_wcb );
            }
            if( !m_wcb )
            {
                debug() << "Handler does not have MediaDeviceHandler::WriteCapability. Aborting.";
                return;
            }
        }
    }
}

/** Observer Methods **/
void
MediaDeviceHandler::metadataChanged( TrackPtr track )
{
    DEBUG_BLOCK

    Meta::MediaDeviceTrackPtr trackPtr = Meta::MediaDeviceTrackPtr::staticCast( track );
    KUrl trackUrl = KUrl::fromPath( trackPtr->uidUrl() );

    setupWriteCapability();

    if( !m_wc )
        return;

    setBasicMediaDeviceTrackInfo( track, trackPtr );

    m_wc->updateTrack( trackPtr );
    m_wc->databaseChanged();
}

void
MediaDeviceHandler::metadataChanged( ArtistPtr artist )
{
    Q_UNUSED( artist );
}

void
MediaDeviceHandler::metadataChanged( AlbumPtr album )
{
    Q_UNUSED( album );
}

void
MediaDeviceHandler::metadataChanged( GenrePtr genre )
{
    Q_UNUSED( genre );
}

void
MediaDeviceHandler::metadataChanged( ComposerPtr composer )
{
    Q_UNUSED( composer );
}

void
MediaDeviceHandler::metadataChanged( YearPtr year )
{
    Q_UNUSED( year );
}

CopyWorkerThread::CopyWorkerThread( const Meta::TrackPtr &track, MediaDeviceHandler* handler )
        : ThreadWeaver::Job()
        , m_success( false )
        , m_track( track )
        , m_handler( handler )
{
    //connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), m_handler, SLOT( slotCopyNextTrackToDevice( ThreadWeaver::Job* ) ), Qt::QueuedConnection );
    connect( this, SIGNAL( failed( ThreadWeaver::Job* ) ), this, SLOT( slotDoneFailed( ThreadWeaver::Job* ) ), Qt::QueuedConnection );
    connect( this, SIGNAL( copyTrackFailed(ThreadWeaver::Job*, const Meta::TrackPtr&)), m_handler, SLOT( slotCopyNextTrackFailed( ThreadWeaver::Job*, const Meta::TrackPtr&) ) );
    connect( this, SIGNAL( copyTrackDone(ThreadWeaver::Job*, const Meta::TrackPtr&)), m_handler, SLOT( slotCopyNextTrackDone( ThreadWeaver::Job*, const Meta::TrackPtr&) ) );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( slotDoneSuccess(ThreadWeaver::Job*)) );

    //connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ) );
}

CopyWorkerThread::~CopyWorkerThread()
{
    //nothing to do
}

bool
CopyWorkerThread::success() const
{
    return m_success;
}

void
CopyWorkerThread::run()
{
    m_success = m_handler->privateCopyTrackToDevice( m_track );
}

void
CopyWorkerThread::slotDoneSuccess( ThreadWeaver::Job* )
{
    emit copyTrackDone( this, m_track );
}

void
CopyWorkerThread::slotDoneFailed( ThreadWeaver::Job* )
{
    emit copyTrackFailed( this, m_track );
}


#include "MediaDeviceHandler.moc"
