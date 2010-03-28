/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2007 Adam Pigg <adam@piggz.co.uk>                                      *
 * Copyright (c) 2007,2008 Casey Link <unnamedrambler@gmail.com>                        *
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

#include "Mp3tunesServiceQueryMaker.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "Mp3tunesMeta.h"
#include "Mp3tunesWorkers.h"
#include "collection/support/MemoryMatcher.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QList>

using namespace Collections;

struct Mp3tunesServiceQueryMaker::Private {
    enum QueryType { NONE, TRACK, ARTIST, ALBUM, COMPOSER, YEAR, GENRE, CUSTOM };
    QueryType type;
    int maxsize;
    bool returnDataPtrs;
};


Mp3tunesServiceQueryMaker::Mp3tunesServiceQueryMaker( Mp3tunesServiceCollection * collection, const QString &sessionId  )
    : DynamicServiceQueryMaker()
        , m_storedTransferJob( 0 )
        , d( new Private )

{
    DEBUG_BLOCK
    m_collection = collection;
    m_sessionId = sessionId;
    reset();
}

Mp3tunesServiceQueryMaker::Mp3tunesServiceQueryMaker( Mp3tunesLocker * locker, const QString &sessionId, Mp3tunesServiceCollection * collection  )
    : DynamicServiceQueryMaker()
        , m_storedTransferJob( 0 )
        , d( new Private )

{
    DEBUG_BLOCK
    m_collection = collection;
    m_sessionId = sessionId;
    m_locker = locker;
    reset();
}

Mp3tunesServiceQueryMaker::~Mp3tunesServiceQueryMaker()
{
    delete d;
}

QueryMaker * Mp3tunesServiceQueryMaker::reset()
{
    DEBUG_BLOCK
    d->type = Private::NONE;
    d->maxsize = -1;
    d->returnDataPtrs = false;
    m_parentArtistId.clear();
    m_parentAlbumId.clear();
    m_artistFilter.clear();

    return this;
}

QueryMaker*
Mp3tunesServiceQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->returnDataPtrs = resultAsDataPtrs;
    return this;
}

void Mp3tunesServiceQueryMaker::run()
{
    DEBUG_BLOCK
    if ( m_storedTransferJob != 0 )
        return;

    m_collection->acquireReadLock();
    //naive implementation, fix this
    //note: we are not handling filtering yet

    if ( d->type == Private::NONE )
        //TODO error handling
        return;
    if (  d->type == Private::ARTIST )
        fetchArtists();
    else if (  d->type == Private::ALBUM )
        fetchAlbums();
    else if (  d->type == Private::TRACK )
        fetchTracks();

    m_collection->releaseLock();
}


void Mp3tunesServiceQueryMaker::abortQuery()
{}

QueryMaker*
Mp3tunesServiceQueryMaker::setQueryType( QueryType type )
{
    switch( type ) {
    case QueryMaker::Artist:
    {
        DEBUG_BLOCK
        d->type = Private::ARTIST;
        return this;
    }

    case QueryMaker::Album:
    {
        DEBUG_BLOCK
        d->type = Private::ALBUM;
        return this;
    }

    case QueryMaker::Track:
    {
        DEBUG_BLOCK
        d->type = Private::TRACK;
        return this;
    }

    case QueryMaker::Genre:
    case QueryMaker::Composer:
    case QueryMaker::Year:
    case QueryMaker::Custom:
    case QueryMaker::None:
    default:
        //TODO: Implement.
        return this;
    }
}



QueryMaker * Mp3tunesServiceQueryMaker::addMatch( const Meta::ArtistPtr & artist )
{
    DEBUG_BLOCK
    if ( m_parentAlbumId.isEmpty() ) {
        const Meta::ServiceArtist * serviceArtist = static_cast< const Meta::ServiceArtist * >( artist.data() );
        m_parentArtistId = QString::number( serviceArtist->id() );
        debug() << "artist parent id set to: " << m_parentArtistId;
    }

    return this;
}

