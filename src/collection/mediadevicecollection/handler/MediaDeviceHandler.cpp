/***************************************************************************
 * copyright            : (C) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "MediaDeviceHandler.h"

#include "MediaDeviceCollection.h"

#include <threadweaver/ThreadWeaver.h>

using namespace Meta;

MediaDeviceHandler::MediaDeviceHandler( QObject *parent )
: QObject( parent )
, m_memColl( qobject_cast<MediaDeviceCollection*>(parent) )
{
    DEBUG_BLOCK
}

bool
MediaDeviceHandler::isWritable() const
{
    return false;
}


void
MediaDeviceHandler::getBasicMediaDeviceTrackInfo( Meta::MediaDeviceTrackPtr track ) const
{
    /* 1-liner info retrieval */

    track->setTitle( libGetTitle() );
    track->setLength( libGetLength() );
    track->setTrackNumber( libGetTrackNumber() );
    track->setComment( libGetComment() );
    track->setDiscNumber( libGetDiscNumber() );
    track->setBitrate( libGetBitrate() );
    track->setSamplerate( libGetSamplerate() );
    track->setBpm( libGetBpm() );
    track->setFileSize( libGetFileSize() );
    track->setPlayCount( libGetPlayCount() );
    track->setLastPlayed( libGetLastPlayed() );
    track->setRating( libGetRating() );

    track->setPlayableUrl( libGetPlayableUrl() );

    track->setType( libGetType() );
}

void
MediaDeviceHandler::setBasicMediaDeviceTrackInfo( Meta::TrackPtr track )
{
           libSetTitle( track->name() );
           libSetAlbum( track->album()->name() );
           libSetArtist( track->artist()->name() );
           libSetComposer( track->composer()->name() );
           libSetGenre( track->genre()->name() );
           libSetYear( track->year()->name() );
           libSetLength( track->length() );
           libSetTrackNumber( track->trackNumber() );
           libSetComment( track->comment() );
           libSetDiscNumber( track->discNumber() );
           libSetBitrate( track->bitrate() );
           libSetSamplerate( track->sampleRate() );
           //libSetBpm( track->bpm() );
           libSetFileSize( track->filesize() );
           libSetPlayCount( track->playCount() );
           libSetLastPlayed( track->lastPlayed() );
           libSetRating( track->rating() );
           libSetType( track->type() );
           libSetPlayableUrl();
}

void
MediaDeviceHandler::addMediaDeviceTrackToCollection()
{


    TrackMap trackMap = m_memColl->trackMap();
    ArtistMap artistMap = m_memColl->artistMap();
    AlbumMap albumMap = m_memColl->albumMap();
    GenreMap genreMap = m_memColl->genreMap();
    ComposerMap composerMap = m_memColl->composerMap();
    YearMap yearMap = m_memColl->yearMap();

    MediaDeviceTrackPtr track( new MediaDeviceTrack( m_memColl ) );

    /* 1-liner info retrieval */

    setCopyTrackForParse();

    getBasicMediaDeviceTrackInfo( track );

    /* map-related info retrieval */
    setupArtistMap( track, artistMap );
    setupAlbumMap( track, albumMap );
    setupGenreMap( track, genreMap );
    setupComposerMap( track, composerMap );
    setupYearMap( track, yearMap );

    /* trackmap also soon to be subordinated */

    trackMap.insert( track->uidUrl(), TrackPtr::staticCast( track ) );

    m_titlemap.insert( track->name(), TrackPtr::staticCast( track ) );

    setAssociateTrack( track );
    // NOTE: not supporting adding track that's already on a playlist
    //mtpTrackMap.insert( mtptrack, track ); // map for playlist formation

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

    // HACK: Copy is said to fail if >=1 tracks isn't copied to device.
    // This is so that a move operation doesn't attempt to delete
    // the tracks from the original collection, as some of these tracks
    // would not have been copied to the device.

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
        emit copyTracksDone( false );
        return;
    }
    debug() << "Copying " << m_tracksToCopy.size() << " tracks";

    // Set up progress bar

    m_statusbar = The::statusBar()->newProgressOperation( this, i18n( "Transferring Tracks to Device" ) );

    m_statusbar->setMaximum( m_tracksToCopy.size() );

    connect( this, SIGNAL( incrementProgress() ),
            The::statusBar(), SLOT( incrementProgress() ) );
    connect( this, SIGNAL( endProgressOperation( const QObject*) ),
            The::statusBar(), SLOT( endProgressOperation( const QObject* ) ) );

    // prepare to copy

    prepareToCopy();

    // begin copying tracks to device

    copyNextTrackToDevice();
    
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

        m_lastTrackCopied = track;

        ThreadWeaver::Weaver::instance()->enqueue( new CopyWorkerThread( track, this ) );

    }

    // No tracks left to copy, emit done
    else
    {
        emit incrementProgress();
        emit endProgressOperation( this );
        emit copyTracksDone( !m_copyFailed );
    }
}


