/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2007 Adam Pigg <adam@piggz.co.uk>                       *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "Mp3tunesServiceQueryMaker.h"

#include "amarok.h"
#include "debug.h"
#include "servicemetabase.h"
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
    m_parentId = QString();

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
    if ( d->type == Private::ALBUM ) {
        const ServiceArtist * serviceArtist = static_cast< const ServiceArtist * >( artist.data() );
        m_parentId = QString::number( serviceArtist->id() );
        debug() << "parent id set to: " << m_parentId;
    }

    return this;
}

QueryMaker * Mp3tunesServiceQueryMaker::addMatch(const Meta::AlbumPtr & album)
{
    DEBUG_BLOCK
    if ( d->type == Private::TRACK ) {
        const ServiceAlbum * serviceAlbum = static_cast< const ServiceAlbum * >( album.data() );
        m_parentId = QString::number( serviceAlbum->id() );
        debug() << "parent id set to: " << m_parentId;
    }

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
    if ( m_collection->artistMap().values().count() != 0 ) {
        handleResult( m_collection->artistMap().values() );
        debug() << "no need to fetch artists again! ";
    }
    else {

        QString urlString = "http://ws.mp3tunes.com/api/v0/lockerData?sid=<SESSION_ID>&partner_token=<PARTNER_TOKEN>&output=xml&type=artist";

        urlString.replace( "<SESSION_ID>", m_sessionId);
        urlString.replace( "<PARTNER_TOKEN>", "7359149936");


        m_storedTransferJob =  KIO::storedGet(  KUrl( urlString ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
            , this, SLOT( artistDownloadComplete( KJob *) ) );
    }
}

void Mp3tunesServiceQueryMaker::fetchAlbums()
{
    DEBUG_BLOCK

    AlbumList albums;


    debug() << "parent id: " << m_parentId;

    if ( !m_parentId.isEmpty() ) {
        ArtistMatcher artistMatcher( m_collection->artistMap()[ m_parentId ] );
        albums = artistMatcher.matchAlbums( m_collection );
    }

    if ( albums.count() > 0 ) {
        handleResult( albums );
    } else {

        QString urlString = "http://ws.mp3tunes.com/api/v0/lockerData?sid=<SESSION_ID>&partner_token=<PARTNER_TOKEN>&output=xml&type=album& artist_id=<ARTIST_ID>";

        urlString.replace( "<SESSION_ID>", m_sessionId );
        urlString.replace( "<PARTNER_TOKEN>", "7359149936" );
        urlString.replace( "<ARTIST_ID>", m_parentId );

        m_storedTransferJob =  KIO::storedGet(  KUrl( urlString ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
            , this, SLOT( albumDownloadComplete( KJob *) ) );
    }
}

void Mp3tunesServiceQueryMaker::fetchTracks()
{
    DEBUG_BLOCK

    TrackList tracks;

    debug() << "parent id: " << m_parentId;

    if ( !m_parentId.isEmpty() ) {
        AlbumMatcher albumMatcher( m_collection->albumMap()[ m_parentId ] );
        tracks = albumMatcher.match( m_collection );
    }

    if ( tracks.count() > 0 ) {
        handleResult( tracks );
    } else {

        QString urlString = "http://ws.mp3tunes.com/api/v0/lockerData?sid=<SESSION_ID>&partner_token=<PARTNER_TOKEN>&output=xml&type=track& album_id=<ALBUM_ID>";

        urlString.replace( "<SESSION_ID>", m_sessionId );
        urlString.replace( "<PARTNER_TOKEN>", "7359149936" );
        urlString.replace( "<ALBUM_ID>", m_parentId );

        m_storedTransferJob =  KIO::storedGet(  KUrl( urlString ), KIO::NoReload, KIO::HideProgressInfo );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
            , this, SLOT( trackDownloadComplete( KJob *) ) );
    }
}



void Mp3tunesServiceQueryMaker::artistDownloadComplete(KJob * job)
{

    DEBUG_BLOCK


    ArtistList artists;

    //debug() << "recieved artists: " <<  m_storedTransferJob->data();

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( m_storedTransferJob->data() );
    QDomElement root = doc.firstChildElement("mp3tunes");
  

    QDomNode n = root.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        //if ( ! (e.tagName() == "item") )
        //    break;

        QDomElement element = n.firstChildElement("artistName");
        ServiceArtist * artist = new ServiceArtist( element.text() );

        debug() << "Adding artist: " <<  element.text();

        element = n.firstChildElement("artistId");
        artist->setId( element.text().toInt() );

        ArtistPtr artistPtr( artist );

        artists.push_back( artistPtr );

        m_collection->addArtist( element.text(),  artistPtr );

        n = n.nextSibling();
    }

   m_storedTransferJob->deleteLater();

   handleResult( artists );
   emit queryDone();

}

