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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MediaDeviceHandler.h"

#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core-impl/collections/mediadevicecollection/MediaDeviceCollection.h"
#include "core-impl/collections/mediadevicecollection/handler/MediaDeviceHandlerCapability.h"
#include "core-impl/collections/support/ArtistHelper.h"
#include "playlistmanager/PlaylistManager.h"

#include <QSharedPointer>
#include <KMessageBox>
#include <ThreadWeaver/Queue>
#include <ThreadWeaver/ThreadWeaver>

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
    return nullptr;
}

MediaDeviceHandler::MediaDeviceHandler( QObject *parent )
    : QObject( parent )
    , m_memColl( qobject_cast<Collections::MediaDeviceCollection*>(parent) )
    , m_provider( nullptr )
    , m_isCopying( false )
    , m_isDeleting( false )
    , m_pc( nullptr )
    , m_rc( nullptr )
    , m_wc( nullptr )
{
    DEBUG_BLOCK

    connect( m_memColl, &Collections::MediaDeviceCollection::deletingCollection,
             this, &MediaDeviceHandler::slotDeletingHandler, Qt::QueuedConnection );

    connect( this, &MediaDeviceHandler::databaseWritten,
             this, &MediaDeviceHandler::slotDatabaseWritten, Qt::QueuedConnection );
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
    if( m_provider )
        The::playlistManager()->removeProvider( m_provider );
    m_memColl = nullptr;
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
    destTrack->setReplayGain( m_rc->libGetReplayGain( srcTrack ) );

    destTrack->setPlayableUrl( m_rc->libGetPlayableUrl( srcTrack ) );

    destTrack->setType( m_rc->libGetType( srcTrack ) );
}

void
MediaDeviceHandler::setBasicMediaDeviceTrackInfo( const Meta::TrackPtr& srcTrack, MediaDeviceTrackPtr destTrack )
{
    DEBUG_BLOCK
    if( !setupWriteCapability() )
        return;

    m_wc->libSetTitle( destTrack, srcTrack->name() );

    QString albumArtist;
    bool isCompilation = false;
    if ( srcTrack->album() )
    {
        AlbumPtr album = srcTrack->album();

        m_wc->libSetAlbum( destTrack, album->name() );
        isCompilation = album->isCompilation();
        m_wc->libSetIsCompilation( destTrack, isCompilation );
        if( album->hasAlbumArtist() )
            albumArtist = album->albumArtist()->name();

        if( album->hasImage() )
            m_wc->libSetCoverArt( destTrack, album->image() );
    }

    QString trackArtist;
    if ( srcTrack->artist() )
    {
        trackArtist = srcTrack->artist()->name();
        m_wc->libSetArtist( destTrack, trackArtist );
    }

    QString composer;
    if ( srcTrack->composer() )
    {
        composer = srcTrack->composer()->name();
        m_wc->libSetComposer( destTrack, composer );
    }

    QString genre;
    if ( srcTrack->genre() )
    {
        genre = srcTrack->genre()->name();
        m_wc->libSetGenre( destTrack, genre );
    }

    if( isCompilation && albumArtist.isEmpty() )
        // iPod doesn't handle empty album artist well for compilation albums (splits these albums)
        albumArtist = i18n( "Various Artists" );
    else
        albumArtist = ArtistHelper::bestGuessAlbumArtist( albumArtist, trackArtist, genre, composer );
    m_wc->libSetAlbumArtist( destTrack, albumArtist );

    if ( srcTrack->year() )
        m_wc->libSetYear( destTrack, srcTrack->year()->name() );
    m_wc->libSetLength( destTrack, srcTrack->length() );
    m_wc->libSetTrackNumber( destTrack, srcTrack->trackNumber() );
    m_wc->libSetComment( destTrack, srcTrack->comment() );
    m_wc->libSetDiscNumber( destTrack, srcTrack->discNumber() );
    m_wc->libSetBitrate( destTrack, srcTrack->bitrate() );
    m_wc->libSetSamplerate( destTrack, srcTrack->sampleRate() );
    m_wc->libSetBpm( destTrack, srcTrack->bpm() );
    m_wc->libSetFileSize( destTrack, srcTrack->filesize() );
    m_wc->libSetPlayCount( destTrack, srcTrack->statistics()->playCount() );
    m_wc->libSetLastPlayed( destTrack, srcTrack->statistics()->lastPlayed() );
    m_wc->libSetRating( destTrack, srcTrack->statistics()->rating() );
    // MediaDeviceTrack stores only track gain:
    m_wc->libSetReplayGain( destTrack, srcTrack->replayGain( Meta::ReplayGain_Track_Gain ) );
    m_wc->libSetType( destTrack, srcTrack->type() );
    //libSetPlayableUrl( destTrack, srcTrack );
}

