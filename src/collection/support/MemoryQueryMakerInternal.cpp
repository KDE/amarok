/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "MemoryQueryMakerInternal.h"

#include "MemoryCollection.h"
#include "MemoryCustomValue.h"
#include "MemoryFilter.h"
#include "MemoryMatcher.h"
#include "MemoryQueryMakerHelper.h"

#include "meta/Meta.h"

#include <KRandomSequence>
#include <KSortableList>

MemoryQueryMakerInternal::MemoryQueryMakerInternal( MemoryCollection *collection )
    : QObject()
    , m_collection( collection )
    , m_filters( 0 )
    , m_matchers( 0 )
    , m_randomize( false )
    , m_maxSize( 0 )
    , m_returnAsDataPtrs( false )
    , m_type( QueryMaker::None )
    , m_albumQueryMode( QueryMaker::AllAlbums )
    , m_orderDescending( false )
    , m_orderByNumberField( false )
    , m_orderByField( 0 )
{

}

MemoryQueryMakerInternal::~MemoryQueryMakerInternal()
{
    delete m_filters;
    delete m_matchers;
    qDeleteAll( m_returnFunctions );
    qDeleteAll( m_returnValues );
}

void
MemoryQueryMakerInternal::runQuery()
{
    m_collection->acquireReadLock();
    //naive implementation, fix this
    if ( m_matchers )
    {
        Meta::TrackList result = m_matchers->match( m_collection );
        if ( m_filters )
        {
            Meta::TrackList filtered;
            foreach( Meta::TrackPtr track, result )
            {
                if( m_filters->filterMatches( track ) )
                    filtered.append( track );
            }
            handleResult( filtered );
        }
        else
            handleResult( result );
    }
    else if ( m_filters )
    {
        Meta::TrackList tracks = m_collection->trackMap().values();
        Meta::TrackList filtered;
        foreach( const Meta::TrackPtr &track, tracks )
        {
            if ( m_filters->filterMatches( track ) )
                filtered.append( track );
        }
        handleResult( filtered );
    }
    else
        handleResult();
    m_collection->releaseLock();
}

template <class PointerType>
void MemoryQueryMakerInternal::emitProperResult( const QList<PointerType>& list )
{
    QList<PointerType> resultList = list;
    if( m_randomize )
    {
        KRandomSequence sequence;
        sequence.randomize<PointerType>( resultList );
    }

    if ( m_maxSize >= 0 && resultList.count() > m_maxSize )
        resultList = resultList.mid( 0, m_maxSize );

    if( m_returnAsDataPtrs )
    {
        Meta::DataList data;
        foreach( PointerType p, resultList )
            data << Meta::DataPtr::staticCast( p );

        emit newResultReady( m_collection->collectionId(), data );
    }
    else
        emit newResultReady( m_collection->collectionId(), list );
}

template<typename T>
static inline QList<T> reverse(const QList<T> &l)
{
    QList<T> ret;
    for (int i=l.size() - 1; i>=0; --i)
        ret.append(l.at(i));
    return ret;
}

