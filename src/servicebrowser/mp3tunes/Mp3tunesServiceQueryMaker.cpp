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

#include "Mp3tunesServiceQueryMaker.h"

#include "Amarok.h"
#include "debug.h"
#include "Mp3tunesMeta.h"
#include "collection/support/MemoryMatcher.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QDomDocument>

using namespace Meta;

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


Mp3tunesServiceQueryMaker::~Mp3tunesServiceQueryMaker()
{
    delete d;
}

QueryMaker * Mp3tunesServiceQueryMaker::reset()
{
    d->type = Private::NONE;
    d->maxsize = -1;
    d->returnDataPtrs = false;
    m_parentArtistId = QString();
    m_parentAlbumId = QString();
    m_artistFilter = QString();

    return this;
}

QueryMaker*
Mp3tunesServiceQueryMaker::returnResultAsDataPtrs( bool resultAsDataPtrs )
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
{
}

QueryMaker * Mp3tunesServiceQueryMaker::startArtistQuery()
{
    DEBUG_BLOCK
    d->type = Private::ARTIST;
    return this;
}

QueryMaker * Mp3tunesServiceQueryMaker::startAlbumQuery()
{
    DEBUG_BLOCK
    d->type = Private::ALBUM;
    return this;
}

QueryMaker * Mp3tunesServiceQueryMaker::startTrackQuery()
{
    DEBUG_BLOCK
    d->type = Private::TRACK;
    return this;
}



QueryMaker * Mp3tunesServiceQueryMaker::addMatch( const ArtistPtr & artist )
{
    DEBUG_BLOCK

    if ( m_parentAlbumId.isEmpty() ) {
        const ServiceArtist * serviceArtist = static_cast< const ServiceArtist * >( artist.data() );
        m_parentArtistId = QString::number( serviceArtist->id() );
        //debug() << "parent id set to: " << m_parentArtistId;
    }


    return this;
}

QueryMaker * Mp3tunesServiceQueryMaker::addMatch(const Meta::AlbumPtr & album)
{
    DEBUG_BLOCK
    const ServiceAlbum * serviceAlbum = static_cast< const ServiceAlbum * >( album.data() );
    m_parentAlbumId = QString::number( serviceAlbum->id() );
    //debug() << "parent id set to: " << m_parentAlbumId;
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


void Mp3tunesServiceQueryMaker::handleResult() {
    DEBUG_BLOCK

}

void Mp3tunesServiceQueryMaker::handleResult( const ArtistList & artists )
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && artists.count() > d->maxsize ) {
        emitProperResult( ArtistPtr, artists.mid( 0, d->maxsize ) );
    } else 
        emitProperResult( ArtistPtr, artists );
}

void Mp3tunesServiceQueryMaker::handleResult( const AlbumList &albums )
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && albums.count() > d->maxsize ) {
        emitProperResult( AlbumPtr, albums.mid( 0, d->maxsize ) );
    } else 
        emitProperResult( AlbumPtr, albums );
}

void Mp3tunesServiceQueryMaker::handleResult(const TrackList & tracks)
{    
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && tracks.count() > d->maxsize ) {
        emitProperResult( TrackPtr, tracks.mid( 0, d->maxsize ) );
    } else 
        emitProperResult( TrackPtr, tracks ); 
}