void
MediaDeviceHandler::addMediaDeviceTrackToCollection( Meta::MediaDeviceTrackPtr& track )
{
    if( !setupReadCapability() )
        return;

    TrackMap trackMap = m_memColl->memoryCollection()->trackMap();
    ArtistMap artistMap = m_memColl->memoryCollection()->artistMap();
    AlbumMap albumMap = m_memColl->memoryCollection()->albumMap();
    GenreMap genreMap = m_memColl->memoryCollection()->genreMap();
    ComposerMap composerMap = m_memColl->memoryCollection()->composerMap();
    YearMap yearMap = m_memColl->memoryCollection()->yearMap();

    /* 1-liner info retrieval */

    //Meta::TrackPtr srcTrack = Meta::TrackPtr::staticCast( track );

    //getBasicMediaDeviceTrackInfo( srcTrack, track );

    /* map-related info retrieval */
    // NB: setupArtistMap _MUST_ be called before setupAlbumMap
    setupArtistMap( track, artistMap );
    setupAlbumMap( track, albumMap, artistMap );
    setupGenreMap( track, genreMap );
    setupComposerMap( track, composerMap );
    setupYearMap( track, yearMap );

    /* trackmap also soon to be subordinated */
    trackMap.insert( track->uidUrl(), TrackPtr::staticCast( track ) );

    m_titlemap.insert( track->name(), TrackPtr::staticCast( track ) );

    // Finally, assign the created maps to the collection
    m_memColl->memoryCollection()->acquireWriteLock();
    m_memColl->memoryCollection()->setTrackMap( trackMap );
    m_memColl->memoryCollection()->setArtistMap( artistMap );
    m_memColl->memoryCollection()->setAlbumMap( albumMap );
    m_memColl->memoryCollection()->setGenreMap( genreMap );
    m_memColl->memoryCollection()->setComposerMap( composerMap );
    m_memColl->memoryCollection()->setYearMap( yearMap );
    m_memColl->memoryCollection()->releaseLock();
}