void Mp3tunesServiceQueryMaker::albumDownloadComplete(KJob * job)
{
    DEBUG_BLOCK

    debug() << "Recieved response: " << m_storedTransferJob->data();

    AlbumList albums;

    //debug() << "recieved artists: " <<  m_storedTransferJob->data();

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( m_storedTransferJob->data() );
    QDomElement root = doc.firstChildElement("mp3tunes");
  

    QDomNode n = root.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        //if ( ! (e.tagName() == "item") )
        //    break;

        QDomElement element = n.firstChildElement("albumTitle");

        QString title = element.text();
        if ( title.isEmpty() ) title = "Unknown";

        ServiceAlbum * album = new ServiceAlbum( title  );
        AlbumPtr albumPtr( album );

        debug() << "Adding album: " <<  title;

        element = n.firstChildElement("albumId");
        album->setId( element.text().toInt() );

        m_collection->addAlbum( element.text(),  albumPtr );


        element = n.firstChildElement("artistId");

        ArtistPtr artistPtr = m_collection->artistMap() [ element.text() ];

        if ( artistPtr.data() != 0 ) { 
           debug() << "Found parent artist";
           ServiceArtist *artist = dynamic_cast< ServiceArtist * > ( artistPtr.data() );
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

    debug() << "Recieved response: " << m_storedTransferJob->data();

    TrackList tracks;

    debug() << "recieved tracks: " <<  m_storedTransferJob->data();

     //so lets figure out what we got here:
    QDomDocument doc( "reply" );
    doc.setContent( m_storedTransferJob->data() );
    QDomElement root = doc.firstChildElement("mp3tunes");


    QDomElement albumDataElement = root.firstChildElement("albumData");
    QDomElement trackListElement = root.firstChildElement("trackList");


    QDomElement e = albumDataElement.firstChildElement("albumId");
    QString albumId = e.text();
    e = albumDataElement.firstChildElement("artistId");
    QString artistId = e.text();


    QDomNode n = trackListElement.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        //if ( ! (e.tagName() == "item") )
        //    break;

        QDomElement element = n.firstChildElement("trackTitle");

        QString title = element.text();
        if ( title.isEmpty() ) title = "Unknown";

        ServiceTrack * track = new ServiceTrack( title  );
        TrackPtr trackPtr( track );

        debug() << "Adding track: " <<  title;

        element = n.firstChildElement("trackId");
        track->setId( element.text().toInt() );

        m_collection->addTrack( element.text(),  trackPtr );


        element = n.firstChildElement("downloadURL");
        track->setUrl( element.text() );

        element = n.firstChildElement("trackLength");
        track->setLength( element.text().toFloat() / 1000 );

        element = n.firstChildElement("trackNumber");
        track->setTrackNumber( element.text().toInt() );


        ArtistPtr artistPtr = m_collection->artistMap() [ artistId ];
        if ( artistPtr.data() != 0 ) { 
           debug() << "Found parent artist";
           ServiceArtist *artist = dynamic_cast< ServiceArtist * > ( artistPtr.data() );
           track->setArtist( artistPtr );
           artist->addTrack( trackPtr );
        }

        AlbumPtr albumPtr = m_collection->albumMap() [ albumId ];
        if ( albumPtr.data() != 0 ) { 
           debug() << "Found parent album";
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













#include "Mp3tunesServiceQueryMaker.moc"






