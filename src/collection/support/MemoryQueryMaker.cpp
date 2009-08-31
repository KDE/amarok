/****************************************************************************************
 * Copyright (c) 2007-2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2007-2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MemoryQueryMaker.h"
#include "MemoryCustomValue.h"
#include "MemoryFilter.h"
#include "MemoryMatcher.h"
#include "MemoryQueryMakerHelper.h"
#include "Debug.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QList>
#include <QSet>
#include <QStack>
#include <QtAlgorithms>

#include <KRandomSequence>
#include <KSortableList>

using namespace Meta;

//QueryJob

class QueryJob : public ThreadWeaver::Job
{
    public:
        QueryJob( MemoryQueryMaker *qm )
            : ThreadWeaver::Job()
            , m_queryMaker( qm )
        {
            //nothing to do
        }

    protected:
        void run()
        {
            m_queryMaker->runQuery();
            setFinished( true );
        }

    private:
        MemoryQueryMaker *m_queryMaker;
};

struct MemoryQueryMaker::Private {
    QueryMaker::QueryType type;
    bool returnDataPtrs;
    MemoryMatcher* matcher;
    QueryJob *job;
    int maxsize;
    QStack<ContainerMemoryFilter*> containerFilters;
    QList<CustomReturnFunction*> returnFunctions;
    QList<CustomReturnValue*> returnValues;
    bool usingFilters;
    bool randomize;
    KRandomSequence sequence;   //do not reset
    qint64 orderByField;
    bool orderDescending;
    bool orderByNumberField;
    AlbumQueryMode albumQueryMode;
};

MemoryQueryMaker::MemoryQueryMaker( MemoryCollection *mc, const QString &collectionId )
    : QueryMaker()
    , m_collection( mc )
    , d( new Private )
{
    m_collection->setCollectionId( collectionId );
    d->matcher = 0;
    d->job = 0;
    reset();
}

MemoryQueryMaker::~MemoryQueryMaker()
{
    if( !d->containerFilters.isEmpty() )
        delete d->containerFilters.first();
    delete d;
}

QueryMaker*
MemoryQueryMaker::reset()
{
    d->type = QueryMaker::None;
    d->returnDataPtrs = false;
    delete d->matcher;
    delete d->job;
    d->maxsize = -1;
    if( !d->containerFilters.isEmpty() )
        delete d->containerFilters.first();
    d->containerFilters.clear();
    d->containerFilters.push( new AndContainerMemoryFilter() );
    d->usingFilters = false;
    d->randomize = false;
    qDeleteAll( d->returnFunctions );
    d->returnFunctions.clear();
    qDeleteAll( d->returnValues );
    d->returnValues.clear();
    d->orderByField = 0;
    d->orderDescending = false;
    d->orderByNumberField = false;
    d->albumQueryMode = AllAlbums;
    return this;
}

void
MemoryQueryMaker::run()
{
    if ( d->type == QueryMaker::None )
        //TODO error handling
        return;
    else if( d->job && !d->job->isFinished() )
    {
        //the worker thread seems to be running
        //TODO: wait or job to complete
    }
    else
    {
        d->job = new QueryJob( this );
        connect( d->job, SIGNAL( done( ThreadWeaver::Job * ) ), SLOT( done( ThreadWeaver::Job * ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( d->job );
    }
}

void
MemoryQueryMaker::abortQuery()
{
}

void
MemoryQueryMaker::runQuery()
{
    m_collection->acquireReadLock();
    //naive implementation, fix this
    if ( d->matcher )
    {
        TrackList result = d->matcher->match( m_collection );
        if ( d->usingFilters )
        {
            TrackList filtered;
            foreach( TrackPtr track, result )
            {
                if( d->containerFilters.first()->filterMatches( track ) )
                    filtered.append( track );
            }
            handleResult( filtered );
        }
        else
            handleResult( result );
    }
    else if ( d->usingFilters )
    {
        TrackList tracks = m_collection->trackMap().values();
        TrackList filtered;
        foreach( const TrackPtr &track, tracks )
        {
            if ( d->containerFilters.first()->filterMatches( track ) )
                filtered.append( track );
        }
        handleResult( filtered );
    }
    else
        handleResult();
    m_collection->releaseLock();
}

template <class PointerType>
void MemoryQueryMaker::emitProperResult( const QList<PointerType>& list )
{
   QList<PointerType> resultList = list;
    if( d->randomize )
        d->sequence.randomize<PointerType>( resultList );

    if ( d->maxsize >= 0 && resultList.count() > d->maxsize )
        resultList = resultList.mid( 0, d->maxsize );

    if( d->returnDataPtrs )
    {
        DataList data;
        foreach( PointerType p, resultList )
            data << DataPtr::staticCast( p );

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
MemoryQueryMaker::handleResult()
{
    //this gets called when we want to return all values for the given query type
    switch( d->type )
    {
        case QueryMaker::Custom :
        {
            QStringList result;
            TrackList tracks = m_collection->trackMap().values();
            if( !d->returnFunctions.empty() )
            {
                //no sorting necessary
                foreach( CustomReturnFunction *function, d->returnFunctions )
                {
                    result.append( function->value( tracks ) );
                }
            }
            else if( !d->returnValues.empty() )
            {
                if( d-> orderByField )
                {
                    if( d->orderByNumberField )
                        tracks = MemoryQueryMakerHelper::orderListByNumber( tracks, d->orderByField, d->orderDescending );
                    else
                        tracks = MemoryQueryMakerHelper::orderListByString( tracks, d->orderByField, d->orderDescending );
                }
                if( d->randomize )
                    d->sequence.randomize<Meta::TrackPtr>( tracks );

                int count = 0;
                foreach( const Meta::TrackPtr &track, tracks )
                {
                    if ( d->maxsize >= 0 && count == d->maxsize )
                        break;
                
                    foreach( CustomReturnValue *value, d->returnValues )
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
            TrackList tracks;

            foreach( TrackPtr track, m_collection->trackMap().values() )
            {
                if( d->albumQueryMode == AllAlbums
                    || ( d->albumQueryMode == OnlyCompilations && track->album()->isCompilation() )
                    || ( d->albumQueryMode == OnlyNormalAlbums && !track->album()->isCompilation()) )
                {
                    tracks.append( track );
                }
            }

            if( d->orderByField )
            {
                if( d->orderByNumberField )
                    tracks = MemoryQueryMakerHelper::orderListByNumber( tracks, d->orderByField, d->orderDescending );
                else
                    tracks = MemoryQueryMakerHelper::orderListByString( tracks, d->orderByField, d->orderDescending );
            }

            emitProperResult<TrackPtr>( tracks );
            break;
        }
        case QueryMaker::Album :
        {

            AlbumList albums;
            foreach( AlbumPtr album, m_collection->albumMap().values() )
            {
                if( d->albumQueryMode == AllAlbums
                    || ( d->albumQueryMode == OnlyCompilations && album->isCompilation() )
                    || ( d->albumQueryMode == OnlyNormalAlbums && !album->isCompilation()) )
                {
                    albums.append( album );
                    break;
                }
            }
 
            albums = MemoryQueryMakerHelper::orderListByName<Meta::AlbumPtr>( albums, d->orderDescending );

            emitProperResult<AlbumPtr>( albums );
            break;
        }
        case QueryMaker::Artist :
        {
            ArtistList artists;
            foreach( ArtistPtr artist, m_collection->artistMap().values() )
            {
                TrackList tracks = artist->tracks();
                foreach( TrackPtr track, tracks )
                {
                    if( d->albumQueryMode == AllAlbums
                        || ( d->albumQueryMode == OnlyCompilations && track->album()->isCompilation() )
                        || ( d->albumQueryMode == OnlyNormalAlbums && !track->album()->isCompilation()) )
                    {
                        artists.append( artist );
                        break;
                    }
                }
            }
            artists = MemoryQueryMakerHelper::orderListByName<Meta::ArtistPtr>( artists, d->orderDescending );
            emitProperResult<ArtistPtr>( artists );
            break;
        }
        case QueryMaker::Composer :
        {
            ComposerList composers;
            foreach( ComposerPtr composer, m_collection->composerMap().values() )
            {
                TrackList tracks = composer->tracks();
                foreach( TrackPtr track, tracks )
                {
                    if( d->albumQueryMode == AllAlbums
                        || ( d->albumQueryMode == OnlyCompilations && track->album()->isCompilation() )
                        || ( d->albumQueryMode == OnlyNormalAlbums && !track->album()->isCompilation()) )
                    {
                        composers.append( composer );
                        break;
                    }
                }
            }
            composers = MemoryQueryMakerHelper::orderListByName<Meta::ComposerPtr>( composers, d->orderDescending );

            emitProperResult<ComposerPtr>( composers );
            break;
        }
        case QueryMaker::Genre :
        {
            GenreList genres;
            foreach( GenrePtr genre, m_collection->genreMap().values() )
            {
                TrackList tracks = genre->tracks();
                foreach( TrackPtr track, tracks )
                {
                    if( d->albumQueryMode == AllAlbums
                        || ( d->albumQueryMode == OnlyCompilations && track->album()->isCompilation() )
                        || ( d->albumQueryMode == OnlyNormalAlbums && !track->album()->isCompilation()) )
                    {
                        genres.append( genre );
                        break;
                    }
                }
            }
            
            genres = MemoryQueryMakerHelper::orderListByName<Meta::GenrePtr>( genres, d->orderDescending );

            emitProperResult<GenrePtr>( genres );
            break;
        }
        case QueryMaker::Year :
        {
            YearList years;
            foreach( YearPtr year, m_collection->yearMap().values() )
            {
                TrackList tracks = year->tracks();
                foreach( TrackPtr track, tracks )
                {
                    if( d->albumQueryMode == AllAlbums
                        || ( d->albumQueryMode == OnlyCompilations && track->album()->isCompilation() )
                        || ( d->albumQueryMode == OnlyNormalAlbums && !track->album()->isCompilation()) )
                    {
                        years.append( year );
                        break;
                    }
                }
            }
        
            //this a special case which requires a bit of code duplication
            //years have to be ordered as numbers, but orderListByNumber does not work for Meta::YearPtrs
            if( d->orderByField == Meta::valYear )
            {
                years = MemoryQueryMakerHelper::orderListByYear( years, d->orderDescending );
            }

            emitProperResult<YearPtr>( years );
            break;
        }
        case QueryMaker::None :
            //nothing to do
            break;
    }
}

void
MemoryQueryMaker::handleResult( const TrackList &tracks )
{
    switch( d->type )
    {
        case QueryMaker::Custom :
        {
            QStringList result;
            if( !d->returnFunctions.empty() )
            {
                //no sorting necessary
                foreach( CustomReturnFunction *function, d->returnFunctions )
                {
                    result.append( function->value( tracks ) );
                }
            }
            else if( !d->returnValues.empty() )
            {
                Meta::TrackList resultTracks = tracks;
                if( d->orderByField )
                {
                    if( d->orderByNumberField )
                        resultTracks = MemoryQueryMakerHelper::orderListByNumber( resultTracks, d->orderByField, d->orderDescending );
                    else
                        resultTracks = MemoryQueryMakerHelper::orderListByString( resultTracks, d->orderByField, d->orderDescending );
                }
                if( d->randomize )
                    d->sequence.randomize<Meta::TrackPtr>( resultTracks );

                int count = 0;
                foreach( const Meta::TrackPtr &track, resultTracks )
                {
                    if ( d->maxsize >= 0 && count == d->maxsize )
                        break;
                    
                    foreach( CustomReturnValue *value, d->returnValues )
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
            TrackList newResult;

            if( d->orderByField )
            {
                if( d->orderByNumberField )
                    newResult = MemoryQueryMakerHelper::orderListByNumber( tracks, d->orderByField, d->orderDescending );
                else
                    newResult = MemoryQueryMakerHelper::orderListByString( tracks, d->orderByField, d->orderDescending );
            }
            else
                newResult = tracks;

            emitProperResult<TrackPtr>( newResult );
            break;
        }
        case QueryMaker::Album :
        {
            QSet<AlbumPtr> albumSet;
            foreach( TrackPtr track, tracks )
            {
                if( d->albumQueryMode == AllAlbums
                    || ( d->albumQueryMode == OnlyCompilations && track->album()->isCompilation() )
                    || ( d->albumQueryMode == OnlyNormalAlbums && !track->album()->isCompilation()) )
                {
                    albumSet.insert( track->album() );
                }
            }
            AlbumList albumList = albumSet.toList();
            albumList = MemoryQueryMakerHelper::orderListByName<Meta::AlbumPtr>( albumList, d->orderDescending );
            emitProperResult<AlbumPtr>( albumList );
            break;
        }
        case QueryMaker::Artist :
        {
            QSet<ArtistPtr> artistSet;
            foreach( TrackPtr track, tracks )
            {
                if( d->albumQueryMode == AllAlbums
                    || ( d->albumQueryMode == OnlyCompilations && track->album()->isCompilation() )
                    || ( d->albumQueryMode == OnlyNormalAlbums && !track->album()->isCompilation()) )
                {
                    artistSet.insert( track->artist() );
                }
            }
            ArtistList list = artistSet.toList();
            list = MemoryQueryMakerHelper::orderListByName<Meta::ArtistPtr>( list, d->orderDescending );
            emitProperResult<ArtistPtr>( list );
            break;
        }
        case QueryMaker::Genre :
        {
            QSet<GenrePtr> genreSet;
            foreach( TrackPtr track, tracks )
            {
                if( d->albumQueryMode == AllAlbums
                    || ( d->albumQueryMode == OnlyCompilations && track->album()->isCompilation() )
                    || ( d->albumQueryMode == OnlyNormalAlbums && !track->album()->isCompilation()) )
                {
                    genreSet.insert( track->genre() );
                }
            }
            GenreList list = genreSet.toList();
            list = MemoryQueryMakerHelper::orderListByName<Meta::GenrePtr>( list, d->orderDescending );
            emitProperResult<GenrePtr>( list );
            break;
        }
        case QueryMaker::Composer :
        {
            QSet<ComposerPtr> composerSet;
            foreach( TrackPtr track, tracks )
            {
                if( d->albumQueryMode == AllAlbums
                    || ( d->albumQueryMode == OnlyCompilations && track->album()->isCompilation() )
                    || ( d->albumQueryMode == OnlyNormalAlbums && !track->album()->isCompilation()) )
                {
                    composerSet.insert( track->composer() );
                }
            }
            ComposerList list = composerSet.toList();
            list = MemoryQueryMakerHelper::orderListByName<Meta::ComposerPtr>( list, d->orderDescending );
            emitProperResult<ComposerPtr>( list );
            break;
        }
        case QueryMaker::Year :
        {
            QSet<YearPtr> yearSet;
            foreach( TrackPtr track, tracks )
            {
                yearSet.insert( track->year() );
            }
            YearList years = yearSet.toList();
            if( d->orderByField == Meta::valYear )
            {
                years = MemoryQueryMakerHelper::orderListByYear( years, d->orderDescending );
            }

            emitProperResult<YearPtr>( years );
            break;
        }
        case QueryMaker::None:
            //should never happen, but handle error anyway
            break;
    }
}

QueryMaker*
MemoryQueryMaker::setQueryType( QueryType type )
{
    switch( type ) {
    case QueryMaker::Track:
        if ( d->type == QueryMaker::None )
            d->type = QueryMaker::Track;
        return this;

    case QueryMaker::Artist:
        if ( d->type == QueryMaker::None )
            d->type = QueryMaker::Artist;
        return this;

    case QueryMaker::Album:
        if ( d->type == QueryMaker::None )
            d->type = QueryMaker::Album;
        return this;

    case QueryMaker::Composer:
        if ( d->type == QueryMaker::None )
            d->type = QueryMaker::Composer;
        return this;

    case QueryMaker::Genre:
        if ( d->type == QueryMaker::None )
            d->type = QueryMaker::Genre;
        return this;

    case QueryMaker::Year:
        if ( d->type == QueryMaker::None )
            d->type = QueryMaker::Year;
        return this;

    case QueryMaker::Custom:
        if ( d->type == QueryMaker::None )
            d->type = QueryMaker::Custom;
        return this;
    case QueryMaker::None:
        return this;
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->returnDataPtrs = resultAsDataPtrs;
    return this;
}

QueryMaker*
MemoryQueryMaker::addReturnValue( qint64 value )
{
    //MQM can not deliver sensible results if both a custom return value and a return function is selected
    if( d->returnFunctions.empty() )
    {
        CustomReturnValue *returnValue = CustomValueFactory::returnValue( value );
        if( returnValue )
            d->returnValues.append( returnValue );
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    //MQM can not deliver sensible results if both a custom return value and a return function is selected
    if( d->returnValues.empty() )
    {
        CustomReturnFunction *returnFunction = CustomValueFactory::returnFunction( function, value );
        if( returnFunction )
            d->returnFunctions.append( returnFunction );
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::orderBy( qint64 value, bool descending )
{
    d->orderByField = value;
    d->orderDescending = descending;
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
            d->orderByNumberField = true;
            break;
        }
        //TODO: what about Meta::valFirstPlayed, Meta::valCreateDate or Meta::valLastPlayed??

        default:
            d->orderByNumberField = false;
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::orderByRandom()
{
    d->randomize = true;
    return this;
}

QueryMaker*
MemoryQueryMaker::includeCollection( const QString &collectionId )
{
    Q_UNUSED( collectionId );
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::excludeCollection( const QString &collectionId )
{
    Q_UNUSED( collectionId );
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const TrackPtr &track )
{
    MemoryMatcher *trackMatcher = new TrackMatcher( track );
    if ( d->matcher == 0 )
        d->matcher = trackMatcher;
    else
    {
        MemoryMatcher *tmp = d->matcher;
        while ( !tmp->isLast() )
            tmp = tmp->next();
        tmp->setNext( trackMatcher );
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const ArtistPtr &artist )
{
    MemoryMatcher *artistMatcher = new ArtistMatcher( artist );
    if ( d->matcher == 0 )
        d->matcher = artistMatcher;
    else
    {
        MemoryMatcher *tmp = d->matcher;
        while ( !tmp->isLast() )
            tmp = tmp->next();
        tmp->setNext( artistMatcher );
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const AlbumPtr &album )
{
    MemoryMatcher *albumMatcher = new AlbumMatcher( album );
    if ( d->matcher == 0 )
        d->matcher = albumMatcher;
    else
    {
        MemoryMatcher *tmp = d->matcher;
        while ( !tmp->isLast() )
            tmp = tmp->next();
        tmp->setNext( albumMatcher );
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const GenrePtr &genre )
{
    MemoryMatcher *genreMatcher = new GenreMatcher( genre );
    if ( d->matcher == 0 )
        d->matcher = genreMatcher;
    else
    {
        MemoryMatcher *tmp = d->matcher;
        while ( !tmp->isLast() )
            tmp = tmp->next();
        tmp->setNext( genreMatcher );
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const ComposerPtr &composer )
{
    MemoryMatcher *composerMatcher = new ComposerMatcher( composer );
    if ( d->matcher == 0 )
        d->matcher = composerMatcher;
    else
    {
        MemoryMatcher *tmp = d->matcher;
        while ( !tmp->isLast() )
            tmp = tmp->next();
        tmp->setNext( composerMatcher );
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const YearPtr &year )
{
    MemoryMatcher *yearMatcher = new YearMatcher( year );
    if ( d->matcher == 0 )
        d->matcher = yearMatcher;
    else
    {
        MemoryMatcher *tmp = d->matcher;
        while ( !tmp->isLast() )
            tmp = tmp->next();
        tmp->setNext( yearMatcher );
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const DataPtr &data )
{
    ( const_cast<DataPtr&>(data) )->addMatchTo( this );
    return this;
}

QueryMaker*
MemoryQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    d->containerFilters.top()->addFilter( FilterFactory::filter( value, filter, matchBegin, matchEnd ) );
    d->usingFilters = true;
    return this;
}

QueryMaker*
MemoryQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    MemoryFilter *tmp = FilterFactory::filter( value, filter, matchBegin, matchEnd );
    d->containerFilters.top()->addFilter( new NegateMemoryFilter( tmp ) );
    d->usingFilters = true;
    return this;
}

QueryMaker*
MemoryQueryMaker::addNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
    d->containerFilters.top()->addFilter( FilterFactory::numberFilter( value, filter, compare ) );
    d->usingFilters = true;
    return this;
}

QueryMaker*
MemoryQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
    MemoryFilter *tmp = FilterFactory::numberFilter( value, filter, compare );
    d->containerFilters.top()->addFilter( new NegateMemoryFilter( tmp ) );
    d->usingFilters = true;
    return this;
}

QueryMaker*
MemoryQueryMaker::limitMaxResultSize( int size )
{
    d->maxsize = size;
    return this;
}

QueryMaker*
MemoryQueryMaker::beginAnd()
{
    ContainerMemoryFilter *filter = new AndContainerMemoryFilter();
    d->containerFilters.top()->addFilter( filter );
    d->containerFilters.push( filter );
    return this;
}

QueryMaker*
MemoryQueryMaker::beginOr()
{
    ContainerMemoryFilter *filter = new OrContainerMemoryFilter();
    d->containerFilters.top()->addFilter( filter );
    d->containerFilters.push( filter );
    return this;
}

QueryMaker*
MemoryQueryMaker::endAndOr()
{
    d->containerFilters.pop();
    return this;
}

void
MemoryQueryMaker::done( ThreadWeaver::Job *job )
{
    ThreadWeaver::Weaver::instance()->dequeue( job );
    job->deleteLater();
    d->job = 0;
    emit queryDone();
}

QueryMaker * MemoryQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    d->albumQueryMode = mode;
    return this;
}

#include "MemoryQueryMaker.moc"
