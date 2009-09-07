/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "ScriptableServiceQueryMaker.h"

#include "MetaConstants.h"
#include "ScriptableServiceMeta.h"
#include "ScriptManager.h"

#include "collection/support/MemoryMatcher.h"

#include "Debug.h"



struct ScriptableServiceQueryMaker::Private {
    //don't change the order of items in this enum
    enum QueryType { TRACK=1, ALBUM=2, ARTIST=3, GENRE=4, NONE=5 };
    QueryType type;
    QueryType closestParent;
    int maxsize;
    bool returnDataPtrs;
    QString callbackString;
    int parentId;
    AlbumQueryMode albumMode;
    QString filter;
    QString lastFilter;
};

ScriptableServiceQueryMaker::ScriptableServiceQueryMaker( ScriptableServiceCollection * collection, QString name )
 : DynamicServiceQueryMaker()
 , d( new Private )

{
    m_collection = collection;
    m_name = name;

    connect( collection, SIGNAL( updateComplete() ), this, SLOT( slotScriptComplete() ) );

    reset();
}

ScriptableServiceQueryMaker::~ScriptableServiceQueryMaker()
{
    delete d;
}

QueryMaker * ScriptableServiceQueryMaker::reset()
{
    d->type = Private::NONE;
    d->closestParent = Private::NONE;
    d->maxsize = -1;
    d->returnDataPtrs = false;
    d->callbackString.clear();
    d->parentId = -1;
    d->albumMode = AllAlbums;
    d->filter.clear();
    d->lastFilter.clear();

    return this;
}

QueryMaker* ScriptableServiceQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
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


    if ( d->type == Private::GENRE ) {
        if ( ( m_collection->levels() == 4 ) && (  m_collection->lastFilter() != d->filter ) )
               m_collection->clear();
        fetchGenre();
    } else if ( d->type == Private::ARTIST ) {
        if ( ( m_collection->levels() == 3 ) && (  m_collection->lastFilter() != d->filter ) )
            m_collection->clear();
        fetchArtists();
    } else if ( d->type == Private::ALBUM ) {
        if ( ( m_collection->levels() == 2 ) && (  m_collection->lastFilter() != d->filter ) )
            m_collection->clear();
        fetchAlbums();
    } else if ( d->type == Private::TRACK ) {
        if ( ( m_collection->levels() == 1 ) && (  m_collection->lastFilter() != d->filter ) )
            m_collection->clear();
        fetchTracks();
    }

}

void ScriptableServiceQueryMaker::abortQuery()
{
}