void
MediaDeviceHandler::removeMediaDeviceTrackFromCollection( Meta::MediaDeviceTrackPtr &track )
{
    TrackMap trackMap = m_memColl->memoryCollection()->trackMap();
    ArtistMap artistMap = m_memColl->memoryCollection()->artistMap();
    AlbumMap albumMap = m_memColl->memoryCollection()->albumMap();
    GenreMap genreMap = m_memColl->memoryCollection()->genreMap();
    ComposerMap composerMap = m_memColl->memoryCollection()->composerMap();
    YearMap yearMap = m_memColl->memoryCollection()->yearMap();

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
        m_memColl->memoryCollection()->acquireWriteLock();
        m_memColl->memoryCollection()->setArtistMap( artistMap );
        m_memColl->memoryCollection()->releaseLock();
    }
    if( album->tracks().isEmpty() )
    {
        albumMap.remove( AlbumPtr::staticCast( album ) );
        m_memColl->memoryCollection()->acquireWriteLock();
        m_memColl->memoryCollection()->setAlbumMap( albumMap );
        m_memColl->memoryCollection()->releaseLock();
    }
    if( genre->tracks().isEmpty() )
    {
        genreMap.remove( genre->name() );
        m_memColl->memoryCollection()->acquireWriteLock();
        m_memColl->memoryCollection()->setGenreMap( genreMap );
        m_memColl->memoryCollection()->releaseLock();
    }
    if( composer->tracks().isEmpty() )
    {
        composerMap.remove( composer->name() );
        m_memColl->memoryCollection()->acquireWriteLock();
        m_memColl->memoryCollection()->setComposerMap( composerMap );
        m_memColl->memoryCollection()->releaseLock();
    }
    if( year->tracks().isEmpty() )
    {
        yearMap.remove( year->year() );
        m_memColl->memoryCollection()->acquireWriteLock();
        m_memColl->memoryCollection()->setYearMap( yearMap );
        m_memColl->memoryCollection()->releaseLock();
    }

    // remove from trackmap
    trackMap.remove( track->uidUrl() );

    m_titlemap.remove( track->name(), TrackPtr::staticCast( track ) );

    // Finally, assign the created maps to the collection
    m_memColl->memoryCollection()->acquireWriteLock();
    m_memColl->memoryCollection()->setTrackMap( trackMap );
    m_memColl->memoryCollection()->setArtistMap( artistMap );
    m_memColl->memoryCollection()->setAlbumMap( albumMap );
    m_memColl->memoryCollection()->setGenreMap( genreMap );
    m_memColl->memoryCollection()->setComposerMap( composerMap );
    m_memColl->memoryCollection()->setYearMap( yearMap );
    m_memColl->memoryCollection()->releaseLock();
}

void
MediaDeviceHandler::getCopyableUrls(const Meta::TrackList &tracks)
{
    QMap<Meta::TrackPtr, QUrl> urls;
    for( Meta::TrackPtr track : tracks )
    {
        if( track->isPlayable() )
            urls.insert( track, track->playableUrl() );
    }

    Q_EMIT gotCopyableUrls( urls );
}

