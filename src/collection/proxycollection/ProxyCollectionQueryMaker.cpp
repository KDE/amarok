/*
 *  Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define DEBUG_PREFIX "ProxyCollectionQueryMaker"

#include "ProxyCollectionQueryMaker.h"

#include "collection/support/MemoryCustomValue.h"
#include "collection/support/MemoryQueryMakerHelper.h"
#include "Debug.h"
#include "meta/Meta.h"
#include "ProxyCollection.h"

#include <QMetaEnum>
#include <QMetaObject>

using namespace ProxyCollection;

ProxyQueryMaker::ProxyQueryMaker( ProxyCollection::Collection *collection, const QList<QueryMaker*> &queryMakers )
    : QueryMaker()
    , m_collection( collection )
    , m_builders( queryMakers )
    , m_queryDoneCount( 0 )
    , m_returnDataPointers( false )
    , m_maxResultSize( -1 )
    , m_randomize( false )
    , m_queryType( QueryMaker::None )
    , m_orderDescending( false )
    , m_orderField( 0 )
    , m_orderByNumberField( false )
    , m_queryDoneCountMutex()
{
    foreach( QueryMaker *b, m_builders )
    {
        connect( b, SIGNAL( queryDone() ), this, SLOT( slotQueryDone() ) );
        //relay signals directly
        connect( b, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( slotNewResultReady( QString, Meta::TrackList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::ArtistList ) ), this, SLOT( slotNewResultReady( QString, Meta::ArtistList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::AlbumList ) ), this, SLOT( slotNewResultReady( QString, Meta::AlbumList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::GenreList ) ), this, SLOT( slotNewResultReady( QString, Meta::GenreList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::ComposerList ) ), this, SLOT( slotNewResultReady( QString, Meta::ComposerList ) ), Qt::DirectConnection );
        connect( b, SIGNAL( newResultReady( QString, Meta::YearList ) ), this, SLOT( slotNewResultReady( QString, Meta::YearList ) ), Qt::DirectConnection );
    }
}

ProxyQueryMaker::~ProxyQueryMaker()
{
    qDeleteAll( m_returnFunctions );
    qDeleteAll( m_returnValues );
    qDeleteAll( m_builders );
}

QueryMaker*
ProxyQueryMaker::reset()
{
    m_queryDoneCount = 0;
    m_returnDataPointers = false;
    m_maxResultSize = -1;
    m_randomize = false;
    m_queryType = QueryMaker::None;
    m_orderDescending = false;
    m_orderField = 0;
    qDeleteAll( m_returnFunctions );
    m_returnFunctions.clear();
    qDeleteAll( m_returnValues );
    m_returnValues.clear();
    foreach( QueryMaker *b, m_builders )
        b->reset();
    return this;
}

void
ProxyQueryMaker::run()
{
    foreach( QueryMaker *b, m_builders )
        b->run();
}

void
ProxyQueryMaker::abortQuery()
{
    foreach( QueryMaker *b, m_builders )
        b->abortQuery();
}

int
ProxyQueryMaker::resultCount() const
{
    return 1;
}

QueryMaker*
ProxyQueryMaker::setQueryType( QueryType type )
{
    m_queryType = type;
    if( type == QueryMaker::Track )
    {
        foreach( QueryMaker *b, m_builders )
            b->setQueryType( QueryMaker::Track );
        return this;
    }
    else if( type == QueryMaker::Artist )
    {
        foreach( QueryMaker *b, m_builders )
            b->setQueryType( QueryMaker::Artist );
        return this;
    }
    else if( type == QueryMaker::Album )
    {
        foreach( QueryMaker *b, m_builders )
            b->setQueryType( QueryMaker::Album );
        return this;
    }
    else if( type == QueryMaker::Genre )
    {
        foreach( QueryMaker *b, m_builders )
            b->setQueryType( QueryMaker::Genre );
        return this;
    }
    else if( type == QueryMaker::Composer )
    {
        foreach( QueryMaker *b, m_builders )
            b->setQueryType( QueryMaker::Composer );
        return this;
    }
    else if( type == QueryMaker::Year )
    {
        foreach( QueryMaker *b, m_builders )
            b->setQueryType( QueryMaker::Year );
        return this;
    }
    else
    {
        //we cannot forward custom queries as there is no way to integrate the results
        //delivered by the QueryMakers. Instead we ask for tracks that match the criterias,
        //and then generate the custom result similar to MemoryQueryMaker.
        //And yes, this means that we will load all tracks when we simply want the count of tracks
        //in the collection. It might be necessary to add some specific logic for that case.
        //On second thought, there is no way around loading all objects, as we want to operate on distinct
        //elements (for some value of distinct) in ProxyCollection. We can only figure out what the union
        //of all elements is after loading them in memory
        foreach( QueryMaker *b, m_builders )
            b->setQueryType( QueryMaker::Track );
        return this;
    }
}

QueryMaker*
ProxyQueryMaker::addReturnValue( qint64 value )
{
    //do not forward this call, see comment in setQueryType()
    m_returnValues.append( CustomValueFactory::returnValue( value ) );
    return this;
}

QueryMaker*
ProxyQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    //do not forward this call, see comment in setQueryType()
    m_returnFunctions.append( CustomValueFactory::returnFunction( function, value ) );
    return this;
}

QueryMaker*
ProxyQueryMaker::orderBy( qint64 value, bool descending )
{
    m_orderDescending = descending;
    m_orderField = value;
    //copied from MemoryQueryMaker. TODO: think of a sensible place to put this code
    switch( value )
    {
        case Meta::valYear:
        case Meta::valDiscNr:
        case Meta::valTrackNr:
        case Meta::valScore:
        case Meta::valRating:
        case Meta::valPlaycount:
        case Meta::valFilesize:
        case Meta::valSamplerate:
        case Meta::valBitrate:
        case Meta::valLength:
        {
            m_orderByNumberField = true;
            break;
        }
        //TODO: what about Meta::valFirstPlayed, Meta::valCreateDate or Meta::valLastPlayed??

        default:
            m_orderByNumberField = false;
    }
    foreach( QueryMaker *b, m_builders )
        b->orderBy( value, descending );
    return this;
}

QueryMaker*
ProxyQueryMaker::orderByRandom()
{
    m_randomize = true;
    foreach( QueryMaker *b, m_builders )
        b->orderByRandom();
    return this;
}

QueryMaker*
ProxyQueryMaker::includeCollection( const QString &collectionId )
{
    foreach( QueryMaker *b, m_builders )
        b->includeCollection( collectionId );
    return this;
}

QueryMaker*
ProxyQueryMaker::excludeCollection( const QString &collectionId )
{
    foreach( QueryMaker *b, m_builders )
        b->excludeCollection( collectionId );
    return this;
}

QueryMaker*
ProxyQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    foreach( QueryMaker *b, m_builders )
        b->addFilter( value, filter, matchBegin, matchEnd );
    return this;
}

QueryMaker*
ProxyQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    foreach( QueryMaker *b, m_builders )
        b->excludeFilter( value, filter, matchBegin, matchEnd );
    return this;
}

QueryMaker*
ProxyQueryMaker::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    foreach( QueryMaker *b, m_builders )
        b->addNumberFilter( value, filter, compare);
    return this;
}

QueryMaker*
ProxyQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    foreach( QueryMaker *b, m_builders )
        b->excludeNumberFilter( value, filter, compare );
    return this;
}

QueryMaker*
ProxyQueryMaker::addMatch( const Meta::TrackPtr &track )
{
    foreach( QueryMaker *b, m_builders )
        b->addMatch( track );
    return this;
}

QueryMaker*
ProxyQueryMaker::addMatch( const Meta::ArtistPtr &artist )
{
    foreach( QueryMaker *b, m_builders )
        b->addMatch( artist );
    return this;
}

QueryMaker*
ProxyQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
    foreach( QueryMaker *b, m_builders )
        b->addMatch( album );
    return this;
}

QueryMaker*
ProxyQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    foreach( QueryMaker *b, m_builders )
        b->addMatch( genre );
    return this;
}

QueryMaker*
ProxyQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
    foreach( QueryMaker *b, m_builders )
        b->addMatch( composer );
    return this;
}

QueryMaker*
ProxyQueryMaker::addMatch( const Meta::YearPtr &year )
{
    foreach( QueryMaker *b, m_builders )
        b->addMatch( year );
    return this;
}

QueryMaker*
ProxyQueryMaker::addMatch( const Meta::DataPtr &data )
{
    Meta::DataPtr tmp = const_cast<Meta::DataPtr&>( data );
    foreach( QueryMaker *b, m_builders )
        tmp->addMatchTo( b );
    return this;
}

QueryMaker*
ProxyQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
    //no point in forwarding this call
    //just let all m_builders return the actual type, then we do not have to cast to subtypes here
    m_returnDataPointers = resultAsDataPtrs;
    return this;
}

QueryMaker*
ProxyQueryMaker::limitMaxResultSize( int size )
{
    //forward the call so the m_builders do not have to do work
    //that we definitely know is unnecessary (like returning more than size results)
    //we have to limit the combined result of all m_builders nevertheless
    m_maxResultSize = size;
    foreach( QueryMaker *b, m_builders )
        b->limitMaxResultSize( size );
    return this;
}

QueryMaker*
ProxyQueryMaker::beginAnd()
{
    foreach( QueryMaker *b, m_builders )
        b->beginAnd();
    return this;
}

QueryMaker*
ProxyQueryMaker::beginOr()
{
    foreach( QueryMaker *b, m_builders )
        b->beginOr();
    return this;
}

QueryMaker*
ProxyQueryMaker::endAndOr()
{
    foreach( QueryMaker *b, m_builders )
        b->endAndOr();
    return this;
}

QueryMaker*
ProxyQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    foreach( QueryMaker *b, m_builders )
        b->setAlbumQueryMode( mode );
    return this;
}

void
ProxyQueryMaker::slotQueryDone()
{
    m_queryDoneCountMutex.lock();
    m_queryDoneCount++;
    if ( m_queryDoneCount == m_builders.size() )
    {
        //make sure we don't give control to code outside this class while holding the lock
        m_queryDoneCountMutex.unlock();
        handleResult();
        emit queryDone();
    }
    else
    {
        m_queryDoneCountMutex.unlock();
    }
}

template <class PointerType>
void ProxyQueryMaker::emitProperResult( const QList<PointerType>& list )
{
   QList<PointerType> resultList = list;
    if( m_randomize )
        m_sequence.randomize<PointerType>( resultList );

    if ( m_maxResultSize >= 0 && resultList.count() > m_maxResultSize )
        resultList = resultList.mid( 0, m_maxResultSize );

    if( m_returnDataPointers )
    {
        Meta::DataList data;
        foreach( PointerType p, resultList )
            data << Meta::DataPtr::staticCast( p );

        emit newResultReady( m_collection->collectionId(), data );
    }
    else
        emit newResultReady( m_collection->collectionId(), list );
}

void
ProxyQueryMaker::handleResult()
{
    //copied from MemoryQueryMaker::handleResult()
    switch( m_queryType )
    {
        case QueryMaker::Custom :
        {
            QStringList result;
            Meta::TrackList tracks;
            foreach( KSharedPtr<ProxyCollection::Track> track, m_tracks )
            {
                tracks.append( Meta::TrackPtr::staticCast( track ) );
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
                if( m_orderField )
                {
                    if( m_orderByNumberField )
                        tracks = MemoryQueryMakerHelper::orderListByNumber( tracks, m_orderField, m_orderDescending );
                    else
                        tracks = MemoryQueryMakerHelper::orderListByString( tracks, m_orderField, m_orderDescending );
                }
                if( m_randomize )
                {
                    m_sequence.randomize<Meta::TrackPtr>( tracks );
                }

                int count = 0;
                foreach( const Meta::TrackPtr &track, tracks )
                {
                    if ( m_maxResultSize >= 0 && count == m_maxResultSize )
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
            foreach( KSharedPtr<ProxyCollection::Track> track, m_tracks )
            {
                tracks.append( Meta::TrackPtr::staticCast( track ) );
            }

            if( m_orderField )
            {
                if( m_orderByNumberField )
                    tracks = MemoryQueryMakerHelper::orderListByNumber( tracks, m_orderField, m_orderDescending );
                else
                    tracks = MemoryQueryMakerHelper::orderListByString( tracks, m_orderField, m_orderDescending );
            }

            emitProperResult<Meta::TrackPtr>( tracks );
            break;
        }
        case QueryMaker::Album :
        {
            Meta::AlbumList albums;
            foreach( KSharedPtr<ProxyCollection::Album> album, m_albums )
            {
                albums.append( Meta::AlbumPtr::staticCast( album ) );
            }

            albums = MemoryQueryMakerHelper::orderListByName<Meta::AlbumPtr>( albums, m_orderDescending );

            emitProperResult<Meta::AlbumPtr>( albums );
            break;
        }
        case QueryMaker::Artist :
        {
            Meta::ArtistList artists;
            foreach( KSharedPtr<ProxyCollection::Artist> artist, m_artists )
            {
                artists.append( Meta::ArtistPtr::staticCast( artist ) );
            }

            artists = MemoryQueryMakerHelper::orderListByName<Meta::ArtistPtr>( artists, m_orderDescending );
            emitProperResult<Meta::ArtistPtr>( artists );
            break;
        }
        case QueryMaker::Composer :
        {
            Meta::ComposerList composers;
            foreach( KSharedPtr<ProxyCollection::Composer> composer, m_composers )
            {
                composers.append( Meta::ComposerPtr::staticCast( composer ) );
            }

            composers = MemoryQueryMakerHelper::orderListByName<Meta::ComposerPtr>( composers, m_orderDescending );

            emitProperResult<Meta::ComposerPtr>( composers );
            break;
        }
        case QueryMaker::Genre :
        {
            Meta::GenreList genres;
            foreach( KSharedPtr<ProxyCollection::Genre> genre, m_genres )
            {
                genres.append( Meta::GenrePtr::staticCast( genre ) );
            }

            genres = MemoryQueryMakerHelper::orderListByName<Meta::GenrePtr>( genres, m_orderDescending );

            emitProperResult<Meta::GenrePtr>( genres );
            break;
        }
        case QueryMaker::Year :
        {
            Meta::YearList years;
            foreach( KSharedPtr<ProxyCollection::Year> year, m_years )
            {
                years.append( Meta::YearPtr::staticCast( year ) );
            }

            //years have to be ordered as numbers, but orderListByNumber does not work for Meta::YearPtrs
            if( m_orderField == Meta::valYear )
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
    m_tracks.clear();
    m_albums.clear();
    m_artists.clear();
    m_composers.clear();
    m_genres.clear();
    m_years.clear();
}

void
ProxyQueryMaker::slotNewResultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    Q_UNUSED( collectionId )

    foreach( const Meta::TrackPtr &track, tracks )
    {
        m_tracks.insert( KSharedPtr<ProxyCollection::Track>( m_collection->getTrack( track ) ) );
    }
}

void
ProxyQueryMaker::slotNewResultReady( const QString &collectionId, const Meta::ArtistList &artists )
{
    Q_UNUSED( collectionId )

    foreach( const Meta::ArtistPtr &artist, artists )
    {
        m_artists.insert( KSharedPtr<ProxyCollection::Artist>( m_collection->getArtist( artist ) ) );
    }
}

void
ProxyQueryMaker::slotNewResultReady( const QString &collectionId, const Meta::AlbumList &albums )
{
    Q_UNUSED( collectionId )

    foreach( const Meta::AlbumPtr &album, albums )
    {
        m_albums.insert( KSharedPtr<ProxyCollection::Album>( m_collection->getAlbum( album ) ) );
    }
}

void
ProxyQueryMaker::slotNewResultReady( const QString &collectionId, const Meta::GenreList &genres )
{
    Q_UNUSED( collectionId )

    foreach( const Meta::GenrePtr &genre, genres )
    {
        m_genres.insert( KSharedPtr<ProxyCollection::Genre>( m_collection->getGenre( genre ) ) );
    }
}

void
ProxyQueryMaker::slotNewResultReady( const QString &collectionId, const Meta::ComposerList &composers )
{
    Q_UNUSED( collectionId )

    foreach( const Meta::ComposerPtr &composer, composers )
    {
        m_composers.insert( KSharedPtr<ProxyCollection::Composer>( m_collection->getComposer( composer ) ) );
    }
}

void
ProxyQueryMaker::slotNewResultReady( const QString &collectionId, const Meta::YearList &years )
{
    Q_UNUSED( collectionId )

    foreach( const Meta::YearPtr &year, years )
    {
        m_years.insert( KSharedPtr<ProxyCollection::Year>( m_collection->getYear( year ) ) );
    }
}

#include "ProxyCollectionQueryMaker.moc"

