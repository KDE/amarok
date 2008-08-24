/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
             (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "MemoryQueryMaker.h"
#include "MemoryFilter.h"
#include "MemoryMatcher.h"
#include "Debug.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QSet>
#include <QStack>

#include <KRandomSequence>

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
    bool usingFilters;
    bool randomize;
    KRandomSequence sequence;   //do not reset
};

MemoryQueryMaker::MemoryQueryMaker( MemoryCollection *mc, const QString &collectionId )
    : QueryMaker()
    , m_memCollection( mc )
    , m_collectionId( collectionId )
    ,d( new Private )
{
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
    m_memCollection->acquireReadLock();
    //naive implementation, fix this
    if ( d->matcher )
    {
        TrackList result = d->matcher->match( m_memCollection );
        if ( d->usingFilters )
        {
            TrackList filtered;
            foreach( TrackPtr track, result )
                if( d->containerFilters.first()->filterMatches( track ) )
                    filtered.append( track );
            handleResult( filtered );
        }
        else
            handleResult( result );
    }
    else if ( d->usingFilters )
    {
        TrackList tracks = m_memCollection->trackMap().values();
        TrackList filtered;
        foreach( const TrackPtr &track, tracks )
        {
            if ( d->containerFilters.first()->filterMatches( track ) )
            {
                filtered.append( track );
            }
        }
        handleResult( filtered );
    }
    else
        handleResult();
    m_memCollection->releaseLock();
}

// What's worse, a bunch of almost identical repeated code, or a not so obvious macro? :-)
// The macro below will emit the proper result signal. If m_resultAsDataPtrs is true,
// it'll emit the signal that takes a list of DataPtrs. Otherwise, it'll call the
// signal that takes the list of the specific class.
// (copied from sqlquerybuilder.cpp with a few minor tweaks)

#define emitProperResult( PointerType, list ) { \
            if( d->randomize ) { \
                d->sequence.randomize<PointerType >( list ); \
            } \
            if ( d->returnDataPtrs ) { \
                DataList data; \
                foreach( PointerType p, list ) { \
                    data << DataPtr::staticCast( p ); \
                } \
                emit newResultReady( m_collectionId, data ); \
            } \
            else { \
                emit newResultReady( m_collectionId, list ); \
            } \
        }

void
MemoryQueryMaker::handleResult()
{
    //this gets called when we want to return all values for the given query type
    switch( d->type )
    {
        case QueryMaker::Track :
        {
            TrackList tracks = m_memCollection->trackMap().values();
            if ( d->maxsize >= 0 && tracks.count() > d->maxsize )
                tracks = tracks.mid( 0, d->maxsize );
            emitProperResult( TrackPtr, tracks );
            break;
        }
        case QueryMaker::Album :
        {
            AlbumList albums = m_memCollection->albumMap().values();
            if ( d->maxsize >= 0 && albums.count() > d->maxsize )
                albums = albums.mid( 0, d->maxsize );
            emitProperResult( AlbumPtr, albums );
            break;
        }
        case QueryMaker::Artist :
        {
            ArtistList artists = m_memCollection->artistMap().values();
            if ( d->maxsize >= 0 && artists.count() > d->maxsize )
                artists = artists.mid( 0, d->maxsize );
            emitProperResult( ArtistPtr, artists );
            break;
        }
        case QueryMaker::Composer :
        {
            ComposerList composers = m_memCollection->composerMap().values();
            if ( d->maxsize >= 0 && composers.count() > d->maxsize )
                composers = composers.mid( 0, d->maxsize );
            emitProperResult( ComposerPtr, composers );
            break;
        }
        case QueryMaker::Genre :
        {
            GenreList genres = m_memCollection->genreMap().values();
            if ( d->maxsize >= 0 && genres.count() > d->maxsize )
                genres = genres.mid( 0, d->maxsize );
            emitProperResult( GenrePtr, genres );
            break;
        }
        case QueryMaker::Year :
        {
            YearList years = m_memCollection->yearMap().values();
            if ( d->maxsize >= 0 && years.count() > d->maxsize )
                years = years.mid( 0, d->maxsize );
            emitProperResult( YearPtr, years );
            break;
        }
        case QueryMaker::Custom :
            //TODO stub, fix this
            break;
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
        case QueryMaker::Track :
        {
                TrackList newResult;
            if ( d->maxsize < 0 || tracks.count() <= d->maxsize )
            {
                newResult = tracks;
            }
            else
            {
                newResult = tracks.mid( 0, d->maxsize );
            }
            emitProperResult( TrackPtr, newResult );
            break;
        }
        case QueryMaker::Album :
        {
            QSet<AlbumPtr> albumSet;
            foreach( TrackPtr track, tracks )
            {
                if ( d->maxsize >= 0 && albumSet.count() == d->maxsize )
                    break;
                albumSet.insert( track->album() );
            }
            AlbumList albumList = albumSet.toList();
            emitProperResult( AlbumPtr, albumList );
            break;
        }
        case QueryMaker::Artist :
        {
            QSet<ArtistPtr> artistSet;
            foreach( TrackPtr track, tracks )
            {
                if ( d->maxsize >= 0 && artistSet.count() == d->maxsize )
                    break;
                artistSet.insert( track->artist() );
            }
            ArtistList list = artistSet.toList();
            emitProperResult( ArtistPtr, list );
            break;
        }
        case QueryMaker::Genre :
        {
            QSet<GenrePtr> genreSet;
            foreach( TrackPtr track, tracks )
            {
                if ( d->maxsize >= 0 && genreSet.count() == d->maxsize )
                    break;
                genreSet.insert( track->genre() );
            }
            GenreList list = genreSet.toList();
            emitProperResult( GenrePtr, list );
            break;
        }
        case QueryMaker::Composer :
        {
            QSet<ComposerPtr> composerSet;
            foreach( TrackPtr track, tracks )
            {
                if ( d->maxsize >= 0 && composerSet.count() == d->maxsize )
                    break;
                composerSet.insert( track->composer() );
            }
            ComposerList list = composerSet.toList();
            emitProperResult( ComposerPtr, list );
            break;
        }
        case QueryMaker::Year :
        {
            QSet<YearPtr> yearSet;
            foreach( TrackPtr track, tracks )
            {
                if ( d->maxsize >= 0 && yearSet.count() == d->maxsize )
                    break;
                yearSet.insert( track->year() );
            }
            YearList list = yearSet.toList();
            emitProperResult( YearPtr, list );
            break;
        }
        case QueryMaker::Custom :
            //hmm, not sure if this makes sense
            break;
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
        if ( d->type == QueryMaker::Custom )
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
    Q_UNUSED( value );
    //TODO stub
    return this;
}

QueryMaker*
MemoryQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    Q_UNUSED( value )
    switch( function )
    {
        case QueryMaker::Count:
            //TODO
            break;
        case QueryMaker::Sum:
            //TODO
            break;
        case QueryMaker::Min:
            //TODO
            break;
        case QueryMaker::Max:
            //TODO
            break;
    }
    return this;
}

QueryMaker*
MemoryQueryMaker::orderBy( qint64 value, bool descending )
{
    Q_UNUSED( value ); Q_UNUSED( descending );
    //TODO stub
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

#include "MemoryQueryMaker.moc"