bool
MediaDeviceHandler::privateCopyTrackToDevice( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    bool success = false;

    // Find path on device to copy to

    findPathToCopy( track );

    // When internal copy method is done, keep going by creating track struct
    // and adding to db etc.

    connect( this, SIGNAL( libCopyTrackDone( const Meta::TrackPtr & ) ),
             this, SLOT( slotFinalizeTrackCopy( const Meta::TrackPtr & ) ) );

    // Copy the file to the device

    success = libCopyTrack( track );

    return success;
}

void
MediaDeviceHandler::slotFinalizeTrackCopy( const Meta::TrackPtr & track )
{
    // Create a track struct

    libCreateTrack();

    // Fill the track struct with info from the track parameter

    setBasicMediaDeviceTrackInfo( track );

    // Add the track into the database, if the library needs to

    addTrackInDB();

    // Add the new Meta::MediaDeviceTrackPtr into the device collection

    // add track to collection
    addMediaDeviceTrackToCollection();
}

void
MediaDeviceHandler::setupArtistMap( Meta::MediaDeviceTrackPtr track, ArtistMap& artistMap )
{
    QString artist( libGetArtist() );
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
    QString album( libGetAlbum() );
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
    QString genre = libGetGenre();
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
    QString composer ( libGetComposer() );
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
    year = year.setNum( libGetYear() );
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

    TrackMap trackMap;
    ArtistMap artistMap;
    AlbumMap albumMap;
    GenreMap genreMap;
    ComposerMap composerMap;
    YearMap yearMap;
    //debug() << "Before Parse";

    /* iterate through tracklist and add to appropriate map */
    for( prepareToParse(); !isEndOfParseList(); prepareToParseNextTrack() )
    {
        //debug() << "Fetching next track to parse";
        /// Fetch next track to parse

        nextTrackToParse();

        //debug() << "Fetched next track to parse";

        //getCoverArt( ipodtrack );

        MediaDeviceTrackPtr track( new MediaDeviceTrack( m_memColl ) );

        //debug() << "Created new media device track";

        getBasicMediaDeviceTrackInfo(track);

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

        setAssociateTrack( track );

        //debug() << "Associated track";
        // TODO: abstract playlist parsing
        //ipodTrackMap.insert( ipodtrack, track ); // map for playlist formation

        // Subscribe to Track for metadata updates

        subscribeTo( Meta::TrackPtr::staticCast( track ) );

        //debug() << "Subscribed";
    }

    // Iterate through ipod's playlists to set track's playlists

// TODO: abstract playlist parsing
/*
    GList *member = 0;

    for ( cur = m_itdb->playlists; cur; cur = cur->next )
    {
        Itdb_Playlist *ipodplaylist = ( Itdb_Playlist * ) cur->data;
        for ( member = ipodplaylist->members; member; member = member->next )
        {
            Itdb_Track *ipodtrack = ( Itdb_Track * )member->data;
            IpodTrackPtr track = ipodTrackMap.value( ipodtrack );
            track->addIpodPlaylist( ipodplaylist );
        }
    }
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
MediaDeviceHandler::slotCopyNextTrackFailed( ThreadWeaver::Job* job )
{
    Q_UNUSED( job );
    m_copyFailed = true;
    QString error = "Job Failed";
    m_tracksFailed.insert( m_lastTrackCopied, error );

    copyNextTrackToDevice();
}

void
MediaDeviceHandler::slotCopyNextTrackToDevice( ThreadWeaver::Job* job )
{
    if ( job->success() )
    {
        emit incrementProgress();
    }
    else
    {
        m_copyFailed = true;
        QString error = "Copy error";
        m_tracksFailed.insert( m_lastTrackCopied, error );
    }

    copyNextTrackToDevice();
}

CopyWorkerThread::CopyWorkerThread( const Meta::TrackPtr &track, MediaDeviceHandler* handler )
        : ThreadWeaver::Job()
        , m_success( false )
        , m_track( track )
        , m_handler( handler )
{
    connect( this, SIGNAL( failed( ThreadWeaver::Job* ) ), m_handler, SLOT( slotCopyNextTrackFailed( ThreadWeaver::Job* ) ) );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), m_handler, SLOT( slotCopyNextTrackToDevice( ThreadWeaver::Job* ) ) );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ) );
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



#include "MediaDeviceHandler.moc"
