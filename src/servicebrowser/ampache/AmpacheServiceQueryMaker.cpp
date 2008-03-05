/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2007  Adam Pigg <adam@piggz.co.uk>                      *
 *             (c) 2007  Casey Link <unnamedrambler@gmail.com>             *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "AmpacheServiceQueryMaker.h"

#include "amarok.h"
#include "debug.h"
#include "AmpacheMeta.h"
#include "collection/support/MemoryMatcher.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QDomDocument>

using namespace Meta;

struct AmpacheServiceQueryMaker::Private {
    enum QueryType { NONE, TRACK, ARTIST, ALBUM, COMPOSER, YEAR, GENRE, CUSTOM };
    QueryType type;
    int maxsize;
    bool returnDataPtrs;
};


AmpacheServiceQueryMaker::AmpacheServiceQueryMaker( AmpacheServiceCollection * collection, const QString &server, const QString &sessionId  )
 : DynamicServiceQueryMaker()
 , m_storedTransferJob( 0 )
 , d( new Private )
 , m_server( server )
 , m_sessionId( sessionId )

{
    DEBUG_BLOCK
    m_collection = collection;
    reset();
}


AmpacheServiceQueryMaker::~AmpacheServiceQueryMaker()
{
    delete d;
}

QueryMaker * AmpacheServiceQueryMaker::reset()
{
    d->type = Private::NONE;
    d->maxsize = -1;
    d->returnDataPtrs = false;
    m_parentArtistId = QString();
    m_parentAlbumId = QString();

    m_artistFilter = QString();
    //m_lastArtistFilter = QString(); this one really should survive a reset....

    return this;
}

QueryMaker*
AmpacheServiceQueryMaker::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->returnDataPtrs = resultAsDataPtrs;
    return this;
}

void AmpacheServiceQueryMaker::run()
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


void AmpacheServiceQueryMaker::abortQuery()
{
}

QueryMaker * AmpacheServiceQueryMaker::startArtistQuery()
{
    DEBUG_BLOCK
    d->type = Private::ARTIST;
    return this;
}

QueryMaker * AmpacheServiceQueryMaker::startAlbumQuery()
{
    DEBUG_BLOCK
    d->type = Private::ALBUM;
    return this;
}

QueryMaker * AmpacheServiceQueryMaker::startTrackQuery()
{
    DEBUG_BLOCK
    d->type = Private::TRACK;
    return this;
}



QueryMaker * AmpacheServiceQueryMaker::addMatch( const ArtistPtr & artist )
{
    DEBUG_BLOCK

    if ( m_parentAlbumId.isEmpty() ) {
        const ServiceArtist * serviceArtist = static_cast< const ServiceArtist * >( artist.data() );
        m_parentArtistId = QString::number( serviceArtist->id() );
        debug() << "parent id set to: " << m_parentArtistId;
    }


    return this;
}

QueryMaker * AmpacheServiceQueryMaker::addMatch(const Meta::AlbumPtr & album)
{
    DEBUG_BLOCK
    const ServiceAlbum * serviceAlbum = static_cast< const ServiceAlbum * >( album.data() );
    m_parentAlbumId = QString::number( serviceAlbum->id() );
    debug() << "parent id set to: " << m_parentAlbumId;
    m_parentArtistId = QString();


    return this;
}





// What's worse, a bunch of almost identical repeated code, or a not so obvious macro? :-)
// The macro below will emit the proper result signal. If m_resultAsDataPtrs is true,
// it'll emit the signal that takes a list of DataPtrs. Otherwise, it'll call the
// signal that takes the list of the specific class.
// (copied from sqlquerybuilder.cpp with a few minor tweaks)

#define emitProperResult( PointerType, list ) { \
            if ( d->returnDataPtrs ) { \
                DataList data; \
                foreach( PointerType p, list ) { \
                    data << DataPtr::staticCast( p ); \
                } \
                emit newResultReady( m_collection->collectionId(), data ); \
            } \
            else { \
                emit newResultReady( m_collection->collectionId(), list ); \
            } \
        }


void AmpacheServiceQueryMaker::handleResult() {
    DEBUG_BLOCK

}

void AmpacheServiceQueryMaker::handleResult( const ArtistList & artists )
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && artists.count() > d->maxsize ) {
        emitProperResult( ArtistPtr, artists.mid( 0, d->maxsize ) );
    } else
        emitProperResult( ArtistPtr, artists );
}

void AmpacheServiceQueryMaker::handleResult( const AlbumList &albums )
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && albums.count() > d->maxsize ) {
        emitProperResult( AlbumPtr, albums.mid( 0, d->maxsize ) );
    } else
        emitProperResult( AlbumPtr, albums );
}