QueryMaker * ScriptableServiceQueryMaker::setQueryType( QueryType type )
{
    DEBUG_BLOCK
    switch( type ) {
    case QueryMaker::Artist:
        d->type = Private::ARTIST;
        return this;

    case QueryMaker::Album:
        d->type = Private::ALBUM;
        return this;

    case QueryMaker::Track:
        d->type = Private::TRACK;
        return this;

    case QueryMaker::Genre:
        d->type = Private::GENRE;
        return this;

    case QueryMaker::Composer:
    case QueryMaker::Year:
    case QueryMaker::Custom:
    case QueryMaker::None:
        //TODO: Implement.
        return this;
    }

    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    if ( d->closestParent > Private::GENRE )
    {
        d->closestParent = Private::GENRE;
        const Meta::ScriptableServiceGenre * scriptableGenre = static_cast< const Meta::ScriptableServiceGenre * >( genre.data() );
        d->callbackString = scriptableGenre->callbackString();
        d->parentId = scriptableGenre->id();
    }
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addMatch( const Meta::ArtistPtr & artist )
{
    if ( d->closestParent > Private::ARTIST )
    {
        d->closestParent = Private::ARTIST;
        const Meta::ScriptableServiceArtist * scriptableArtist = static_cast< const Meta::ScriptableServiceArtist * >( artist.data() );
        d->callbackString = scriptableArtist->callbackString();
        d->parentId = scriptableArtist->id();
    }
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addMatch( const Meta::AlbumPtr & album )
{
    if ( d->closestParent > Private::ALBUM )
    {
        d->closestParent = Private::ALBUM;
        debug() << "Here!";
        const Meta::ScriptableServiceAlbum * scriptableAlbum = static_cast< const Meta::ScriptableServiceAlbum * >( album.data() );
        d->callbackString = scriptableAlbum->callbackString();
        d->parentId = scriptableAlbum->id();
    }
    return this;
}

template<class PointerType, class ListType>
void ScriptableServiceQueryMaker::emitProperResult( const ListType& list )
{
    if ( d->returnDataPtrs ) {
        DataList data;
        foreach( PointerType p, list )
            data << DataPtr::staticCast( p );

        emit newResultReady( m_collection->collectionId(), data );
    }
    else
        emit newResultReady( m_collection->collectionId(), list );
}

void ScriptableServiceQueryMaker::handleResult()
{
}


void ScriptableServiceQueryMaker::handleResult( const Meta::GenreList & genres )
{
    if ( d->maxsize >= 0 && genres.count() > d->maxsize )
        emitProperResult<GenrePtr, Meta::GenreList>( genres.mid( 0, d->maxsize ) );
    else
        emitProperResult<GenrePtr, Meta::GenreList>( genres );
}

void ScriptableServiceQueryMaker::handleResult( const Meta::AlbumList & albums )
{
    if ( d->maxsize >= 0 && albums.count() > d->maxsize )
        emitProperResult<AlbumPtr, Meta::AlbumList>( albums.mid( 0, d->maxsize ) );
    else
        emitProperResult<AlbumPtr, Meta::AlbumList>( albums );
}

void ScriptableServiceQueryMaker::handleResult( const Meta::ArtistList & artists )
{
    if ( d->maxsize >= 0 && artists.count() > d->maxsize )
        emitProperResult<ArtistPtr, Meta::ArtistList>( artists.mid( 0, d->maxsize ) );
    else
        emitProperResult<ArtistPtr, Meta::ArtistList>( artists );
}

void ScriptableServiceQueryMaker::handleResult( const Meta::TrackList & tracks )
{
    if ( d->maxsize >= 0 && tracks.count() > d->maxsize )
    {
        debug() << "Emitting " << tracks.count() << " tracks";
        emitProperResult<TrackPtr, Meta::TrackList>( tracks.mid( 0, d->maxsize ) );
    }
    else
    {
        debug() << "Emitting " << tracks.count() << " tracks";
        emitProperResult<TrackPtr, Meta::TrackList>( tracks );
    }
}




void ScriptableServiceQueryMaker::fetchGenre()
{
    DEBUG_BLOCK
    GenreList genre  = m_collection->genreMap().values();

    if ( genre.count() > 0 )
    {
        handleResult( genre );
        emit( queryDone() );
    }
    else
        //this is where we call the script to get it to add more stuff!
        ScriptManager::instance()->ServiceScriptPopulate( m_name, 3, d->parentId, d->callbackString, d->filter );
}

void ScriptableServiceQueryMaker::fetchArtists()
{
    DEBUG_BLOCK
    ArtistList artists;

    if ( d->parentId != -1 )
    {
        GenrePtr genrePtr =  m_collection->genreById( d->parentId );
        ScriptableServiceGenre * scGenre = dynamic_cast<ScriptableServiceGenre *> ( genrePtr.data() );
        if ( scGenre )
        {
            ArtistList allArtists = m_collection->artistMap().values();

            foreach ( ArtistPtr artistPtr, allArtists )
            {
                ScriptableServiceArtist *scArtist = dynamic_cast<ScriptableServiceArtist *> ( artistPtr.data() );
                if ( scArtist && scArtist->genreId() == d->parentId )
                    artists.append( artistPtr );
            }
        }
    }

    if ( artists.count() > 0 )
    {
        handleResult( artists );
        emit( queryDone() );
    }
    else
        //this is where we call the script to get it to add more stuff!
        ScriptManager::instance()->ServiceScriptPopulate( m_name, 2, d->parentId, d->callbackString, d->filter );
}

void ScriptableServiceQueryMaker::fetchAlbums()
{
    DEBUG_BLOCK
    debug() << "parent id: " << d->parentId;

    if ( d->albumMode == OnlyCompilations)
        return;

    AlbumList albums;

    if ( d->parentId != -1 )
    {
        ArtistMatcher artistMatcher( m_collection->artistById( d->parentId ) );
        albums = artistMatcher.matchAlbums( m_collection );
    }
    else
        albums = m_collection->albumMap().values();
    if ( albums.count() > 0 )
    {
        handleResult( albums );
        emit( queryDone() );
    }
    else
        //this is where we call the script to get it to add more stuff!
        ScriptManager::instance()->ServiceScriptPopulate( m_name, 1, d->parentId, d->callbackString, d->filter );
}

void ScriptableServiceQueryMaker::fetchTracks()
{
    DEBUG_BLOCK

    TrackList tracks;

    debug() << "parent id: " << d->parentId;

    AlbumPtr album;
    if ( d->parentId != -1 && ( album = m_collection->albumById( d->parentId ) ) )
    {
        AlbumMatcher albumMatcher( album );
        tracks = albumMatcher.match( m_collection );
    }
    else
        tracks = m_collection->trackMap().values();

    if ( tracks.count() > 0 ) {
        handleResult( tracks );
        emit( queryDone() );
    }
    else
        //this is where we call the script to get it to add more stuff!
    {
        debug() << "i am sending signals!";
        ScriptManager::instance()->ServiceScriptPopulate( m_name, 0, d->parentId, d->callbackString, d->filter );
    }
}

void ScriptableServiceQueryMaker::slotScriptComplete()
{
    DEBUG_BLOCK

    if ( d->type == Private::GENRE )
    {
        GenreList genre = m_collection->genreMap().values();
        handleResult( genre );
    }
    else if ( d->type == Private::ARTIST )
    {
        ArtistList artists;
        if ( d->parentId != -1 )
        {
            GenrePtr genrePtr =  m_collection->genreById( d->parentId );
            ScriptableServiceGenre * scGenre = dynamic_cast<ScriptableServiceGenre *> ( genrePtr.data() );
            if ( scGenre )
            {
                ArtistList allArtists = m_collection->artistMap().values();

                foreach ( ArtistPtr artistPtr, allArtists )
                {
                    ScriptableServiceArtist *scArtist = dynamic_cast<ScriptableServiceArtist *> ( artistPtr.data() );

                    if ( scArtist && scArtist->genreId() == d->parentId )
                        artists.append( artistPtr );
                }
            }
        }
        else
            artists = m_collection->artistMap().values();
        debug() << "there are " << artists.count() << " artists";
        handleResult( artists );
    }
    else if ( d->type == Private::ALBUM )
    {
       AlbumList albums;
       if ( d->parentId != -1 )
       {
           ArtistMatcher artistMatcher( m_collection->artistById( d->parentId ) );
            albums = artistMatcher.matchAlbums( m_collection );
       }
       else
            albums = m_collection->albumMap().values();

       debug() << "there are " << albums.count() << " albums";
       handleResult( albums );
    }
    else if ( d->type == Private::TRACK )
    {
        TrackList tracks;
        if ( d->parentId != -1 )
        {
            Meta::AlbumPtr album = m_collection->albumById( d->parentId );
            if( album )
            {
                AlbumMatcher albumMatcher( album );
                tracks = albumMatcher.match( m_collection );
            }
        }
        else
                tracks = m_collection->trackMap().values();
        debug() << "there are " << tracks.count() << " tracks";
        handleResult( tracks );
    }
    emit( queryDone() );
}

QueryMaker * ScriptableServiceQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    d->albumMode = mode;
    return this;
}

QueryMaker * ScriptableServiceQueryMaker::addFilter( qint64 value, const QString & filter, bool matchBegin, bool matchEnd )
{
    Q_UNUSED( matchBegin )
    Q_UNUSED( matchEnd )

    DEBUG_BLOCK

    if ( value == Meta::valTitle )
    {
        //I am sure there is a really good reason to add this space, as nothing works if it is removed, but WHY?!?
        d->filter += filter + ' ';
        d->filter = d->filter.replace( ' ', "%20" );
    }

    int level = 0;

    if (  d->type == Private::GENRE )
        level = 4;
    if (  d->type == Private::ARTIST )
        level = 3;
    else if (  d->type == Private::ALBUM )
        level = 2;
    else if (  d->type == Private::TRACK )
        level = 1;

    // should only clear all if we are querying for a top level item
    if ( m_collection->levels() == level )
    {
        //we need to clear everything as we have no idea what the scripts wants to do...
        //TODO: with KSharedPointers in use, does this leak!?

        debug() << "clear all!!!!!!!!!!!!!!";
        m_collection->clear();
    }

    d->lastFilter = d->filter;
    m_collection->setLastFilter( d->filter );
    return this;

}

QueryMaker * ScriptableServiceQueryMaker::addMatch( const Meta::DataPtr & data )
{
    //DEBUG_BLOCK
    ( const_cast<DataPtr&>(data) )->addMatchTo( this );
    return this;
}

#include "ScriptableServiceQueryMaker.moc"