void
MemoryQueryMakerInternal::handleResult()
{
    //this gets called when we want to return all values for the given query type
    switch( m_type )
    {
        case QueryMaker::Custom :
        {
            QStringList result;
            Meta::TrackList tmpTracks = m_collection->trackMap().values();
            Meta::TrackList tracks;
            foreach( const Meta::TrackPtr &track, tmpTracks )
            {
                if( m_albumQueryMode == QueryMaker::AllAlbums
                    || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                    || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) )
                {
                    tracks.append( track );
                }
            }

            if( !m_returnFunctions.empty() )
            {
                //no sorting necessary
                foreach( CustomReturnFunction *function, m_returnFunctions )
                {
                    result.append( function->value( tracks ) );
                }
            }
            else if( !m_returnValues.empty() )
            {
                if( m_orderByField )
                {
                    if( m_orderByNumberField )
                        tracks = MemoryQueryMakerHelper::orderListByNumber( tracks, m_orderByField, m_orderDescending );
                    else
                        tracks = MemoryQueryMakerHelper::orderListByString( tracks, m_orderByField, m_orderDescending );
                }
                if( m_randomize )
                {
                    KRandomSequence sequence;
                    sequence.randomize<Meta::TrackPtr>( tracks );
                }

                int count = 0;
                foreach( const Meta::TrackPtr &track, tracks )
                {
                    if ( m_maxSize >= 0 && count == m_maxSize )
                        break;

                    foreach( CustomReturnValue *value, m_returnValues )
                    {
                        result.append( value->value( track ) );
                    }
                    count++;
                }
            }
            emit newResultReady( m_collection->collectionId(), result );
            break;
        }
        case QueryMaker::Track :
        {
            Meta::TrackList tracks;

            foreach( Meta::TrackPtr track, m_collection->trackMap().values() )
            {
                if( m_albumQueryMode == QueryMaker::AllAlbums
                    || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                    || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) )
                {
                    tracks.append( track );
                }
            }

            if( m_orderByField )
            {
                if( m_orderByNumberField )
                    tracks = MemoryQueryMakerHelper::orderListByNumber( tracks, m_orderByField, m_orderDescending );
                else
                    tracks = MemoryQueryMakerHelper::orderListByString( tracks, m_orderByField, m_orderDescending );
            }

            emitProperResult<Meta::TrackPtr>( tracks );
            break;
        }
        case QueryMaker::Album :
        {

            Meta::AlbumList albums;
            foreach( Meta::AlbumPtr album, m_collection->albumMap().values() )
            {
                if( m_albumQueryMode == QueryMaker::AllAlbums
                    || ( m_albumQueryMode == QueryMaker::OnlyCompilations && album->isCompilation() )
                    || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !album->isCompilation()) )
                {
                    albums.append( album );
                    break;
                }
            }

            albums = MemoryQueryMakerHelper::orderListByName<Meta::AlbumPtr>( albums, m_orderDescending );

            emitProperResult<Meta::AlbumPtr>( albums );
            break;
        }
        case QueryMaker::Artist :
        {
            Meta::ArtistList artists;
            foreach( Meta::ArtistPtr artist, m_collection->artistMap().values() )
            {
                Meta::TrackList tracks = artist->tracks();
                foreach( Meta::TrackPtr track, tracks )
                {
                    if( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) )
                    {
                        artists.append( artist );
                        break;
                    }
                }
            }
            artists = MemoryQueryMakerHelper::orderListByName<Meta::ArtistPtr>( artists, m_orderDescending );
            emitProperResult<Meta::ArtistPtr>( artists );
            break;
        }
        case QueryMaker::Composer :
        {
            Meta::ComposerList composers;
            foreach( Meta::ComposerPtr composer, m_collection->composerMap().values() )
            {
                Meta::TrackList tracks = composer->tracks();
                foreach( Meta::TrackPtr track, tracks )
                {
                    if( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) )
                    {
                        composers.append( composer );
                        break;
                    }
                }
            }
            composers = MemoryQueryMakerHelper::orderListByName<Meta::ComposerPtr>( composers, m_orderDescending );

            emitProperResult<Meta::ComposerPtr>( composers );
            break;
        }
        case QueryMaker::Genre :
        {
            Meta::GenreList genres;
            foreach( Meta::GenrePtr genre, m_collection->genreMap().values() )
            {
                Meta::TrackList tracks = genre->tracks();
                foreach( Meta::TrackPtr track, tracks )
                {
                    if( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) )
                    {
                        genres.append( genre );
                        break;
                    }
                }
            }

            genres = MemoryQueryMakerHelper::orderListByName<Meta::GenrePtr>( genres, m_orderDescending );

            emitProperResult<Meta::GenrePtr>( genres );
            break;
        }
        case QueryMaker::Year :
        {
        Meta::YearList years;
            foreach( Meta::YearPtr year, m_collection->yearMap().values() )
            {
                Meta::TrackList tracks = year->tracks();
                foreach( Meta::TrackPtr track, tracks )
                {
                    if( m_albumQueryMode == QueryMaker::AllAlbums
                        || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
                        || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) )
                    {
                        years.append( year );
                        break;
                    }
                }
            }

            //this a special case which requires a bit of code duplication
            //years have to be ordered as numbers, but orderListByNumber does not work for Meta::YearPtrs
            if( m_orderByField == Meta::valYear )
            {
                years = MemoryQueryMakerHelper::orderListByYear( years, m_orderDescending );
            }

            emitProperResult<Meta::YearPtr>( years );
            break;
        }
        case QueryMaker::None :
            //nothing to do
            break;
    }
}