void AmpacheServiceQueryMaker::handleResult(const TrackList & tracks)
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && tracks.count() > d->maxsize ) {
        emitProperResult( TrackPtr, tracks.mid( 0, d->maxsize ) );
    } else
        emitProperResult( TrackPtr, tracks );
}


void AmpacheServiceQueryMaker::fetchArtists()
{
    DEBUG_BLOCK

    //this stuff causes crashes and "whiteouts" and will need to change anyway if the Ampache API is updated to
    // allow multilevel filtering. Hence it is commented out but left in for future reference
    /*if ( ( m_collection->artistMap().values().count() != 0 ) && ( m_artistFilter == m_lastArtistFilter ) ) {
        handleResult( m_collection->artistMap().values() );
        debug() << "no need to fetch artists again! ";
        debug() << "    filter: " << m_artistFilter;
        debug() << "    last filter: " << m_lastArtistFilter;

    }*/
    //else {

        QString urlString = "<SERVER>/server/xml.server.php?action=artists&auth=<SESSION_ID>";

        urlString.replace( "<SERVER>", m_server);
        urlString.replace( "<SESSION_ID>", m_sessionId);

        if ( !m_artistFilter.isEmpty() ) {
            urlString += QString( "&filter=" + m_artistFilter );
        }

        debug() << "Artist url: " << urlString;


        m_storedTransferJob =  KIO::storedGet(  KUrl( urlString ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
            , this, SLOT( artistDownloadComplete( KJob *) ) );
    //}

    m_lastArtistFilter = m_artistFilter;
}

void AmpacheServiceQueryMaker::fetchAlbums()
{
    DEBUG_BLOCK

    AlbumList albums;


    //debug() << "parent id: " << m_parentId;

    if ( !m_parentArtistId.isEmpty() ) {
        ArtistMatcher artistMatcher( m_collection->artistById( m_parentArtistId.toInt() ) );
        albums = artistMatcher.matchAlbums( m_collection );
    } else
        return;

    if ( albums.count() > 0 ) {
        handleResult( albums );
    } else {

        QString urlString = "<SERVER>/server/xml.server.php?action=artist_albums&auth=<SESSION_ID>&filter=<FILTER>";

        urlString.replace( "<SERVER>", m_server);
        urlString.replace( "<SESSION_ID>", m_sessionId);
        urlString.replace( "<FILTER>", m_parentArtistId );

        debug() << "request url: " << urlString;

        m_storedTransferJob =  KIO::storedGet(  KUrl( urlString ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
            , this, SLOT( albumDownloadComplete( KJob *) ) );
    }
}

void AmpacheServiceQueryMaker::fetchTracks()
{
    DEBUG_BLOCK

    TrackList tracks;

    debug() << "parent album id: " << m_parentAlbumId;

    if ( !m_parentAlbumId.isEmpty() ) {
        AlbumMatcher albumMatcher( m_collection->albumById( m_parentAlbumId.toInt() ) );
        tracks = albumMatcher.match( m_collection );
    } else
        return;

    if ( tracks.count() > 0 ) {
        handleResult( tracks );
    } else {

        QString urlString = "<SERVER>/server/xml.server.php?action=album_songs&auth=<SESSION_ID>&filter=<FILTER>";

        urlString.replace( "<SERVER>", m_server);
        urlString.replace( "<SESSION_ID>", m_sessionId);
        if(  !m_parentAlbumId.isEmpty() )
            urlString.replace( "<FILTER>", m_parentAlbumId );
        else
            urlString.replace( "<FILTER>", m_parentArtistId );

        m_storedTransferJob =  KIO::storedGet(  KUrl( urlString ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
            , this, SLOT( trackDownloadComplete( KJob *) ) );
    }
}



void AmpacheServiceQueryMaker::artistDownloadComplete(KJob * job)
{
    DEBUG_BLOCK

    if( job->error() )
    {
        error() << job->error();
        m_storedTransferJob->deleteLater();
        return;
    }

    ArtistList artists;

    //debug() << "recieved artists: " <<  m_storedTransferJob->data();

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( m_storedTransferJob->data() );
    QDomElement root = doc.firstChildElement("root");

    QDomNode n = root.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        //if ( ! (e.tagName() == "item") )
        //    break;

        QDomElement element = n.firstChildElement("name");
        ServiceArtist * artist = new ServiceArtist( element.text() );

        int artistId = e.attribute( "id", "0").toInt();

        //debug() << "Adding artist: " << element.text() << " with id: " << artistId;

        artist->setId( artistId );

        ArtistPtr artistPtr( artist );

        artists.push_back( artistPtr );

        m_collection->addArtist( artist->name(),  artistPtr );

        n = n.nextSibling();
    }

   m_storedTransferJob->deleteLater();

   handleResult( artists );
   emit queryDone();

}

void AmpacheServiceQueryMaker::albumDownloadComplete(KJob * job)
{
    DEBUG_BLOCK

    if( job->error() )
    {
        error() << job->error();
        m_storedTransferJob->deleteLater();
        return;
    }

    //debug() << "Recieved response: " << m_storedTransferJob->data();

    AlbumList albums;

    //debug() << "recieved artists: " <<  m_storedTransferJob->data();

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( m_storedTransferJob->data() );
    QDomElement root = doc.firstChildElement( "root" );


    QDomNode n = root.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        //if ( ! (e.tagName() == "item") )
        //    break;

        QDomElement element = n.firstChildElement( "name" );

        QString title = element.text();
        if ( title.isEmpty() ) title = "Unknown";


        int albumId = e.attribute( "id", "0" ).toInt();


        AmpacheAlbum * album = new AmpacheAlbum( title );
        //ServiceAlbum * album = new ServiceAlbum( title );
        album->setId( albumId );


        element = n.firstChildElement("art");

        QString coverUrl = element.text();


        AlbumPtr albumPtr( album );

        //debug() << "Adding album: " <<  title;
        //debug() << "   Id: " <<  albumId;
        //debug() << "   Cover url: " <<  coverUrl;


        m_collection->addAlbum( title,  albumPtr );

        element = n.firstChildElement( "artist" );

        int artistId = element.attribute( "id", "0" ).toInt();
        debug() << "   Artist id: " <<  artistId;


        ArtistPtr artistPtr = m_collection->artistById( m_parentArtistId.toInt() );
        if ( artistPtr.data() != 0 )
        {
           //debug() << "Found parent artist";
           //ServiceArtist *artist = dynamic_cast< ServiceArtist * > ( artistPtr.data() );
           album->setAlbumArtist( artistPtr );
        }

        album->setCoverUrl( coverUrl );

        albums.push_back( albumPtr );

        n = n.nextSibling();
    }

   m_storedTransferJob->deleteLater();

   handleResult( albums );
   emit queryDone();

}

void AmpacheServiceQueryMaker::trackDownloadComplete(KJob * job)
{
    DEBUG_BLOCK

    if( job->error() )
    {
        error() << job->error();
        m_storedTransferJob->deleteLater();
        return;
    }

    //debug() << "Recieved response: " << m_storedTransferJob->data();

    TrackList tracks;

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( m_storedTransferJob->data() );
    QDomElement root = doc.firstChildElement("root");

    QDomNode n = root.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        //if ( ! (e.tagName() == "item") )
        //    break;

        int trackId = e.attribute( "id", "0").toInt();


        QDomElement element = n.firstChildElement("title");

        QString title = element.text();
        if ( title.isEmpty() ) title = "Unknown";

        AmpacheTrack * track = new AmpacheTrack( title  );
        TrackPtr trackPtr( track );

        //debug() << "Adding track: " <<  title;

        track->setId( trackId );

        m_collection->addTrack( element.text(),  trackPtr );


        element = n.firstChildElement("url");
        track->setUrl( element.text() );

        element = n.firstChildElement("time");
        track->setLength( element.text().toInt() );

        element = n.firstChildElement("track");
        track->setTrackNumber( element.text().toInt() );


        QDomElement albumElement = n.firstChildElement("album");
        int albumId = albumElement.attribute( "id", "0").toInt();

        QDomElement artistElement = n.firstChildElement("artist");
        int artistId = artistElement.attribute( "id", "0").toInt();


        ArtistPtr artistPtr = m_collection->artistById( artistId );
        if ( artistPtr.data() != 0 ) {
            //debug() << "Found parent artist " << artistPtr->name();
           ServiceArtist *artist = dynamic_cast< ServiceArtist * > ( artistPtr.data() );
           track->setArtist( artistPtr );
           artist->addTrack( trackPtr );
        }

        AlbumPtr albumPtr = m_collection->albumById( albumId );
        if ( albumPtr.data() != 0 ) {
           //debug() << "Found parent album " << albumPtr->name() ;
           ServiceAlbum *album = dynamic_cast< ServiceAlbum * > ( albumPtr.data() );
           track->setAlbum( albumPtr );
           album->addTrack( trackPtr );
        }

        tracks.push_back( trackPtr );

        n = n.nextSibling();
    }

   m_storedTransferJob->deleteLater();

   handleResult( tracks );
   emit queryDone();

}

QueryMaker * AmpacheServiceQueryMaker::addFilter(qint64 value, const QString & filter, bool matchBegin, bool matchEnd)
{
    DEBUG_BLOCK
    //for now, only accept artist filters
    if ( value == valArtist ) {
        debug() << "Filter: " << filter;
        m_artistFilter = filter;
    }
    return this;
}

#include "AmpacheServiceQueryMaker.moc"


