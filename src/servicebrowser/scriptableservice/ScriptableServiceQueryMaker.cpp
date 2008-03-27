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

#include "ScriptableServiceQueryMaker.h"
#include "ScriptableServiceMeta.h"

#include "collection/support/MemoryMatcher.h"
#include "AmarokProcess.h"

#include "debug.h"

#include <KRun>
#include <KShell>


struct ScriptableServiceQueryMaker::Private {
    //don't change the order of items in this enum
    enum QueryType { TRACK=1, ALBUM=2, ARTIST=3, GENRE=4, NONE=5 };
    QueryType type;
    QueryType closestParent;
    int maxsize;
    bool returnDataPtrs;
    QString callbackString;
    int parentId;
    AmarokProcess * scriptProcess;
    AlbumQueryMode albumMode;
    QString filter;
};


ScriptableServiceQueryMaker::ScriptableServiceQueryMaker( ScriptableServiceCollection * collection, AmarokProcIO * script )
 : DynamicServiceQueryMaker()
 , d( new Private )

{
    DEBUG_BLOCK
    m_collection = collection;
    m_script = script;

    connect( collection, SIGNAL( updateComplete() ), this, SLOT( slotScriptComplete() ) );
    
    reset();
}


ScriptableServiceQueryMaker::~ScriptableServiceQueryMaker()
{
    DEBUG_BLOCK
    delete d->scriptProcess;
    delete d;
}

QueryMaker * ScriptableServiceQueryMaker::reset()
{
    DEBUG_BLOCK
    d->type = Private::NONE;
    d->closestParent = Private::NONE;
    d->maxsize = -1;
    d->returnDataPtrs = false;
    d->callbackString = QString();
    d->parentId = -1;
    d->scriptProcess = 0;
    d->filter = QString();

    return this;
}

QueryMaker*
        ScriptableServiceQueryMaker::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->returnDataPtrs = resultAsDataPtrs;
    return this;
}

void ScriptableServiceQueryMaker::run()
{
    DEBUG_BLOCK

    if ( d->albumMode == OnlyCompilations )
        return;

    if ( d->type == Private::NONE )
        //TODO error handling
        return;

    if ( d->callbackString.isEmpty() )
        d->callbackString = "none";

    if (  d->type == Private::GENRE )
        fetchGenre();
    if (  d->type == Private::ARTIST )       
        fetchArtists();
    else if (  d->type == Private::ALBUM )       
        fetchAlbums();
    else if (  d->type == Private::TRACK )       
        fetchTracks();

}


void ScriptableServiceQueryMaker::abortQuery()
{
}