void
MediaDeviceHandler::copyTrackListToDevice(const Meta::TrackList tracklist)
{
    DEBUG_BLOCK
    const QString copyErrorCaption = i18n( "Copying Tracks Failed" );

    if ( m_isCopying )
    {
        KMessageBox::error( nullptr, i18n( "Tracks not copied: the device is already being copied to" ), copyErrorCaption );
        return;
    }

    setupReadCapability();
    if( !setupWriteCapability() )
        return;

    m_isCopying = true;

    bool isDupe = false;
    bool hasError = false;
    QString format;
    TrackMap trackMap = m_memColl->memoryCollection()->trackMap();

    Meta::TrackList tempTrackList;

    m_copyFailed = false;

    m_tracksFailed.clear();

    // Clear Transfer queue
    m_tracksToCopy.clear();

    // Check for same tags, don't copy if same tags
    // Also check for compatible format
    for( Meta::TrackPtr track : tracklist )
    {
        // Check for compatible formats
        format = track->type();

        if( !m_wc->supportedFormats().contains( format ) )
        {
             const QString error = i18n("Unsupported format: %1", format);
             m_tracksFailed.insert( track, error );
	     hasError = true;
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

        for( Meta::TrackPtr tempTrack : tempTrackList )
        {
            if( ( tempTrack->artist()->name() != track->artist()->name() )
                || ( tempTrack->album()->name() != track->album()->name() )
                || ( tempTrack->genre()->name() != track->genre()->name() )
                || ( tempTrack->composer()->name() != track->composer()->name() )
                || ( tempTrack->year()->name() != track->year()->name() ) )
            {
                continue;
            }

            // Track is already on there, break
            isDupe = true;
            hasError = true;
            break;
        }

        if( !isDupe )
            m_tracksToCopy.append( track );
        else
        {
            debug() << "Track " << track->name() << " is a dupe!";

            const QString error = i18n("Already on device");
            m_tracksFailed.insert( track, error );
        }
    }

    // NOTE: see comment at top of copyTrackListToDevice
    if( hasError )
        m_copyFailed = true;

    /* List ready, begin copying */

    // Do not bother copying 0 tracks
    // This could happen if all tracks to copy are dupes

    if( m_tracksToCopy.isEmpty() )
    {
        KMessageBox::error( nullptr, i18n( "Tracks not copied: the device already has these tracks" ), copyErrorCaption );
        m_isCopying = false;
        Q_EMIT copyTracksDone( false );
        return;
    }

    // Check for available space, in bytes, after the copy

    int transfersize = 0;

    for( Meta::TrackPtr track : m_tracksToCopy )
        transfersize += track->filesize();

    // NOTE: if the device will not have more than 5 MB to spare, abort the copy
    // This is important because on some devices if there is insufficient space to write the database, it will appear as
    // though all music has been wiped off the device - since neither the device nor amarok will be able to read the
    // (corrupted) database.
    if( !( (freeSpace() - transfersize) > 1024*1024*5 ) )
    {
        debug() << "Free space: " << freeSpace();
        debug() << "Space would've been after copy: " << (freeSpace() - transfersize);
        KMessageBox::error( nullptr, i18n( "Tracks not copied: the device has insufficient space" ), copyErrorCaption );
        m_isCopying = false;
        Q_EMIT copyTracksDone( false );
        return;
    }
    debug() << "Copying " << m_tracksToCopy.size() << " tracks";

    // Set up progress bar

    Amarok::Logger::newProgressOperation( this,
            i18n( "Transferring Tracks to Device" ), m_tracksToCopy.size() );

    // prepare to copy
    m_wc->prepareToCopy();

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
    DEBUG_BLOCK
    Meta::TrackPtr track;

    debug() << "Tracks left to copy after this one is now done: " << m_numTracksToCopy;

    if ( !m_tracksToCopy.isEmpty() )
    {
        // Pop the track off the front of the list
        track = m_tracksToCopy.takeFirst();

        // Copy the track and check result
        if ( !privateCopyTrackToDevice( track ) )
            slotCopyTrackFailed( track );
    }
    else
    {
        if ( m_numTracksToCopy > 0 )
            debug() << "Oops. \"Tracks to copy\" counter is not zero, but copy list is empty. Something missed?";

        if ( m_copyFailed )
        {
            Amarok::Logger::shortMessage(
                        i18np( "%1 track failed to copy to the device",
                               "%1 tracks failed to copy to the device", m_tracksFailed.size() ) );
        }

        // clear maps/hashes used
        m_tracksCopying.clear();
        m_trackSrcDst.clear();
        m_tracksFailed.clear();
        m_tracksToCopy.clear();

        // copying done

        m_isCopying = false;
        Q_EMIT copyTracksDone( true );
    }
}


bool
MediaDeviceHandler::privateCopyTrackToDevice( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    // Create new destTrack that will go into the device collection, based on source track
    Meta::MediaDeviceTrackPtr destTrack ( new Meta::MediaDeviceTrack( m_memColl ) );

    // find path to copy to
    m_wc->findPathToCopy( track, destTrack );

    // Create a track struct, associate it to destTrack
    m_wc->libCreateTrack( destTrack );

    // Fill the track struct of the destTrack with info from the track parameter as source
    setBasicMediaDeviceTrackInfo( track, destTrack );

    // set up the play url
    m_wc->libSetPlayableUrl( destTrack, track );

    getBasicMediaDeviceTrackInfo( destTrack, destTrack );

    m_trackSrcDst[ track ] = destTrack; // associate source with destination, for finalizing copy

    // Copy the file to the device
    return m_wc->libCopyTrack( track, destTrack );
}

/// @param track is the source track from which we are copying
void
MediaDeviceHandler::slotFinalizeTrackCopy( const Meta::TrackPtr & track )
{
    DEBUG_BLOCK
    //m_tracksCopying.removeOne( track );

    Meta::MediaDeviceTrackPtr destTrack = m_trackSrcDst[ track ];

    // Add the track struct into the database, if the library needs to
    m_wc->addTrackInDB( destTrack );

    // Inform subclass that a track has been added to the db
    m_wc->setDatabaseChanged();

    // Add the new Meta::MediaDeviceTrackPtr into the device collection

    // add track to collection
    addMediaDeviceTrackToCollection( destTrack );

    Q_EMIT incrementProgress();
    m_numTracksToCopy--;
}

void
MediaDeviceHandler::slotCopyTrackFailed( const Meta::TrackPtr & track )
{
    DEBUG_BLOCK
    Q_EMIT incrementProgress();

    m_numTracksToCopy--;

    QString error = i18n( "The track failed to copy to the device" );

    m_tracksFailed.insert( track, error );
}

void
MediaDeviceHandler::removeTrackListFromDevice( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK

    QString removeError = i18np( "Track not deleted:", "Tracks not deleted:", tracks.size() );
    QString removeErrorCaption = i18np( "Deleting Track Failed", "Deleting Tracks Failed", tracks.size() );

    if ( m_isDeleting )
    {
        KMessageBox::error( nullptr, i18n( "%1 tracks are already being deleted from the device.", removeError ), removeErrorCaption );
        return;
    }

    if( !setupWriteCapability() )
        return;

    m_isDeleting = true;

    // Init the list of tracks to be deleted
    m_tracksToDelete = tracks;

    // Set up statusbar for deletion operation
    Amarok::Logger::newProgressOperation( this,
            i18np( "Removing Track from Device", "Removing Tracks from Device", tracks.size() ),
            tracks.size() );

    m_wc->prepareToDelete();

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

        track = m_tracksToDelete.takeFirst();

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
    m_wc->libDeleteTrackFile( devicetrack );
}

void
MediaDeviceHandler::slotFinalizeTrackRemove( const Meta::TrackPtr & track )
{
    DEBUG_BLOCK
    Meta::MediaDeviceTrackPtr devicetrack = Meta::MediaDeviceTrackPtr::staticCast( track );

    // Remove the track struct from the db, references to it
    m_wc->removeTrackFromDB( devicetrack );

    // delete the struct associated with this track
    m_wc->libDeleteTrack( devicetrack );

    // Inform subclass that a track has been removed from
    m_wc->setDatabaseChanged();

    // remove from memory collection
    removeMediaDeviceTrackFromCollection( devicetrack );

    Q_EMIT incrementProgress();

    m_numTracksToRemove--;

    if( m_numTracksToRemove == 0 )
    {
        /*
        if( m_tracksFailed.size() > 0 )
        {
            Amarok::Logger::shortMessage(
                        i18n( "%1 tracks failed to copy to the device", m_tracksFailed.size() ) );
        }
        */
        debug() << "Done removing tracks";
        m_isDeleting = false;
        Q_EMIT removeTracksDone();
    }
}

void
MediaDeviceHandler::slotDatabaseWritten( bool success )
{
    DEBUG_BLOCK
    Q_UNUSED( success )

    Q_EMIT endProgressOperation( this );

    m_memColl->collectionUpdated();
}


void
MediaDeviceHandler::setupArtistMap( Meta::MediaDeviceTrackPtr track, ArtistMap &artistMap )
{
    const QString artist( m_rc->libGetArtist( track ) );
    MediaDeviceArtistPtr artistPtr;

    if( artistMap.contains( artist ) )
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
MediaDeviceHandler::setupAlbumMap( Meta::MediaDeviceTrackPtr track, AlbumMap& albumMap, ArtistMap &artistMap )
{
    const QString album( m_rc->libGetAlbum( track ) );
    QString albumArtist( m_rc->libGetAlbumArtist( track ) );
    if( albumArtist.compare( QLatin1String("Various Artists"), Qt::CaseInsensitive ) == 0 ||
        albumArtist.compare( i18n( "Various Artists" ), Qt::CaseInsensitive ) == 0 )
    {
        albumArtist.clear();
    }
    MediaDeviceAlbumPtr albumPtr;

    if ( albumMap.contains( album, albumArtist ) )
        albumPtr = MediaDeviceAlbumPtr::staticCast( albumMap.value( album, albumArtist ) );
    else
    {
        MediaDeviceArtistPtr albumArtistPtr;
        if( artistMap.contains( albumArtist ) )
            albumArtistPtr = MediaDeviceArtistPtr::staticCast( artistMap.value( albumArtist ) );
        else if( !albumArtist.isEmpty() )
        {
            albumArtistPtr = MediaDeviceArtistPtr( new MediaDeviceArtist( albumArtist ) );
            artistMap.insert( albumArtist, ArtistPtr::staticCast( albumArtistPtr ) );
        }

        albumPtr = MediaDeviceAlbumPtr( new MediaDeviceAlbum( m_memColl, album ) );
        albumPtr->setAlbumArtist( albumArtistPtr ); // needs to be before albumMap.insert()
        albumMap.insert( AlbumPtr::staticCast( albumPtr ) );
    }

    albumPtr->addTrack( track );
    track->setAlbum( albumPtr );

    bool isCompilation = albumPtr->isCompilation();
    /* if at least one track from album identifies itself as a part of compilation, mark
     * whole album as such: (we should be deterministic wrt track adding order) */
    isCompilation |= m_rc->libIsCompilation( track );
    albumPtr->setIsCompilation( isCompilation );

    if( albumArtist.isEmpty() )
    {
        // set compilation flag, otherwise the album would be invisible in collection
        // browser if "Album Artist / Album" view is selected.
        albumPtr->setIsCompilation( true );
    }
}

void
MediaDeviceHandler::setupGenreMap( Meta::MediaDeviceTrackPtr track, GenreMap& genreMap )
{
    const QString genre = m_rc->libGetGenre( track );
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
    QString composer ( m_rc->libGetComposer( track ) );
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
    int year = m_rc->libGetYear( track );
    MediaDeviceYearPtr yearPtr;
    if ( yearMap.contains( year ) )
        yearPtr = MediaDeviceYearPtr::staticCast( yearMap.value( year ) );
    else
    {
        yearPtr = MediaDeviceYearPtr( new MediaDeviceYear( QString::number(year) ) );
        yearMap.insert( year, YearPtr::staticCast( yearPtr ) );
    }
    yearPtr->addTrack( track );
    track->setYear( yearPtr );
}

bool
MediaDeviceHandler::privateParseTracks()
{
    DEBUG_BLOCK

    if( !setupReadCapability() )
        return false;

    TrackMap trackMap;
    ArtistMap artistMap;
    AlbumMap albumMap;
    GenreMap genreMap;
    ComposerMap composerMap;
    YearMap yearMap;

    /* iterate through tracklist and add to appropriate map */
    for( m_rc->prepareToParseTracks(); !m_rc->isEndOfParseTracksList(); m_rc->prepareToParseNextTrack() )
    {
        /// Fetch next track to parse

        m_rc->nextTrackToParse();

        // FIXME: should we return true or false?
        if (!m_memColl)
            return true;

        MediaDeviceTrackPtr track( new MediaDeviceTrack( m_memColl ) );

        m_rc->setAssociateTrack( track );
        getBasicMediaDeviceTrackInfo( track, track );

        /* map-related info retrieval */
        setupArtistMap( track, artistMap );
        setupAlbumMap( track, albumMap, artistMap );
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
        m_provider = new Playlists::MediaDeviceUserPlaylistProvider( m_memColl );

        // Begin parsing the playlists
        Playlists::MediaDevicePlaylistList playlists;

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
            Playlists::MediaDevicePlaylistPtr playlist( new Playlists::MediaDevicePlaylist( m_pc->libGetPlaylistName(), tracklist ) );

            m_pc->setAssociatePlaylist( playlist );

            // Insert the new playlist into the list of playlists
            //playlists << playlist;

            // Inform the provider of the new playlist
            m_provider->addMediaDevicePlaylist( playlist );
        }

        // When the provider saves a playlist, the handler should save it internally
        connect( m_provider, &Playlists::MediaDeviceUserPlaylistProvider::playlistSaved,
                 this, &MediaDeviceHandler::savePlaylist );
        connect( m_provider, &Playlists::MediaDeviceUserPlaylistProvider::playlistRenamed,
                 this, &MediaDeviceHandler::renamePlaylist );
        connect( m_provider, &Playlists::MediaDeviceUserPlaylistProvider::playlistsDeleted,
                 this, &MediaDeviceHandler::deletePlaylists );

        The::playlistManager()->addProvider(  m_provider,  m_provider->category() );
        m_provider->sendUpdated();

    }

    if( !m_podcastCapability && hasCapabilityInterface( Handler::Capability::Podcast ) )
    {

    }

    // Finally, assign the created maps to the collection
    m_memColl->memoryCollection()->acquireWriteLock();
    m_memColl->memoryCollection()->setTrackMap( trackMap );
    m_memColl->memoryCollection()->setArtistMap( artistMap );
    m_memColl->memoryCollection()->setAlbumMap( albumMap );
    m_memColl->memoryCollection()->setGenreMap( genreMap );
    m_memColl->memoryCollection()->setComposerMap( composerMap );
    m_memColl->memoryCollection()->setYearMap( yearMap );
    m_memColl->memoryCollection()->releaseLock();

    m_memColl->collectionUpdated();

    return true;
}

void
MediaDeviceHandler::slotCopyNextTrackFailed( ThreadWeaver::JobPointer job, const Meta::TrackPtr& track )
{
    Q_UNUSED( job );
    enqueueNextCopyThread();
    m_copyFailed = true;
    slotCopyTrackFailed( track );
}

void
MediaDeviceHandler::slotCopyNextTrackDone( ThreadWeaver::JobPointer job, const Meta::TrackPtr& track )
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
        ThreadWeaver::Queue::instance()->enqueue( (QSharedPointer<ThreadWeaver::Job>(new CopyWorkerThread( track,  this )) ) );
    }
    else
    {
	// Finish the progress bar
	Q_EMIT incrementProgress();
	Q_EMIT endProgressOperation( this );

	// Inform CollectionLocation that copying is done
	m_isCopying = false;
	Q_EMIT copyTracksDone( true );
    }
}

