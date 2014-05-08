/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2007 Adam Pigg <adam@piggz.co.uk>                                      *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 *           (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "AmpacheServiceQueryMaker"

#include "AmpacheServiceQueryMaker.h"

#include "AmpacheMeta.h"
#include "core/meta/Statistics.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/meta/support/MetaConstants.h"
#include "core-impl/collections/support/MemoryMatcher.h"

#include <QSet>
#include <QAtomicInt>
#include <QDomDocument>

using namespace Collections;

struct AmpacheServiceQueryMaker::Private
{
    AmpacheServiceCollection* collection;

    QueryMaker::QueryType type;
    int maxsize;

    QAtomicInt expectedReplies;

    QString server;
    QString sessionId;
    QList<int> parentTrackIds;
    QList<int> parentAlbumIds;
    QList<int> parentArtistIds;
    uint dateFilter;
    QString artistFilter;
    QString albumFilter;

    /** We are collecting the results of the queries and submit them
        in one block to ensure that we don't report albums twice
        and because the CollectionTreeItemModelBase does not handle
        multiple results correctly (which it should).
    */
    Meta::AlbumList albumResults;
    Meta::ArtistList artistResults;
    Meta::TrackList trackResults;
};

AmpacheServiceQueryMaker::AmpacheServiceQueryMaker( AmpacheServiceCollection * collection, const QString &server, const QString &sessionId  )
    : DynamicServiceQueryMaker()
    , d( new Private )
{
    d->collection = collection;
    d->type = QueryMaker::None;
    d->maxsize = 0;
    d->server = server;
    d->sessionId = sessionId;
    d->dateFilter = 0;
}

AmpacheServiceQueryMaker::~AmpacheServiceQueryMaker()
{
    delete d;
}

void
AmpacheServiceQueryMaker::run()
{
    DEBUG_BLOCK

    if( d->expectedReplies ) // still running an old query
        return;

    //naive implementation, fix this
    //note: we are not handling filtering yet

    d->collection->acquireReadLock();

    if (  d->type == QueryMaker::Artist )
        fetchArtists();
    else if( d->type == QueryMaker::Album )
        fetchAlbums();
    else if( d->type == QueryMaker::Track )
        fetchTracks();
    else
        warning() << "Requested unhandled query type"; //TODO error handling

     d->collection->releaseLock();
}

void
AmpacheServiceQueryMaker::abortQuery()
{
}

QueryMaker *
AmpacheServiceQueryMaker::setQueryType( QueryType type )
{
    d->type = type;

    return this;
}

QueryMaker*
AmpacheServiceQueryMaker::addMatch( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    const Meta::AmpacheTrack* serviceTrack = dynamic_cast< const Meta::AmpacheTrack * >( track.data() );
    if( serviceTrack )
    {
        d->parentTrackIds << serviceTrack->id();
        debug() << "parent id set to: " << d->parentTrackIds;
    }
    else
    {
        // searching for something from another collection
        //hmm, not sure what to do now
    }

    return this;
}

QueryMaker*
AmpacheServiceQueryMaker::addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour )
{
    Q_UNUSED( behaviour ) // TODO
    DEBUG_BLOCK

    if( d->parentAlbumIds.isEmpty() )
    {
        const Meta::AmpacheArtist* serviceArtist = dynamic_cast< const Meta::AmpacheArtist * >( artist.data() );
        if( serviceArtist )
        {
            d->parentArtistIds << serviceArtist->id();
        }
        else
        {
            // searching for something from another collection
            if( d->collection->artistMap().contains( artist->name() ) )
            {
                serviceArtist = static_cast< const Meta::AmpacheArtist* >( d->collection->artistMap().value( artist->name() ).data() );
                d->parentArtistIds << serviceArtist->id();
            }
            else
            {
                //hmm, not sure what to do now
            }
        }
    }
    return this;
}