void Mp3tunesServiceQueryMaker::fetchArtists()
{
    DEBUG_BLOCK
    /*if ( m_collection->artistMap().values().count() != 0 && m_artistFilter.isEmpty()) {
        handleResult( m_collection->artistMap().values() );
        debug() << "no need to fetch artists again! ";
    }*/
    //else {

        QString urlString = "http://ws.mp3tunes.com/api/v1/lockerData?sid=<SESSION_ID>&partner_token=<PARTNER_TOKEN>&output=xml&type=artist";

        if ( !m_artistFilter.isEmpty() ) {
            urlString = "http://ws.mp3tunes.com/api/v1/lockerSearch?output=xml&sid=<SESSION_ID>&partner_token=<PARTNER_TOKEN>&type=artist&s=" + m_artistFilter;
        }



        urlString.replace( "<SESSION_ID>", m_sessionId);
        urlString.replace( "<PARTNER_TOKEN>", "7359149936");

        debug() << "url: " << urlString;


        m_storedTransferJob =  KIO::storedGet(  KUrl( urlString ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
            , this, SLOT( artistDownloadComplete( KJob *) ) );
    //}
}

void Mp3tunesServiceQueryMaker::fetchAlbums()
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

        QString urlString = "http://ws.mp3tunes.com/api/v1/lockerData?sid=<SESSION_ID>&partner_token=<PARTNER_TOKEN>&output=xml&type=album& artist_id=<ARTIST_ID>";

        urlString.replace( "<SESSION_ID>", m_sessionId );
        urlString.replace( "<PARTNER_TOKEN>", "7359149936" );
        urlString.replace( "<ARTIST_ID>", m_parentArtistId );

        m_storedTransferJob =  KIO::storedGet(  KUrl( urlString ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
            , this, SLOT( albumDownloadComplete( KJob *) ) );
    }
}

void Mp3tunesServiceQueryMaker::fetchTracks()
{
    DEBUG_BLOCK

    TrackList tracks;

    //debug() << "parent id: " << m_parentId;

    if ( !m_parentAlbumId.isEmpty() ) {
        AlbumMatcher albumMatcher( m_collection->albumById( m_parentAlbumId.toInt() ) );
        tracks = albumMatcher.match( m_collection );
    } else
        return;

    if ( tracks.count() > 0 ) {
        handleResult( tracks );
    } else {

        QString urlString = "http://ws.mp3tunes.com/api/v1/lockerData?sid=<SESSION_ID>&partner_token=<PARTNER_TOKEN>&output=xml&type=track& album_id=<ALBUM_ID>";

        urlString.replace( "<SESSION_ID>", m_sessionId );
        urlString.replace( "<PARTNER_TOKEN>", "7359149936" );
        if(  !m_parentAlbumId.isEmpty() )
            urlString.replace( "<ALBUM_ID>", m_parentAlbumId );
        else 
            urlString.replace( "<ALBUM_ID>", m_parentArtistId );

        m_storedTransferJob =  KIO::storedGet(  KUrl( urlString ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
            , this, SLOT( trackDownloadComplete( KJob *) ) );
    }
}



void Mp3tunesServiceQueryMaker::artistDownloadComplete(KJob * job)
{
    DEBUG_BLOCK

    if( job->error() )
    {
        error() << job->error();
        m_storedTransferJob->deleteLater();
        return;
    }

    ArtistList artists;

    //debug() << "received artists: " <<  m_storedTransferJob->data();

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( m_storedTransferJob->data() );
    QDomElement root = doc.firstChildElement( "mp3tunes" );
    root = root.firstChildElement( "artistList" );
  

    QDomNode n = root.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        //if ( ! (e.tagName() == "item") )
        //    break;

        QDomElement element = n.firstChildElement("artistName");
        ServiceArtist * artist = new ServiceArtist( element.text() );

        //debug() << "Adding artist: " <<  element.text();

        element = n.firstChildElement("artistId");
        artist->setId( element.text().toInt() );

        ArtistPtr artistPtr( artist );

        artists.push_back( artistPtr );

        m_collection->acquireWriteLock();
        m_collection->addArtist( artistPtr );
        m_collection->releaseLock();

        n = n.nextSibling();
    }

   m_storedTransferJob->deleteLater();

   handleResult( artists );
   emit queryDone();

}