QueryMaker * ScriptableServiceQueryMaker::startGenreQuery()
{
    DEBUG_BLOCK
    d->type = Private::GENRE;
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::startArtistQuery()
{
    DEBUG_BLOCK
    d->type = Private::ARTIST;
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::startAlbumQuery()
{
    DEBUG_BLOCK
    d->type = Private::ALBUM;
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::startTrackQuery()
{
    DEBUG_BLOCK
    d->type = Private::TRACK;
    return this;
}


QueryMaker * ScriptableServiceQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    DEBUG_BLOCK
    if ( d->closestParent > Private::GENRE ) {

        d->closestParent = Private::GENRE;
        const Meta::ScriptableServiceGenre * scriptableGenre = static_cast< const Meta::ScriptableServiceGenre * >( genre.data() );
        d->callbackString = scriptableGenre->callbackString();
        d->parentId = scriptableGenre->id();
    }
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addMatch( const Meta::ArtistPtr & artist )
{
    DEBUG_BLOCK
    if ( d->closestParent > Private::ARTIST ) {
        d->closestParent = Private::ARTIST;
        const Meta::ScriptableServiceArtist * scriptableArtist = static_cast< const Meta::ScriptableServiceArtist * >( artist.data() );
        d->callbackString = scriptableArtist->callbackString();
        d->parentId = scriptableArtist->id();
    }
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addMatch( const Meta::AlbumPtr & album )
{
    DEBUG_BLOCK
    if ( d->closestParent > Private::ALBUM ) {
        d->closestParent = Private::ALBUM;
        debug() << "Here!";
        const Meta::ScriptableServiceAlbum * scriptableAlbum = static_cast< const Meta::ScriptableServiceAlbum * >( album.data() );
        d->callbackString = scriptableAlbum->callbackString();
        d->parentId = scriptableAlbum->id();
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


void ScriptableServiceQueryMaker::handleResult()
{
    DEBUG_BLOCK
}


void ScriptableServiceQueryMaker::handleResult(const Meta::GenreList & genres)
{
    DEBUG_BLOCK
            
    if ( d->maxsize >= 0 && genres.count() > d->maxsize ) {
        emitProperResult( GenrePtr, genres.mid( 0, d->maxsize ) );
    } else
        emitProperResult( GenrePtr, genres );
}

void ScriptableServiceQueryMaker::handleResult(const Meta::AlbumList & albums)
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && albums.count() > d->maxsize ) {
        emitProperResult( AlbumPtr, albums.mid( 0, d->maxsize ) );
    } else 
        emitProperResult( AlbumPtr, albums );

}

void ScriptableServiceQueryMaker::handleResult(const Meta::ArtistList & artists)
{
    DEBUG_BLOCK

    if ( d->maxsize >= 0 && artists.count() > d->maxsize ) {
        emitProperResult( ArtistPtr, artists.mid( 0, d->maxsize ) );
    } else 
        emitProperResult( ArtistPtr, artists );
}

void ScriptableServiceQueryMaker::handleResult(const Meta::TrackList & tracks)
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




void ScriptableServiceQueryMaker::fetchGenre()
{

    DEBUG_BLOCK
    GenreList genre  = m_collection->genreMap().values();
    
    if ( genre.count() > 0 ) {
        handleResult( genre );
        emit( queryDone() );
    } else {
        //this is where we call the script to get it to add more stuff!

        QString args;
        args += "populate ";
        args += "3 ";
        args += QString::number( d->parentId );
        args += " ";
        args += d->callbackString;
        if ( !d->filter.isEmpty() ) {
            args += " ";
            args += d->filter;
        }
        debug() << "sending: "  << args;
        m_script->writeStdin( args );

    }

}

void ScriptableServiceQueryMaker::fetchArtists()
{
    DEBUG_BLOCK
    ArtistList artists;

    if ( d->parentId != -1 ) {
        GenrePtr genrePtr =  m_collection->genreById( d->parentId );
        ScriptableServiceGenre * scGenre = dynamic_cast<ScriptableServiceGenre *> ( genrePtr.data() );

        if ( scGenre ) {
            ArtistList allArtists = m_collection->artistMap().values();

            foreach ( ArtistPtr artistPtr, allArtists ) {
                ScriptableServiceArtist *scArtist = dynamic_cast<ScriptableServiceArtist *> ( artistPtr.data() );

                if ( scArtist && scArtist->genreId() == d->parentId )
                    artists.append( artistPtr );
            }

        }

    }
    
    if ( artists.count() > 0 ) {
        handleResult( artists );
        emit( queryDone() );
    } else {
        //this is where we call the script to get it to add more stuff!

        QString args;
        args += "populate ";
        args += "2 ";
        args += QString::number( d->parentId );
        args += " ";
        args += d->callbackString;
        if ( !d->filter.isEmpty() ) {
            args += " ";
            args += d->filter;
        }
        debug() << "sending: "  << args;
        m_script->writeStdin( args );

    }
}

void ScriptableServiceQueryMaker::fetchAlbums()
{
    DEBUG_BLOCK

    AlbumList albums;

    if ( d->parentId != -1 ) {
        ArtistMatcher artistMatcher( m_collection->artistById( d->parentId ) );
       albums = artistMatcher.matchAlbums( m_collection );
    } else {
        albums = m_collection->albumMap().values();
    }
    
    if ( albums.count() > 0 ) {
        handleResult( albums );
        emit( queryDone() );
    } else {
        //this is where we call the script to get it to add more stuff!


        QString args;
        args += "populate ";
        args += "1 ";
        args += QString::number( d->parentId );
        args += " ";
        args += d->callbackString;

        if ( !d->filter.isEmpty() ) {
            args += " ";
            args += d->filter;
        }
        
        debug() << "sending: "  << args;
        m_script->writeStdin( args );
        
    }

}

void ScriptableServiceQueryMaker::fetchTracks()
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
        emit( queryDone() );
    } else {
        //this is where we call the script to get it to add more stuff!

        QString args;
        args += "populate ";
        args += "0 ";
        args += QString::number( d->parentId );
        args += " ";
        args += d->callbackString;
        if ( !d->filter.isEmpty() ) {
            args += " ";
            args += d->filter;
        }
        debug() << "sending: "  << args;
        m_script->writeStdin( args );
        
    }

}

void ScriptableServiceQueryMaker::slotScriptComplete()
{
    DEBUG_BLOCK

    if ( d->type == Private::GENRE ) {

        GenreList genre = m_collection->genreMap().values();
        handleResult( genre );

    } else if ( d->type == Private::ARTIST ) {

        ArtistList artists;

        if ( d->parentId != -1 ) {
            GenrePtr genrePtr =  m_collection->genreById( d->parentId );
            ScriptableServiceGenre * scGenre = dynamic_cast<ScriptableServiceGenre *> ( genrePtr.data() );

            if ( scGenre ) {
                ArtistList allArtists = m_collection->artistMap().values();

                foreach ( ArtistPtr artistPtr, allArtists ) {
                    ScriptableServiceArtist *scArtist = dynamic_cast<ScriptableServiceArtist *> ( artistPtr.data() );

                    if ( scArtist && scArtist->genreId() == d->parentId )
                        artists.append( artistPtr );
                }
            }
        } else
            artists = m_collection->artistMap().values();

        debug() << "there are " << artists.count() << " artists";
        handleResult( artists );

    } else if ( d->type == Private::ALBUM ) {

       AlbumList albums;
     
       if ( d->parentId != -1 ) {
           ArtistMatcher artistMatcher( m_collection->artistById( d->parentId ) );
            albums = artistMatcher.matchAlbums( m_collection );
        } else
            albums = m_collection->albumMap().values();

        debug() << "there are " << albums.count() << " albums";
        handleResult( albums );

        
     } else if ( d->type == Private::ALBUM ) {

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

QueryMaker * ScriptableServiceQueryMaker::setAlbumQueryMode(AlbumQueryMode mode)
{
    d->albumMode = mode;
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addFilter(qint64 value, const QString & filter, bool matchBegin, bool matchEnd)
{
    DEBUG_BLOCK

    if ( value == valTitle ) {
        d->filter += filter + " ";
        d->filter = d->filter.replace( " ", "%20" );
    }

    //we need to clear everything as we have no idea what the scripts wants to do...
    //TODO: with KSharedPointers in use, does this leak!?


    m_collection->acquireWriteLock();
            
    m_collection->genreMap().clear();
    m_collection->setGenreMap( GenreMap() );
    
    m_collection->artistMap().clear();
    m_collection->setArtistMap( ArtistMap() );
    
    m_collection->albumMap().clear();
    m_collection->setAlbumMap( AlbumMap() );
    
    m_collection->trackMap().clear();
    m_collection->setTrackMap( TrackMap() );
    
    m_collection->releaseLock();
}



#include "ScriptableServiceQueryMaker.moc"