void
MemoryQueryMakerInternal::handleResult( const Meta::TrackList &tmpTracks )
{
    Meta::TrackList tracks;
    foreach( const Meta::TrackPtr &track, tmpTracks )
    {
        if( m_albumQueryMode == QueryMaker::AllAlbums
            || ( m_albumQueryMode == QueryMaker::OnlyCompilations && track->album()->isCompilation() )
            || ( m_albumQueryMode == QueryMaker::OnlyNormalAlbums && !track->album()->isCompilation()) )
        {
            tracks.append( track );
        }
    }

    switch( m_type )
    {
        case QueryMaker::Custom :
        {
            QStringList result;
            if( !m_returnFunctions.empty() )
            {
                //no sorting necessary
                foreach( CustomReturnFunction *function, m_returnFunctions )
                {
                    result.append( function->value( tracks ) );
                }
            }
            else if( !m_returnValues.empty() )
            {
                Meta::TrackList resultTracks = tracks;
                if( m_orderByField )
                {
                    if( m_orderByNumberField )
                        resultTracks = MemoryQueryMakerHelper::orderListByNumber( resultTracks, m_orderByField, m_orderDescending );
                    else
                        resultTracks = MemoryQueryMakerHelper::orderListByString( resultTracks, m_orderByField, m_orderDescending );
                }
                if( m_randomize )
                {
                    KRandomSequence sequence;
                    sequence.randomize<Meta::TrackPtr>( resultTracks );
                }

                int count = 0;
                foreach( const Meta::TrackPtr &track, resultTracks )
                {
                    if ( m_maxSize >= 0 && count == m_maxSize )
                        break;

                    foreach( CustomReturnValue *value, m_returnValues )
                    {
                        result.append( value->value( track ) );
                    }
                    count++;
                }
            }
            emit newResultReady( m_collection->collectionId(), result );
            break;
        }
        case QueryMaker::Track :
        {
            Meta::TrackList newResult;

            if( m_orderByField )
            {
                if( m_orderByNumberField )
                    newResult = MemoryQueryMakerHelper::orderListByNumber( tracks, m_orderByField, m_orderDescending );
                else
                    newResult = MemoryQueryMakerHelper::orderListByString( tracks, m_orderByField, m_orderDescending );
            }
            else
                newResult = tracks;

            emitProperResult<Meta::TrackPtr>( newResult );
            break;
        }
        case QueryMaker::Album :
        {
            QSet<Meta::AlbumPtr> albumSet;
            foreach( Meta::TrackPtr track, tracks )
            {
                albumSet.insert( track->album() );
            }
            Meta::AlbumList albumList = albumSet.toList();
            albumList = MemoryQueryMakerHelper::orderListByName<Meta::AlbumPtr>( albumList, m_orderDescending );
            emitProperResult<Meta::AlbumPtr>( albumList );
            break;
        }
        case QueryMaker::Artist :
        {
            QSet<Meta::ArtistPtr> artistSet;
            foreach( Meta::TrackPtr track, tracks )
            {
                artistSet.insert( track->artist() );
            }
            Meta::ArtistList list = artistSet.toList();
            list = MemoryQueryMakerHelper::orderListByName<Meta::ArtistPtr>( list, m_orderDescending );
            emitProperResult<Meta::ArtistPtr>( list );
            break;
        }
        case QueryMaker::Genre :
        {
            QSet<Meta::GenrePtr> genreSet;
            foreach( Meta::TrackPtr track, tracks )
            {
                genreSet.insert( track->genre() );
            }
            Meta::GenreList list = genreSet.toList();
            list = MemoryQueryMakerHelper::orderListByName<Meta::GenrePtr>( list, m_orderDescending );
            emitProperResult<Meta::GenrePtr>( list );
            break;
        }
        case QueryMaker::Composer :
        {
            QSet<Meta::ComposerPtr> composerSet;
            foreach( Meta::TrackPtr track, tracks )
            {
                composerSet.insert( track->composer() );
            }
            Meta::ComposerList list = composerSet.toList();
            list = MemoryQueryMakerHelper::orderListByName<Meta::ComposerPtr>( list, m_orderDescending );
            emitProperResult<Meta::ComposerPtr>( list );
            break;
        }
        case QueryMaker::Year :
        {
            QSet<Meta::YearPtr> yearSet;
            foreach( Meta::TrackPtr track, tracks )
            {
                yearSet.insert( track->year() );
            }
            Meta::YearList years = yearSet.toList();
            if( m_orderByField == Meta::valYear )
            {
                years = MemoryQueryMakerHelper::orderListByYear( years, m_orderDescending );
            }

            emitProperResult<Meta::YearPtr>( years );
            break;
        }
        case QueryMaker::None:
            //should never happen, but handle error anyway
            break;
    }
}

void
MemoryQueryMakerInternal::setMatchers( MemoryMatcher *matchers )
{
    m_matchers = matchers;
}

void
MemoryQueryMakerInternal::setFilters( MemoryFilter *filters )
{
    m_filters = filters;
}

void
MemoryQueryMakerInternal::setRandomize( bool randomize )
{
    m_randomize = randomize;
}

void
MemoryQueryMakerInternal::setMaxSize( int maxSize )
{
    m_maxSize = maxSize;
}

void
MemoryQueryMakerInternal::setReturnAsDataPtrs( bool returnAsDataPtrs )
{
    m_returnAsDataPtrs = returnAsDataPtrs;
}

void
MemoryQueryMakerInternal::setType( QueryMaker::QueryType type )
{
    m_type = type;
}

void
MemoryQueryMakerInternal::setCustomReturnFunctions( const QList<CustomReturnFunction *> &functions )
{
    m_returnFunctions = functions;
}

void
MemoryQueryMakerInternal::setCustomReturnValues( const QList<CustomReturnValue *> &values )
{
    m_returnValues = values;
}

#include "MemoryQueryMakerInternal.moc"