QueryMaker * Mp3tunesServiceQueryMaker::addMatch(const Meta::AlbumPtr & album)
{
    DEBUG_BLOCK
    const Meta::ServiceAlbum * serviceAlbum = static_cast< const Meta::ServiceAlbum * >( album.data() );
    m_parentAlbumId = QString::number( serviceAlbum->id() );
    debug() << "album parent id set to: " << m_parentAlbumId;
    m_parentArtistId.clear();

    return this;
}

template<class PointerType, class ListType>
void Mp3tunesServiceQueryMaker::emitProperResult( const ListType& list )
{
    if ( d->returnDataPtrs ) {
        Meta::DataList data;
        foreach( PointerType p, list )
            data << Meta::DataPtr::staticCast( p );

        emit newResultReady( m_collection->collectionId(), data );
    }
    else
        emit newResultReady( m_collection->collectionId(), list );
}

void Mp3tunesServiceQueryMaker::handleResult()
{
    DEBUG_BLOCK
}

void Mp3tunesServiceQueryMaker::handleResult( const Meta::ArtistList & artists )
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && artists.count() > d->maxsize ) {
        emitProperResult<Meta::ArtistPtr, Meta::ArtistList>( artists.mid( 0, d->maxsize ) );
    } else {
        emitProperResult<Meta::ArtistPtr, Meta::ArtistList>( artists );
    }
}

void Mp3tunesServiceQueryMaker::handleResult( const Meta::AlbumList &albums )
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && albums.count() > d->maxsize ) {
        emitProperResult<Meta::AlbumPtr, Meta::AlbumList>( albums.mid( 0, d->maxsize ) );
    } else {
        emitProperResult<Meta::AlbumPtr, Meta::AlbumList>( albums );
    }
}

void Mp3tunesServiceQueryMaker::handleResult(const Meta::TrackList & tracks)
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && tracks.count() > d->maxsize ) {
        emitProperResult<Meta::TrackPtr, Meta::TrackList>( tracks.mid( 0, d->maxsize ) );
    } else {
        emitProperResult<Meta::TrackPtr, Meta::TrackList>( tracks );
    }
}


