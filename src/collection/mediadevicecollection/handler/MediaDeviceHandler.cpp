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

#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/JobCollection.h>

using namespace Meta;

//Meta::MetaHandlerCapability

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

//Meta::MediaDeviceHandler

MediaDeviceHandler::MediaDeviceHandler( QObject *parent )
: QObject( parent )
, m_memColl( qobject_cast<MediaDeviceCollection*>(parent) )
{
    DEBUG_BLOCK

    connect( m_memColl, SIGNAL( deletingCollection() ),
                 this, SLOT( slotDeletingHandler() ) );

}

MediaDeviceHandler::~MediaDeviceHandler()
{
    DEBUG_BLOCK
    delete m_provider;
}

void
MediaDeviceHandler::slotDeletingHandler()
{
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
MediaDeviceHandler::setBasicMediaDeviceTrackInfo( const Meta::TrackPtr& srcTrack, MediaDeviceTrackPtr destTrack )
{
    DEBUG_BLOCK
           libSetTitle( destTrack, srcTrack->name() );
           libSetAlbum( destTrack, srcTrack->album()->name() );
           libSetArtist( destTrack, srcTrack->artist()->name() );
           libSetComposer( destTrack, srcTrack->composer()->name() );
           libSetGenre( destTrack, srcTrack->genre()->name() );
           libSetYear( destTrack, srcTrack->year()->name() );
           libSetLength( destTrack, srcTrack->length() );
           libSetTrackNumber( destTrack, srcTrack->trackNumber() );
           libSetComment( destTrack, srcTrack->comment() );
           libSetDiscNumber( destTrack, srcTrack->discNumber() );
           libSetBitrate( destTrack, srcTrack->bitrate() );
           libSetSamplerate( destTrack, srcTrack->sampleRate() );
           //libSetBpm( destTrack, srcTrack->bpm() );
           libSetFileSize( destTrack, srcTrack->filesize() );
           libSetPlayCount( destTrack, srcTrack->playCount() );
           libSetLastPlayed( destTrack, srcTrack->lastPlayed() );
           libSetRating( destTrack, srcTrack->rating() );
           libSetType( destTrack, srcTrack->type() );
           //libSetPlayableUrl( destTrack, srcTrack );
/*
           if( srcTrack->album()->hasImage() )
               libSetCoverArt( destTrack, srcTrack->album()->image() );
           */
}

void
MediaDeviceHandler::addMediaDeviceTrackToCollection(Meta::MediaDeviceTrackPtr& track)
{
    DEBUG_BLOCK


    TrackMap trackMap = m_memColl->trackMap();
    ArtistMap artistMap = m_memColl->artistMap();
    AlbumMap albumMap = m_memColl->albumMap();
    GenreMap genreMap = m_memColl->genreMap();
    ComposerMap composerMap = m_memColl->composerMap();
    YearMap yearMap = m_memColl->yearMap();

    /* 1-liner info retrieval */

    //setCopyTrackForParse();

    getBasicMediaDeviceTrackInfo( track, track );

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

    return;
}

void
MediaDeviceHandler::removeMediaDeviceTrackFromCollection( Meta::MediaDeviceTrackPtr &track )
{
        DEBUG_BLOCK

    // get pointers

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

    debug() << "Artist name: " << artist->name();

    artist->remTrack( track );
    album->remTrack( track );
    genre->remTrack( track );
    composer->remTrack( track );
    year->remTrack( track );

    // if empty, get rid of metadata in general

    if( artist->tracks().isEmpty() )
    {
        artistMap.remove( artist->name() );
        debug() << "Artist still in artist map: " << ( artistMap.contains( artist->name() ) ? "yes" : "no");
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

        DEBUG_BLOCK

    bool isDupe;
    bool hasDupe;
    QString format;
    TrackMap trackMap = m_memColl->trackMap();

    Meta::TrackList tempTrackList;

    m_copyFailed = false;

    hasDupe = false;

    m_tracksFailed.clear();

    /* Clear Transfer queue */

    m_tracksToCopy.clear();


    /* Check for same tags, don't copy if same tags */
    /* Also check for compatible format */

    foreach( Meta::TrackPtr track, tracklist )
    {
        /* Check for compatible formats */

        format = track->type();

        if( !supportedFormats().contains( format ) )
        {
             QString error = "Unsupported format: " + format;
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

        debug() << "Same title present, checking other tags";

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

            /* Track is already on there, break */

            isDupe = true;
            hasDupe = true;
            break;
        }

        if( !isDupe )
            m_tracksToCopy.append( track );
        else
        {
            debug() << "Track " << track->name() << " is a dupe!";

            QString error = "Already on device";
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
        The::statusBar()->shortMessage( i18n( "The tracks to copy are already on the device!" ) );
        emit copyTracksDone( false );
        return;
    }
    debug() << "Copying " << m_tracksToCopy.size() << " tracks";

    // Set up progress bar

    m_statusbar = The::statusBar()->newProgressOperation( this, i18n( "Transferring Tracks to Device" ) );

    m_statusbar->setMaximum( m_tracksToCopy.size() );

    connect( this, SIGNAL( incrementProgress() ),
            The::statusBar(), SLOT( incrementProgress() ), Qt::QueuedConnection );

     connect( this, SIGNAL( databaseWritten(bool)),
              this, SLOT( slotDatabaseWritten(bool)), Qt::QueuedConnection );

    // prepare to copy

    prepareToCopy();

    m_numTracksToCopy = m_tracksToCopy.count();
    m_tracksCopying.clear();
    m_trackSrcDst.clear();

    // begin copying tracks to device

    if( !m_copyingthreadsafe )
    {
            copyNextTrackToDevice();
    }

    else
    {
        enqueueNextCopyThread();
    }

    return;
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

        //m_tracksCopying.insert( track );

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

    findPathToCopy( track, destTrack );

    // Create a track struct, associate it to destTrack

    libCreateTrack( destTrack );

    // Fill the track struct of the destTrack with info from the track parameter as source

    setBasicMediaDeviceTrackInfo( track, destTrack );

    // set up the play url

    libSetPlayableUrl( destTrack, track );

    m_trackSrcDst[ track ] = destTrack; // associate source with destination, for finalizing copy

    // Copy the file to the device

    success = libCopyTrack( track, destTrack );

    return success;
}

/// @param track is the source track from which we are copying
void
MediaDeviceHandler::slotFinalizeTrackCopy( const Meta::TrackPtr & track )
{
    DEBUG_BLOCK
    //m_tracksCopying.removeOne( track );

    Meta::MediaDeviceTrackPtr destTrack = m_trackSrcDst[ track ];

    // Add the track struct into the database, if the library needs to

    addTrackInDB( destTrack );

    // Inform subclass that a track has been added to the db

    databaseChanged();

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
            The::statusBar()->shortMessage( i18n( "%1 tracks failed to copy to the device", m_tracksFailed.size() ) );
        }
        // clear maps/hashes used

        m_tracksCopying.clear();
        m_trackSrcDst.clear();
        m_tracksFailed.clear();
        m_tracksToCopy.clear();

        // copying done

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

    // Init the list of tracks to be deleted

    m_tracksToDelete = tracks;

    // Set up statusbar for deletion operation

    m_statusbar = The::statusBar()->newProgressOperation( this, i18n( "Removing Tracks from Device" ) );

    m_statusbar->setMaximum( tracks.size() );

    connect( this, SIGNAL( incrementProgress() ),
            The::statusBar(), SLOT( incrementProgress() ), Qt::QueuedConnection );

     connect( this, SIGNAL( databaseWritten(bool)),
              this, SLOT( slotDatabaseWritten(bool)), Qt::QueuedConnection );

    prepareToDelete();

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

    libDeleteTrackFile( devicetrack );


}

void
MediaDeviceHandler::slotFinalizeTrackRemove( const Meta::TrackPtr & track )
{
    DEBUG_BLOCK
    Meta::MediaDeviceTrackPtr devicetrack = Meta::MediaDeviceTrackPtr::staticCast( track );

    // Remove the track struct from the db, references to it

    removeTrackFromDB( devicetrack );

    // delete the struct associated with this track

    libDeleteTrack( devicetrack );

    // remove from titlemap

    m_titlemap.remove( track->name(), track );

    // remove from collection

    removeMediaDeviceTrackFromCollection( devicetrack );

    // Inform subclass that a track has been removed from

    databaseChanged();

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
        emit removeTracksDone();
    }
}

void
MediaDeviceHandler::slotDatabaseWritten( bool success )
{
    DEBUG_BLOCK

    debug() << "Database write: " << (success? "succeeded" : "failed" );
    connect( this, SIGNAL( endProgressOperation( const QObject*) ),
             The::statusBar(), SLOT( endProgressOperation( const QObject* ) ), Qt::QueuedConnection );

    emit endProgressOperation( this );
    The::statusBar()->shortMessage( i18n( "Operation complete!" ) );

    m_memColl->collectionUpdated();
}


void
MediaDeviceHandler::setupArtistMap( Meta::MediaDeviceTrackPtr track, ArtistMap& artistMap )
{
    QString artist( m_rc->libGetArtist( track ) );
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
    QString album( m_rc->libGetAlbum( track ) );
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
    QString genre = m_rc->libGetGenre( track );
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
    QString year;
    year = year.setNum( m_rc->libGetYear( track ) );
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

    if( !m_rc )
    {
        if( this->hasCapabilityInterface( Handler::Capability::Readable ) )
        {
            m_rc = this->create<Handler::ReadCapability>();
            if( !m_rc )
            {
                debug() << "Handler does not have MediaDeviceHandler::ReadCapability. Aborting parse.";
                return;
            }
        }
    }

    TrackMap trackMap;
    ArtistMap artistMap;
    AlbumMap albumMap;
    GenreMap genreMap;
    ComposerMap composerMap;
    YearMap yearMap;
    //debug() << "Before Parse";

    /* iterate through tracklist and add to appropriate map */
    for( m_rc->prepareToParseTracks(); !(m_rc->isEndOfParseTracksList()); m_rc->prepareToParseNextTrack() )
    {
        //debug() << "Fetching next track to parse";
        /// Fetch next track to parse

        m_rc->nextTrackToParse();

        //debug() << "Fetched next track to parse";

// TODO: coverart

        MediaDeviceTrackPtr track( new MediaDeviceTrack( m_memColl ) );

        m_rc->setAssociateTrack( track );

        //debug() << "Created new media device track";

        getBasicMediaDeviceTrackInfo(track, track);

       // debug() << "Got basic info";

        /* map-related info retrieval */

        setupArtistMap( track, artistMap );
        setupAlbumMap( track, albumMap );
        setupGenreMap( track, genreMap );
        setupComposerMap( track, composerMap );
        setupYearMap( track, yearMap );

        //debug() << "Setup maps";

        /* TrackMap stuff to be subordinated later */
        trackMap.insert( track->uidUrl(), TrackPtr::staticCast( track ) );

        //debug() << "Inserted to trackmap";

        // TODO: setup titlemap for copy/deleting
        m_titlemap.insert( track->name(), TrackPtr::staticCast( track ) );




        // TODO: abstract playlist parsing


        // Subscribe to Track for metadata updates



        subscribeTo( Meta::TrackPtr::staticCast( track ) );

        //debug() << "Subscribed";
    }


    if( !m_pc )
    {
        if( this->hasCapabilityInterface( Handler::Capability::Playlist ) )
        {
            m_pc = this->create<Handler::PlaylistCapability>();
            if( !m_pc )
            {
                debug() << "Handler does not have MediaDeviceHandler::PlaylistCapability. Not parsing playlists.";
            }
        }
    }

#if 0
    if( m_pc )
    {
        // Register the playlist provider with the playlistmanager

        // register a playlist provider for this type of device
        debug() << "adding provider";
        m_provider = new MediaDeviceUserPlaylistProvider();

        // Begin parsing the playlists

        Meta::MediaDevicePlaylistList playlists;

        for ( m_pc->prepareToParsePlaylists(); !(m_pc->isEndOfParsePlaylistsList()); m_pc->prepareToParseNextPlaylist() )
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

            // Insert the new playlist into the list of playlists

            //playlists << playlist;

            // Inform the provider of the new playlist

            m_provider->addPlaylist( playlist );
        }

        The::playlistManager()->addProvider(  m_provider,  m_provider->category() );
        m_provider->sendUpdated();

    }

#endif

    // Inform the provider of these new playlists
/*
    foreach( Meta::MediaDevicePlaylistPtr playlist, playlists )
        {

        }
*/


    // HACK: add a blank playlist
    /*
    TrackList tracks;
    MediaDeviceTrackPtr tp( new MediaDeviceTrack( 0 ) );
    TrackPtr track = TrackPtr::staticCast( tp );
    MediaDevicePlaylistPtr list(  new MediaDevicePlaylist(  "Testlist",  tracks ) );
    m_provider->addPlaylist( list );
    */


    // Finally, assign the created maps to the collection

    debug() << "Setting memcoll stuff";


    m_memColl->acquireWriteLock();
    //debug() << "Debug 1";
    m_memColl->setTrackMap( trackMap );
    //debug() << "Debug 2";
    m_memColl->setArtistMap( artistMap );
    //debug() << "Debug 3";
    m_memColl->setAlbumMap( albumMap );
    //debug() << "Debug 4";
    m_memColl->setGenreMap( genreMap );
    //debug() << "Debug 5";
    m_memColl->setComposerMap( composerMap );
    //debug() << "Debug 6";
    m_memColl->setYearMap( yearMap );
    //debug() << "Debug 7";
    m_memColl->releaseLock();

    //debug() << "Done setting memcoll stuff";
}

