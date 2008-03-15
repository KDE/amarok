/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#include "DynamicScriptableQueryMaker.h"
#include "DynamicScriptableServiceMeta.h"

#include "collection/support/MemoryMatcher.h"
#include "AmarokProcess.h"

#include "debug.h"

#include <KRun>
#include <KShell>


struct DynamicScriptableQueryMaker::Private {
    //dont change the order of items in this enum
    enum QueryType { TRACK=1, ALBUM=2, ARTIST=3, GENRE=4, NONE=5 };
    QueryType type;
    QueryType closestParent;
    int maxsize;
    bool returnDataPtrs;
    QString callbackString;
    int parentId;
    AmarokProcess * scriptProcess;
};


DynamicScriptableQueryMaker::DynamicScriptableQueryMaker( DynamicScriptableServiceCollection * collection, QString script )
 : DynamicServiceQueryMaker()
 , d( new Private )

{
    DEBUG_BLOCK
    m_collection = collection;
    m_script = script;
    reset();
}


DynamicScriptableQueryMaker::~DynamicScriptableQueryMaker()
{
    delete d->scriptProcess;
    delete d;
}

QueryMaker * DynamicScriptableQueryMaker::reset()
{
    d->type = Private::NONE;
    d->closestParent = Private::NONE;
    d->maxsize = -1;
    d->returnDataPtrs = false;
    d->callbackString = QString();
    d->parentId = -1;
    d->scriptProcess = 0;

    return this;
}

QueryMaker*
DynamicScriptableQueryMaker::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->returnDataPtrs = resultAsDataPtrs;
    return this;
}

void DynamicScriptableQueryMaker::run()
{
    DEBUG_BLOCK

    if ( d->type == Private::NONE )
        //TODO error handling
        return;
    if (  d->type == Private::ARTIST )       
        fetchArtists();
    else if (  d->type == Private::ALBUM )       
        fetchAlbums();
    else if (  d->type == Private::TRACK )       
        fetchTracks();

}


void DynamicScriptableQueryMaker::abortQuery()
{
}

QueryMaker * DynamicScriptableQueryMaker::startGenreQuery()
{
    DEBUG_BLOCK
    d->type = Private::GENRE;
    return this;
}

QueryMaker * DynamicScriptableQueryMaker::startArtistQuery()
{
    DEBUG_BLOCK
    d->type = Private::ARTIST;
    return this;
}

QueryMaker * DynamicScriptableQueryMaker::startAlbumQuery()
{
    DEBUG_BLOCK
    d->type = Private::ALBUM;
    return this;
}

QueryMaker * DynamicScriptableQueryMaker::startTrackQuery()
{
    DEBUG_BLOCK
    d->type = Private::TRACK;
    return this;
}


QueryMaker * DynamicScriptableQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    DEBUG_BLOCK
    if ( d->closestParent > Private::GENRE ) { 
        const Meta::DynamicScriptableGenre * dynamicGenre = static_cast< const Meta::DynamicScriptableGenre * >( genre.data() );
        d->callbackString = dynamicGenre->callbackString();
        d->parentId = dynamicGenre->id();
    }
    return this;
}

QueryMaker * DynamicScriptableQueryMaker::addMatch( const Meta::ArtistPtr & artist )
{
    DEBUG_BLOCK
    if ( d->closestParent > Private::ARTIST ) { 
        const Meta::DynamicScriptableArtist * dynamicArtist = static_cast< const Meta::DynamicScriptableArtist * >( artist.data() );
        d->callbackString = dynamicArtist->callbackString();
        d->parentId = dynamicArtist->id();
    }
    return this;
}

QueryMaker * DynamicScriptableQueryMaker::addMatch(const Meta::AlbumPtr & album)
{
    DEBUG_BLOCK
    if ( d->closestParent > Private::ALBUM ) {
        debug() << "Here!";
        const Meta::DynamicScriptableAlbum * dynamicAlbum = static_cast< const Meta::DynamicScriptableAlbum * >( album.data() );
        d->callbackString = dynamicAlbum->callbackString();
        d->parentId = dynamicAlbum->id();
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


void DynamicScriptableQueryMaker::handleResult()
{
    DEBUG_BLOCK
}


void DynamicScriptableQueryMaker::handleResult(const Meta::GenreList & genres)
{
    DEBUG_BLOCK
    Q_UNUSED( genres );
}

void DynamicScriptableQueryMaker::handleResult(const Meta::AlbumList & albums)
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && albums.count() > d->maxsize ) {
        emitProperResult( AlbumPtr, albums.mid( 0, d->maxsize ) );
    } else 
        emitProperResult( AlbumPtr, albums );

}