float
MediaDeviceHandler::freeSpace()
{
    if ( setupReadCapability() )
        return m_rc->totalCapacity() - m_rc->usedCapacity();
    else
        return 0.0;
}

float
MediaDeviceHandler::usedcapacity()
{
    if ( setupReadCapability() )
        return m_rc->usedCapacity();
    else
        return 0.0;
}

float
MediaDeviceHandler::totalcapacity()
{
    if ( setupReadCapability() )
        return m_rc->totalCapacity();
    else
        return 0.0;
}

Playlists::UserPlaylistProvider*
MediaDeviceHandler::provider()
{
    DEBUG_BLOCK
    return (qobject_cast<Playlists::UserPlaylistProvider *>( m_provider ) );
}

void
MediaDeviceHandler::savePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist, const QString& name )
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
MediaDeviceHandler::renamePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist )
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
MediaDeviceHandler::deletePlaylists( const Playlists::MediaDevicePlaylistList &playlistlist )
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
        for( Playlists::MediaDevicePlaylistPtr playlist : playlistlist )
        {
            m_pc->deletePlaylist( playlist );
        }

        writeDatabase();
    }
}

bool
MediaDeviceHandler::setupReadCapability()
{
    if( m_rc )
        return true;
    if( !hasCapabilityInterface( Handler::Capability::Readable ) )
        return false;

    m_rc = create<Handler::ReadCapability>();
    return (bool) m_rc;
}