QueryMaker *
AmpacheServiceQueryMaker::addMatch( const Meta::AlbumPtr & album )
{
    DEBUG_BLOCK
    const Meta::AmpacheAlbum* serviceAlbum = dynamic_cast< const Meta::AmpacheAlbum * >( album.data() );
    if( serviceAlbum )
    {
        d->parentAlbumIds << serviceAlbum->ids();
        debug() << "parent id set to: " << d->parentAlbumIds;
        d->parentArtistIds.clear();
    }
    else
    {
        // searching for something from another collection
        if( d->collection->albumMap().contains( album ) )  // compares albums by value
        {
            serviceAlbum = static_cast< const Meta::AmpacheAlbum* >( d->collection->albumMap().value( album ).data() );
            d->parentAlbumIds << serviceAlbum->ids();
            d->parentArtistIds.clear();
        }
        else
        {
            //hmm, not sure what to do now
        }
    }

    return this;
}

void
AmpacheServiceQueryMaker::fetchArtists()
{
    DEBUG_BLOCK

    Meta::ArtistList artists;

    // first try the cache
    if( !d->parentArtistIds.isEmpty() )
    {
        foreach( int artistId, d->parentArtistIds )
            artists << d->collection->artistById( artistId );
    }

    if( !artists.isEmpty() )
    {
        debug() << "got" << artists.count() << "artists from the memory collection";
        emit newResultReady( artists );
        emit queryDone();
        return;
    }

    KUrl request = getRequestUrl( "artists" );

    if ( !d->artistFilter.isEmpty() )
        request.addQueryItem( "filter", d->artistFilter );

    d->expectedReplies.ref();
    The::networkAccessManager()->getData( request, this,
                                          SLOT(artistDownloadComplete(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
AmpacheServiceQueryMaker::fetchAlbums()
{
    DEBUG_BLOCK

    Meta::AlbumList albums;

    // first try the cache
    if( !d->parentArtistIds.isEmpty() )
    {
        foreach( int artistId, d->parentArtistIds )
            albums << matchAlbums( d->collection, d->collection->artistById( artistId ) );
    }
    if( !albums.isEmpty() )
    {
        debug() << "got" << albums.count() << "albums from the memory collection";
        emit newResultReady( albums );
        emit queryDone();
        return;
    }

    if( !d->parentArtistIds.isEmpty() )
    {
        foreach( int id, d->parentArtistIds )
        {
            KUrl request = getRequestUrl( "artist_albums" );
            request.addQueryItem( "filter", QString::number( id ) );

            d->expectedReplies.ref();
            The::networkAccessManager()->getData( request, this,
                                                  SLOT(albumDownloadComplete(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
        }
    }
    else
    {
        KUrl request = getRequestUrl( "albums" );

        if ( !d->albumFilter.isEmpty() )
            request.addQueryItem( "filter", d->albumFilter );

        d->expectedReplies.ref();
        The::networkAccessManager()->getData( request, this,
                                              SLOT(albumDownloadComplete(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
    }
}

void
AmpacheServiceQueryMaker::fetchTracks()
{
    DEBUG_BLOCK

    Meta::TrackList tracks;

    //debug() << "parent album id: " << d->parentAlbumId;

    // first try the cache
    // TODO: this is fishy as we cannot be sure that the cache contains
    //       everything
    //       we should cache database query results instead
    if( !d->parentTrackIds.isEmpty() )
    {
        foreach( int trackId, d->parentTrackIds )
        {
            tracks << d->collection->trackById( trackId );
        }
    }
    else if( !d->parentAlbumIds.isEmpty() )
    {
        foreach( int albumId, d->parentAlbumIds )
        {
            AlbumMatcher albumMatcher( d->collection->albumById( albumId ) );
            tracks << albumMatcher.match( d->collection->trackMap().values() );
        }
    }
    else if( d->parentArtistIds.isEmpty() )
    {
        foreach( int artistId, d->parentArtistIds )
        {
            ArtistMatcher artistMatcher( d->collection->artistById( artistId ) );
            tracks << artistMatcher.match( d->collection->trackMap().values() );
        }
    }

    if( !tracks.isEmpty() )
    {
        debug() << "got" << tracks.count() << "tracks from the memory collection";
        emit newResultReady( tracks );
        emit queryDone();
        return;
    }

    KUrl request = getRequestUrl();


    if( !d->parentAlbumIds.isEmpty() )
    {
        foreach( int id, d->parentAlbumIds )
        {
            KUrl request = getRequestUrl( "album_songs" );
            request.addQueryItem( "filter", QString::number( id ) );

            d->expectedReplies.ref();
            The::networkAccessManager()->getData( request, this,
                                                  SLOT(trackDownloadComplete(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
        }
    }
    else if( !d->parentArtistIds.isEmpty() )
    {
        foreach( int id, d->parentArtistIds )
        {
            KUrl request = getRequestUrl( "artist_songs" );
            request.addQueryItem( "filter", QString::number( id ) );

            d->expectedReplies.ref();
            The::networkAccessManager()->getData( request, this,
                                                  SLOT(trackDownloadComplete(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
        }
    }
    else
    {
        KUrl request = getRequestUrl( "songs" );

        d->expectedReplies.ref();
        The::networkAccessManager()->getData( request, this,
                                              SLOT(trackDownloadComplete(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
    }
}

void
AmpacheServiceQueryMaker::artistDownloadComplete( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url );

    if( e.code != QNetworkReply::NoError )
    {
        warning() << "Artist download error:" << e.description;
        if( !d->expectedReplies.deref() )
            emit queryDone();
        return;
    }

    // DEBUG_BLOCK

    // so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( data );
    QDomElement root = doc.firstChildElement( "root" );

    // Is this an error, if so we need to 'un-ready' the service and re-authenticate before contiuning
    QDomElement domError = root.firstChildElement( "error" );

    if ( !domError.isNull() )
    {
        warning() << "Error getting Artist List" << domError.text();
        AmpacheService *parentService = dynamic_cast< AmpacheService * >( d->collection->service() );
        if( parentService == 0 )
            return;
        else
            parentService->reauthenticate();
    }

    for( QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.

        QDomElement element = n.firstChildElement( "name" );
        int artistId = e.attribute( "id", "0").toInt();

        // check if we have the artist already
        Meta::ArtistPtr artistPtr = d->collection->artistById( artistId );

        if( !artistPtr )
        {
            // new artist
            Meta::ServiceArtist* artist = new Meta::AmpacheArtist( element.text(), d->collection->service() );
            artist->setId( artistId );

            // debug() << "Adding artist: " << element.text() << " with id: " << artistId;

            artistPtr = artist;

            d->collection->acquireWriteLock();
            d->collection->addArtist( artistPtr );
            d->collection->releaseLock();
        }

        if( !d->artistResults.contains( artistPtr ) )
            d->artistResults.push_back( artistPtr );
    }

    if( !d->expectedReplies.deref() )
    {
        emit newResultReady( d->artistResults );
        emit queryDone();
        d->artistResults.clear();
    }
}

void
AmpacheServiceQueryMaker::albumDownloadComplete( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url );

    if( e.code != QNetworkReply::NoError )
    {
        warning() << "Album download error:" << e.description;
        if( !d->expectedReplies.deref() )
            emit queryDone();
        return;
    }

    // DEBUG_BLOCK

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( data );
    QDomElement root = doc.firstChildElement( "root" );

    // Is this an error, if so we need to 'un-ready' the service and re-authenticate before contiuning
    QDomElement domError = root.firstChildElement( "error" );

    if( !domError.isNull() )
    {
        warning() << "Error getting Album List" << domError.text();
        AmpacheService *parentService = dynamic_cast< AmpacheService * >(d->collection->service());
        if( parentService == 0 )
            return;
        else
            parentService->reauthenticate();
    }

    for( QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.

        // --- the album artist
        Meta::ArtistPtr artistPtr;
        QDomElement artistElement = n.firstChildElement( "artist" );
        if( !artistElement.isNull() )
        {
            int artistId = artistElement.attribute( "id", "0").toInt();
            // check if we already know the artist
            artistPtr = d->collection->artistById( artistId );
            if( !artistPtr.data() )
            {
                // new artist.
                Meta::ServiceArtist* artist = new Meta::AmpacheArtist( artistElement.text(), d->collection->service() );
                artistPtr = artist;

                artist->setId( artistId );
                // debug() << "Adding artist: " << artistElement.text() << " with id: " << artistId;

                d->collection->acquireWriteLock();
                d->collection->addArtist( artistPtr );
                d->collection->releaseLock();
            }
        }

        QDomElement element = n.firstChildElement( "name" );
        QString title = element.text();

        Meta::AmpacheAlbum::AmpacheAlbumInfo info;
        info.id = e.attribute( "id", "0" ).toInt();

        element = n.firstChildElement( "disk" );
        info.discNumber = element.text().toInt();

        element = n.firstChildElement( "year" );
        info.year = element.text().toInt();

        // check if we have the album already
        Meta::AlbumPtr albumPtr = d->collection->albumById( info.id );

        if( !albumPtr )
        {
            // check if we at least have an album with the same title and artist
            Meta::AmpacheAlbum* album = static_cast<Meta::AmpacheAlbum*>(
            const_cast<Meta::Album*>( d->collection->albumMap().value( title, artistPtr ? artistPtr->name() : QString() ).data() ) );

            if( !album )
            {
                // new album
                album = new Meta::AmpacheAlbum( title );
                album->setAlbumArtist( artistPtr );

                // -- cover
                element = n.firstChildElement( "art" );

                QString coverUrl = element.text();
                album->setCoverUrl( coverUrl );
            }
            album->addInfo( info );

            // debug() << "Adding album" << title << "with id:" << info.id;

            albumPtr = album;

            // register a new id with the ServiceCollection
            album->setId( info.id );
            d->collection->acquireWriteLock();
            d->collection->addAlbum( albumPtr );
            d->collection->releaseLock();
        }

        if( !d->albumResults.contains( albumPtr ) )
            d->albumResults.push_back( albumPtr );
    }

    if( !d->expectedReplies.deref() )
    {
        emit newResultReady( d->albumResults );
        emit queryDone();
        d->albumResults.clear();
    }
}

void
AmpacheServiceQueryMaker::trackDownloadComplete( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url );

    if( e.code != QNetworkReply::NoError )
    {
        warning() << "Track download error:" << e.description;
        if( !d->expectedReplies.deref() )
            emit queryDone();
        return;
    }

    // DEBUG_BLOCK

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( data );
    QDomElement root = doc.firstChildElement( "root" );

    // Is this an error, if so we need to 'un-ready' the service and re-authenticate before contiuning
    QDomElement domError = root.firstChildElement( "error" );

    if( !domError.isNull() )
    {
        warning() << "Error getting Track Download " << domError.text();
        AmpacheService *parentService = dynamic_cast< AmpacheService * >( d->collection->service() );
        if( parentService == 0 )
            return;
        else
            parentService->reauthenticate();
    }

    for( QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.

        int trackId = e.attribute( "id", "0" ).toInt();
        Meta::TrackPtr trackPtr = d->collection->trackById( trackId );

        if( !trackPtr )
        {
            // new track

            QDomElement element = n.firstChildElement( "title" );
            QString title = element.text();
            Meta::AmpacheTrack * track = new Meta::AmpacheTrack( title, d->collection->service() );
            trackPtr = track;

            track->setId( trackId );

            element = n.firstChildElement( "url" );
            track->setUidUrl( element.text() );

            element = n.firstChildElement( "time" );
            track->setLength( element.text().toInt() * 1000 );

            element = n.firstChildElement( "track" );
            track->setTrackNumber( element.text().toInt() );

            element = n.firstChildElement( "rating" );
            track->statistics()->setRating( element.text().toDouble() * 2.0 );

            QDomElement albumElement = n.firstChildElement( "album" );
            int albumId = albumElement.attribute( "id", "0").toInt();

            QDomElement artistElement = n.firstChildElement( "artist" );
            int artistId = artistElement.attribute( "id", "0").toInt();

            Meta::ArtistPtr artistPtr = d->collection->artistById( artistId );
            // TODO: this assumes that we query all artist before tracks
            if( artistPtr )
            {
                // debug() << "Found parent artist " << artistPtr->name();
                Meta::ServiceArtist *artist = dynamic_cast< Meta::ServiceArtist * > ( artistPtr.data() );
                track->setArtist( artistPtr );
                artist->addTrack( trackPtr );
            }

            Meta::AlbumPtr albumPtr = d->collection->albumById( albumId );
            // TODO: this assumes that we query all albums before tracks
            if( albumPtr )
            {
                // debug() << "Found parent album " << albumPtr->name() << albumId;
                Meta::AmpacheAlbum *album = dynamic_cast< Meta::AmpacheAlbum * > ( albumPtr.data() );
                track->setDiscNumber( album->getInfo( albumId ).discNumber );
                track->setYear( album->getInfo( albumId ).year );
                track->setAlbumPtr( albumPtr );
                // debug() << " parent album with"<<track->discNumber()<<track->year();
                album->addTrack( trackPtr );
            }

            // debug() << "Adding track: " <<  title << " with id: " << trackId;

            d->collection->acquireWriteLock();
            d->collection->addTrack( trackPtr );
            d->collection->releaseLock();
        }

        if( !d->trackResults.contains( trackPtr ) )
            d->trackResults.push_back( trackPtr );
    }

    if( !d->expectedReplies.deref() )
    {
        emit newResultReady( d->trackResults );
        emit queryDone();
        d->trackResults.clear();
    }
}

QueryMaker *
AmpacheServiceQueryMaker::addFilter( qint64 value, const QString & filter, bool matchBegin, bool matchEnd )
{
    Q_UNUSED( matchBegin )
    Q_UNUSED( matchEnd )

    //for now, only accept artist filters
    // TODO: What about albumArtist?
    if( value == Meta::valArtist )
    {
        d->artistFilter = filter;
    }
    else if( value == Meta::valAlbum )
    {
        d->albumFilter = filter;
    }
    else
    {
        warning() << "unsupported filter" << Meta::nameForField( value );
    }
    return this;
}

QueryMaker*
AmpacheServiceQueryMaker::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    if( value == Meta::valCreateDate && compare == QueryMaker::GreaterThan )
    {
        debug() << "asking to filter based on added date";
        d->dateFilter = filter;
        debug() << "setting dateFilter to:" << d->dateFilter;
    }
    else
    {
        warning() << "unsupported filter" << Meta::nameForField( value );
    }
    return this;
}

int
AmpacheServiceQueryMaker::validFilterMask()
{
    //we only supprt artist and album filters for now...
    return ArtistFilter | AlbumFilter;
}

QueryMaker *
AmpacheServiceQueryMaker::limitMaxResultSize( int size )
{
    d->maxsize = size;
    return this;
}

KUrl
AmpacheServiceQueryMaker::getRequestUrl( const QString &action ) const
{
    QString path = d->server;

    if( !path.startsWith("http://") && !path.startsWith("https://") )
        path = "http://" + path;

    KUrl url( path );

    url.addPath( "/server/xml.server.php" );
    url.addQueryItem( "auth", d->sessionId );

    if( !action.isEmpty() )
        url.addQueryItem( "action", action );

    if( d->dateFilter > 0 )
    {
        QDateTime from;
        from.setTime_t( d->dateFilter );
        url.addQueryItem( "add", from.toString( Qt::ISODate ) );
    }
    url.addQueryItem( "limit", QString::number( d->maxsize ) );

    return url;
}

#include "AmpacheServiceQueryMaker.moc"