void DynamicScriptableQueryMaker::handleResult(const Meta::ArtistList & artists)
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && artists.count() > d->maxsize ) {
        emitProperResult( ArtistPtr, artists.mid( 0, d->maxsize ) );
    } else 
        emitProperResult( ArtistPtr, artists );
}

void DynamicScriptableQueryMaker::handleResult(const Meta::TrackList & tracks)
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && tracks.count() > d->maxsize ) {
        debug() << "Emitting " << tracks.count() << " tracks";
        emitProperResult( TrackPtr, tracks.mid( 0, d->maxsize ) );
    } else {
        debug() << "Emitting " << tracks.count() << " tracks";
        emitProperResult( TrackPtr, tracks );
    }
        
}




void DynamicScriptableQueryMaker::fetchGenres()
{

}

void DynamicScriptableQueryMaker::fetchArtists()
{
     if ( m_collection->artistMap().values().count() != 0 ) {
        handleResult( m_collection->artistMap().values() );
    }
}

void DynamicScriptableQueryMaker::fetchAlbums()
{

    DEBUG_BLOCK

    AlbumList albums;

    if ( d->parentId != -1 ) {
       ArtistMatcher artistMatcher( m_collection->artistById( m_parentArtistId ) );
       albums = artistMatcher.matchAlbums( m_collection );
    } //else 
    //    return;

    if ( albums.count() > 0 ) {
        handleResult( albums );
    } else {
        //this is where we call the script to get it to add more stuff!
        debug() << "running: " <<  m_script + " --populate -1";

        QStringList args;
        args << "--populate ";
        args << "-1";

        d->scriptProcess = new AmarokProcess();
        d->scriptProcess->setProgram( m_script, args );
        connect ( d->scriptProcess, SIGNAL( finished ( int ) ), this, SLOT( slotScriptComplete() ) );
        d->scriptProcess->start();
    }

}

void DynamicScriptableQueryMaker::fetchTracks()
{
    DEBUG_BLOCK

    TrackList tracks;

    debug() << "parent id: " << d->parentId;

    if ( d->parentId != -1 ) {
        AlbumMatcher albumMatcher( m_collection->albumById( d->parentId ) );
        tracks = albumMatcher.match( m_collection );
    } else
        return;

    if ( tracks.count() > 0 ) {
        handleResult( tracks );
    } else {
        //this is where we call the script to get it to add more stuff!
        debug() << "running: " <<  m_script + " --populate " + QString::number( d->parentId );

        QStringList args;
        args << "--populate";
        args << QString::number( d->parentId );
        args << d->callbackString;

        d->scriptProcess = new AmarokProcess();
        d->scriptProcess->setProgram( m_script, args );
        connect ( d->scriptProcess, SIGNAL( finished ( int ) ), this, SLOT( slotScriptComplete() ) );
        d->scriptProcess->start();
        
    }

}

void DynamicScriptableQueryMaker::slotScriptComplete()
{
     DEBUG_BLOCK

     
     if ( d->type == Private::ALBUM ) {

       AlbumList albums;
     
       if ( d->parentId != -1 ) {
           ArtistMatcher artistMatcher( m_collection->artistById( d->parentId ) );
            albums = artistMatcher.matchAlbums( m_collection );
        } else
            albums = m_collection->albumMap().values();

        debug() << "there are " << albums.count() << " albums";

         handleResult( albums );

        
     } else if ( d->type == Private::TRACK ) {

         TrackList tracks;
         
         if ( d->parentId != -1 ) {
             AlbumMatcher albumMatcher( m_collection->albumById( d->parentId ) );
             tracks = albumMatcher.match( m_collection );
         } 

         debug() << "there are " << tracks.count() << " tracks";
         handleResult( tracks );

     }

     emit( queryDone() );
}






#include "DynamicScriptableQueryMaker.moc"