bool
MediaDeviceHandler::setupWriteCapability()
{
    if( m_wc )
        return true;
    if( !hasCapabilityInterface( Handler::Capability::Writable ) )
        return false;

    m_wc = create<Handler::WriteCapability>();
    return (bool) m_wc;
}

/** Observer Methods **/
void
MediaDeviceHandler::metadataChanged( const TrackPtr &track )
{
    DEBUG_BLOCK

    Meta::MediaDeviceTrackPtr trackPtr = Meta::MediaDeviceTrackPtr::staticCast( track );

    if( !setupWriteCapability() )
        return;

    setBasicMediaDeviceTrackInfo( track, trackPtr );
    m_wc->setDatabaseChanged();

    m_wc->updateTrack( trackPtr );
}

void
MediaDeviceHandler::metadataChanged(const  ArtistPtr &artist )
{
    Q_UNUSED( artist );
}

void
MediaDeviceHandler::metadataChanged(const  AlbumPtr &album )
{
    Q_UNUSED( album );
}

void
MediaDeviceHandler::metadataChanged(const  GenrePtr &genre )
{
    Q_UNUSED( genre );
}

void
MediaDeviceHandler::metadataChanged(const  ComposerPtr &composer )
{
    Q_UNUSED( composer );
}

void
MediaDeviceHandler::metadataChanged(const YearPtr &year )
{
    Q_UNUSED( year );
}