void
MediaDeviceHandler::slotCopyNextTrackFailed( ThreadWeaver::Job* job, const Meta::TrackPtr& track )
{
    Q_UNUSED( job );
    enqueueNextCopyThread();
    m_copyFailed = true;
    QString error = "Job Failed";
    slotCopyTrackFailed( track );
}

void
MediaDeviceHandler::slotCopyNextTrackDone( ThreadWeaver::Job* job, const Meta::TrackPtr& track )
{
    Q_UNUSED( track )
    enqueueNextCopyThread();
    if ( job->success() )
    {
        slotFinalizeTrackCopy( track );
    }
    else
    {
        m_copyFailed = true;
        QString error = "Copy error";
        slotCopyTrackFailed( track );
    }
}

void
MediaDeviceHandler::enqueueNextCopyThread()
{
    Meta::TrackPtr track;

    // If there are more tracks to copy, copy the next one
        if (  !m_tracksToCopy.isEmpty() )
        {
            // Pop the track off the front of the list

            track = m_tracksToCopy.first();
            m_tracksToCopy.removeFirst();

            // Copy the track

//            m_lastTrackCopied = track;

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

    emit copyTracksDone( true );
}

/** Observer Methods **/
void
MediaDeviceHandler::metadataChanged( TrackPtr track )
{
    DEBUG_BLOCK

    Meta::MediaDeviceTrackPtr trackPtr = Meta::MediaDeviceTrackPtr::staticCast( track );
    KUrl trackUrl = KUrl::fromPath( trackPtr->uidUrl() );

    setBasicMediaDeviceTrackInfo( track, trackPtr );

    updateTrack( trackPtr );
    databaseChanged();
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
    /*

    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), m_handler, SLOT( slotCopyNextTrackToDevice( ThreadWeaver::Job* ) ), Qt::QueuedConnection );
*/
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
    debug() << "running";
    m_success = m_handler->privateCopyTrackToDevice( m_track );
    debug() << "thread run";
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