void Mp3tunesServiceQueryMaker::albumDownloadComplete(KJob * job)
{
    DEBUG_BLOCK

    if( job->error() )
    {
        error() << job->error();
        m_storedTransferJob->deleteLater();
        return;
    }

    //debug() << "Received response: " << m_storedTransferJob->data();

    AlbumList albums;

    //debug() << "received artists: " <<  m_storedTransferJob->data();

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( m_storedTransferJob->data() );
    QDomElement root = doc.firstChildElement("mp3tunes");
    root = root.firstChildElement( "albumList" );
  

    QDomNode n = root.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        //if ( ! (e.tagName() == "item") )
        //    break;

        QDomElement element = n.firstChildElement("albumTitle");

        QString title = element.text();
        if ( title.isEmpty() ) title = "Unknown";

        element = n.firstChildElement("albumId");
        QString albumIdStr = element.text();
        int albumId = element.text().toInt();

        element = n.firstChildElement("hasArt");
        int hasArt = element.text().toInt();

        Mp3TunesAlbum * album = new Mp3TunesAlbum( title );
        
        if ( hasArt > 0 )
        {
        
            QString coverUrl = "http://content.mp3tunes.com/storage/albumartget/<ALBUM_ID>?alternative=1&partner_token=<PARTNER_TOKEN>&sid=<SESSION_ID>";

            coverUrl.replace( "<SESSION_ID>", m_sessionId );
            coverUrl.replace( "<PARTNER_TOKEN>", "7359149936" );
            coverUrl.replace( "<ALBUM_ID>", albumIdStr );
        
            album->setCoverUrl(coverUrl);
        }

        AlbumPtr albumPtr( album );

        //debug() << "Adding album: " <<  title;

        album->setId( albumId );
        m_collection->acquireWriteLock();
        m_collection->addAlbum( albumPtr );
        m_collection->releaseLock();

        element = n.firstChildElement("artistId");

        ArtistPtr artistPtr = m_collection->artistById( element.text().toInt() );
        if ( artistPtr.data() != 0 )
        {
           //debug() << "Found parent artist";
           album->setAlbumArtist( artistPtr );
        }

        albums.push_back( albumPtr );

        n = n.nextSibling();
    }

   m_storedTransferJob->deleteLater();

   handleResult( albums );
   emit queryDone();

}

void Mp3tunesServiceQueryMaker::trackDownloadComplete(KJob * job)
{
    DEBUG_BLOCK

    if( job->error() )
    {
        error() << job->error();
        m_storedTransferJob->deleteLater();
        return;
    }

    //debug() << "Received response: " << m_storedTransferJob->data();

    TrackList tracks;

    //debug() << "received tracks: " <<  m_storedTransferJob->data();

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( m_storedTransferJob->data() );
    QDomElement root = doc.firstChildElement("mp3tunes");


    QDomElement albumDataElement = root.firstChildElement("albumData");
    QDomElement trackListElement = root.firstChildElement("trackList");




    QDomNode n = trackListElement.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        //if ( ! (e.tagName() == "item") )
        //    break;

        QDomElement element = n.firstChildElement("trackTitle");

        QString title = element.text();
        if ( title.isEmpty() ) title = "Unknown";

        Mp3TunesTrack * track = new Mp3TunesTrack( title  );
        TrackPtr trackPtr( track );

        //debug() << "Adding track: " <<  title;

        element = n.firstChildElement("trackId");
        track->setId( element.text().toInt() );

        element = n.firstChildElement("playURL");
        track->setUrl( element.text() );

        element = n.firstChildElement("trackLength");
        track->setLength( (int)( element.text().toFloat() / 1000 ) );

        element = n.firstChildElement("trackNumber");
        track->setTrackNumber( element.text().toInt() );

        m_collection->acquireWriteLock();
        m_collection->addTrack( trackPtr );
        m_collection->releaseLock();

        element = n.firstChildElement("albumId");
        QString albumId = element.text();
        element = n.firstChildElement("artistId");
        QString artistId = element.text();


        ArtistPtr artistPtr = m_collection->artistById( artistId.toInt() );
        if ( artistPtr.data() != 0 ) { 
           //debug() << "Found parent artist";
           ServiceArtist *artist = dynamic_cast< ServiceArtist * > ( artistPtr.data() );
           track->setArtist( artistPtr );
           artist->addTrack( trackPtr );
        }

        AlbumPtr albumPtr = m_collection->albumById( albumId.toInt() );
        if ( albumPtr.data() != 0 ) { 
           //debug() << "Found parent album";
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

QueryMaker * Mp3tunesServiceQueryMaker::addFilter(qint64 value, const QString & filter, bool matchBegin, bool matchEnd)
{
    DEBUG_BLOCK
            //debug() << "value: " << value;
    //for now, only accept artist filters
    if ( value == valArtist ) {
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


