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
    Q_UNUSED( tracklist );
    // TODO: generically implement, abstract from MediaDeviceHandler
    return;
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
        //m_titlemap.insert( track->name(), TrackPtr::staticCast( track ) );

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




#include "MediaDeviceHandler.moc"