void Mp3tunesServiceQueryMaker::fetchArtists()
{
    DEBUG_BLOCK
    if ( !m_artistFilter.isEmpty() )
    {
        debug() << "Artist Filtering";
        Mp3tunesSearchMonkey * searchMonkey = new Mp3tunesSearchMonkey( m_locker, m_artistFilter, Mp3tunesSearchResult::ArtistQuery );
        connect( searchMonkey, SIGNAL( searchComplete( QList<Mp3tunesLockerArtist> ) ), this, SLOT( artistDownloadComplete( QList<Mp3tunesLockerArtist> ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( searchMonkey ); //Go!
    } else if( m_locker->sessionValid() )
    {
        debug() << "Artist Fetching";
        Mp3tunesArtistFetcher * artistFetcher = new Mp3tunesArtistFetcher( m_locker );
        connect( artistFetcher, SIGNAL( artistsFetched( QList<Mp3tunesLockerArtist> ) ), this, SLOT( artistDownloadComplete( QList<Mp3tunesLockerArtist> ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( artistFetcher );
    }
}

void Mp3tunesServiceQueryMaker::fetchAlbums()
{
    DEBUG_BLOCK

    Meta::AlbumList albums;

    debug() << "Fetching Albums for parentArtist id: " << m_parentArtistId;

    if ( !m_parentArtistId.isEmpty() ) {
        albums = matchAlbums( m_collection, m_collection->artistById( m_parentArtistId.toInt() ) );
    } else {
        debug() << "parent id empty";
        return;
    }

    if ( albums.count() > 0 ) {
        handleResult( albums );
    } else if ( m_locker->sessionValid() ) {
        Mp3tunesAlbumWithArtistIdFetcher * albumFetcher = new Mp3tunesAlbumWithArtistIdFetcher( m_locker, m_parentArtistId.toInt() );
        connect( albumFetcher, SIGNAL( albumsFetched( QList<Mp3tunesLockerAlbum> ) ), this, SLOT( albumDownloadComplete( QList<Mp3tunesLockerAlbum> ) ) );

        ThreadWeaver::Weaver::instance()->enqueue( albumFetcher );
    } else {
        debug() << "Session Invalid";
    }
}

void Mp3tunesServiceQueryMaker::fetchTracks()
{
    DEBUG_BLOCK

    Meta::AlbumList albums;
    Meta::TrackList tracks;

    debug() << "album parent id: " << m_parentAlbumId;
    debug() << "artist parent id: " << m_parentArtistId;

    if ( !m_parentArtistId.isEmpty() ) {
        ArtistMatcher artistMatcher( m_collection->artistById( m_parentArtistId.toInt() ) );
        tracks = artistMatcher.match( m_collection->trackMap().values() );
    } else if ( !m_parentAlbumId.isEmpty() ) {
        AlbumMatcher albumMatcher( m_collection->albumById( m_parentAlbumId.toInt() ) );
        tracks = albumMatcher.match( m_collection->trackMap().values() );
    } else {
        debug() << "parent id empty";
        return;
    }

    if ( tracks.count() > 0 ) {
        debug() << tracks.count() << " Tracks selected";
        handleResult( tracks );
        emit queryDone();
    } else if ( m_locker->sessionValid() ) {
        if( !m_parentArtistId.isEmpty() ) {
            debug() << "Creating track w/ artist id Fetch Worker";
            Mp3tunesTrackWithArtistIdFetcher * trackFetcher = new Mp3tunesTrackWithArtistIdFetcher( m_locker, m_parentArtistId.toInt() );
            connect( trackFetcher, SIGNAL( tracksFetched( QList<Mp3tunesLockerTrack> ) ), this, SLOT( trackDownloadComplete( QList<Mp3tunesLockerTrack> ) ) );
            ThreadWeaver::Weaver::instance()->enqueue( trackFetcher ); //Go!
        } else if ( !m_parentAlbumId.isEmpty() ) {
            debug() << "Creating track w/ album id Fetch Worker";
            Mp3tunesTrackWithAlbumIdFetcher * trackFetcher = new Mp3tunesTrackWithAlbumIdFetcher( m_locker, m_parentAlbumId.toInt() );
            connect( trackFetcher, SIGNAL( tracksFetched( QList<Mp3tunesLockerTrack> ) ), this, SLOT( trackDownloadComplete( QList<Mp3tunesLockerTrack> ) ) );
            ThreadWeaver::Weaver::instance()->enqueue( trackFetcher ); //Go!
        }
    } else {
        debug() << "Session Invalid";
        return;
    }
}

void Mp3tunesServiceQueryMaker::artistDownloadComplete( QList<Mp3tunesLockerArtist> artistList )
{
    DEBUG_BLOCK

    Meta::ArtistList artists;

    debug() << "Received artists";
    foreach(const Mp3tunesLockerArtist &artist, artistList) {
        Meta::ServiceArtist * serviceArtist = new Meta::ServiceArtist( artist.artistName() );

        //debug() << "Adding artist: " <<  artist.artistName();

        serviceArtist->setId( artist.artistId() );

        Meta::ArtistPtr artistPtr( serviceArtist );

        artists.push_back( artistPtr );

        m_collection->acquireWriteLock();
        m_collection->addArtist( artistPtr );
        m_collection->releaseLock();

    }

    handleResult( artists );
    emit queryDone();

}

void Mp3tunesServiceQueryMaker::albumDownloadComplete( QList<Mp3tunesLockerAlbum> albumsList )
{
    DEBUG_BLOCK

    debug() << "Received albums";

    Meta::AlbumList albums;
    foreach(const Mp3tunesLockerAlbum &album, albumsList) {

        QString title = album.albumTitle();
        if ( title.contains("* PlayMix") ) continue;
        if ( title.isEmpty() ) title = "Unknown";

        QString albumIdStr = QString::number( album.albumId() );
        int albumId = album.albumId();

        bool hasArt = album.hasArt();

        Meta::Mp3TunesAlbum * serviceAlbum = new Meta::Mp3TunesAlbum( title );

        if ( hasArt )
        {

            QString coverUrl = "http://content.mp3tunes.com/storage/albumartget/<ALBUM_ID>?alternative=1&partner_token=<PARTNER_TOKEN>&sid=<SESSION_ID>";

            coverUrl.replace( "<SESSION_ID>", m_locker->sessionId() );
            coverUrl.replace( "<PARTNER_TOKEN>", m_locker->partnerToken() );
            coverUrl.replace( "<ALBUM_ID>", albumIdStr );

            serviceAlbum->setCoverUrl(coverUrl);
        }

        Meta::AlbumPtr albumPtr( serviceAlbum );

        //debug() << "Adding album: " <<  title;

        serviceAlbum->setId( albumId );
        m_collection->acquireWriteLock();
        m_collection->addAlbum( albumPtr );
        m_collection->releaseLock();

        Meta::ArtistPtr artistPtr = m_collection->artistById( album.artistId() );
        if ( artistPtr.data() != 0 )
        {
           //debug() << "Found parent artist";
            serviceAlbum->setAlbumArtist( artistPtr );
        }

        albums.push_back( albumPtr );

    }

    handleResult( albums );
    emit queryDone();

}

void Mp3tunesServiceQueryMaker::trackDownloadComplete( QList<Mp3tunesLockerTrack> tracksList )
{
    DEBUG_BLOCK
    //debug() << "Received Tracks";

    Meta::TrackList tracks;

     //so lets figure out what we got here:

    foreach(const Mp3tunesLockerTrack &track, tracksList)
    {

        QString title = track.trackTitle();
        if ( title.isEmpty() ) title = "Unknown";

        Meta::Mp3TunesTrack * serviceTrack = new Meta::Mp3TunesTrack( title );
        Meta::TrackPtr trackPtr( serviceTrack );

      //  debug() << "Adding track: " <<  title;

        serviceTrack->setId( track.trackId() );

        serviceTrack->setUidUrl( track.playUrl() );
        serviceTrack->setDownloadableUrl( track.downloadUrl() );

        serviceTrack->setLength( track.trackLength() );

        serviceTrack->setTrackNumber( track.trackNumber() );

        serviceTrack->setYear( QString::number( track.albumYear() ) );

        debug() << "setting type: " << Amarok::extension( track.trackFileName() );
        serviceTrack->setType( Amarok::extension( track.trackFileName() ) );
        //debug() << "set type";
        m_collection->acquireWriteLock();
        //debug() << "adding track";
        m_collection->addTrack( trackPtr );
        //debug() << "added tracktrack";
        m_collection->releaseLock();
        QString albumId = QString::number( track.albumId() );
        QString artistId = QString::number( track.artistId() );

        Meta::ArtistPtr artistPtr = m_collection->artistById( artistId.toInt() );
        if ( artistPtr.data() != 0 ) {
            debug() << "Found parent artist";
            Meta::ServiceArtist *artist = dynamic_cast< Meta::ServiceArtist * > ( artistPtr.data() );
            serviceTrack->setArtist( artistPtr );
            artist->addTrack( trackPtr );
        }

        Meta::AlbumPtr albumPtr = m_collection->albumById( albumId.toInt() );
        if ( albumPtr.data() != 0 ) {
            debug() << "Found parent album";
            Meta::ServiceAlbum *album = dynamic_cast< Meta::ServiceAlbum * > ( albumPtr.data() );
            serviceTrack->setAlbumPtr( albumPtr );
            album->addTrack( trackPtr );
        }

        tracks.push_back( trackPtr );
    }

    //ThreadWeaver::Weaver::instance()->dequeue( job );
    //job->deleteLater();

    handleResult( tracks );
    emit queryDone();

}

QueryMaker * Mp3tunesServiceQueryMaker::addFilter(qint64 value, const QString & filter, bool /*matchBegin*/, bool /*matchEnd*/)
{
    DEBUG_BLOCK
    //debug() << "value: " << value;
    //for now, only accept artist filters
    if ( value == Meta::valArtist ) {
        //debug() << "Filter: " << filter;
        m_artistFilter = filter;
    }
    return this;
}

int Mp3tunesServiceQueryMaker::validFilterMask()
{
    //we only supprt artist filters for now...
    return ArtistFilter;
}

#include "Mp3tunesServiceQueryMaker.moc"