void
MediaDeviceHandler::parseTracks()
{
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(new ParseWorkerThread( this )) );
}

// ParseWorkerThread

ParseWorkerThread::ParseWorkerThread( MediaDeviceHandler* handler )
        : QObject()
        , ThreadWeaver::Job()
        , m_success( false )
        , m_handler( handler )
{
    connect( this, &ParseWorkerThread::done, this, &ParseWorkerThread::slotDoneSuccess );
}

ParseWorkerThread::~ParseWorkerThread()
{
    //nothing to do
}

bool
ParseWorkerThread::success() const
{
    return m_success;
}

void
ParseWorkerThread::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    m_success = m_handler->privateParseTracks();
}

void
ParseWorkerThread::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
ParseWorkerThread::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

void
ParseWorkerThread::slotDoneSuccess( ThreadWeaver::JobPointer )
{
    if (m_handler->m_memColl)
        m_handler->m_memColl->emitCollectionReady();
}

// CopyWorkerThread

CopyWorkerThread::CopyWorkerThread( const Meta::TrackPtr &track, MediaDeviceHandler* handler )
        : QObject()
        , ThreadWeaver::Job()
        , m_success( false )
        , m_track( track )
        , m_handler( handler )
{
    //connect( this, &CopyWorkerThread::done, m_handler, &Meta::MediaDeviceHandler::slotCopyNextTrackToDevice, Qt::QueuedConnection );
    connect( this, &CopyWorkerThread::failed, this, &CopyWorkerThread::slotDoneFailed, Qt::QueuedConnection );
    connect( this, &CopyWorkerThread::copyTrackFailed, m_handler, &Meta::MediaDeviceHandler::slotCopyNextTrackFailed );
    connect( this, &CopyWorkerThread::copyTrackDone, m_handler, &Meta::MediaDeviceHandler::slotCopyNextTrackDone );
    connect( this, &CopyWorkerThread::done, this, &CopyWorkerThread::slotDoneSuccess );

    //connect( this, &CopyWorkerThread::done, this, &CopyWorkerThread::deleteLater );
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
CopyWorkerThread::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    m_success = m_handler->privateCopyTrackToDevice( m_track );
}

void
CopyWorkerThread::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
CopyWorkerThread::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

void
CopyWorkerThread::slotDoneSuccess( ThreadWeaver::JobPointer )
{
    Q_EMIT copyTrackDone( QSharedPointer<ThreadWeaver::Job>(this), m_track );
}

void
CopyWorkerThread::slotDoneFailed( ThreadWeaver::JobPointer )
{
    Q_EMIT copyTrackFailed( QSharedPointer<ThreadWeaver::Job>(this), m_track );
}
