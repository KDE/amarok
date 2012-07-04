/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#define DEBUG_PREFIX "NepomukCollection QueryMaker"

#include "NepomukCollection.h"
#include "NepomukQueryMaker.h"
#include "NepomukQueryMakerInternal.h"
#include "meta/NepomukTrack.h"
#include "meta/NepomukArtist.h"
#include "meta/NepomukGenre.h"
#include "meta/NepomukAlbum.h"
#include "meta/NepomukComposer.h"

#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryFilter.h"
#include "core-impl/collections/support/MemoryCustomValue.h"
#include "core-impl/collections/support/MemoryQueryMakerHelper.h"

// Nepomuk includes
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/Term>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Query/Result>
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Query/QueryParser>
#include <Nepomuk/Resource>
#include <Nepomuk/File>

// Threadweaver includes
#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QList>
#include <QSet>
#include <QStack>
#include <QtAlgorithms>
#include <KRandomSequence>
#include <KSortableList>

using namespace Nepomuk::Query;
using namespace Collections;
using namespace FilterFactory;
using namespace CustomValueFactory;

class NepomukQueryJob : public ThreadWeaver::Job
{
public:
    NepomukQueryJob( NepomukQueryMakerInternal *queryMaker )
        : ThreadWeaver::Job()
        , queryMakerInternal( queryMaker )
        , m_abort( false )
    {
        DEBUG_BLOCK
        //nothing to do
        debug() << "in nepomukqueryjob";
    }

    void requestAbort()
    {
        m_abort = true;
    }

    ~NepomukQueryJob()
    {
        delete queryMakerInternal;
    }

protected:
    void run()
    {
        if( !m_abort )
        {
            queryMakerInternal->runQuery();
        }
        setFinished( true );
    }

public:
    NepomukQueryMakerInternal *queryMakerInternal;
private:
    bool m_abort;
};


// inspired by Private of MemoryQueryMaker
struct NepomukQueryMaker::Private
{
    QueryMaker::QueryType type;
    bool returnDataPtrs;
    //    MemoryMatcher* matcher;
    NepomukQueryJob *job;
    int maxsize;
    QStack<ContainerMemoryFilter*> containerFilters;
    QList<CustomReturnFunction*> returnFunctions;
    QList<CustomReturnValue*> returnValues;
    bool usingFilters;
    KRandomSequence sequence;   //do not reset
    qint64 orderByField;
    bool orderDescending;
    bool orderByNumberField;
    AlbumQueryMode albumQueryMode;
    ArtistQueryMode artistQueryMode;
    LabelQueryMode labelQueryMode;
    QString collectionId;
};


NepomukQueryMaker::NepomukQueryMaker( NepomukCollection *collection )
    : QueryMaker()
    , m_collection( collection )
    , d( new Private )
{
    debug() << "in nepomukquerymaker constructor";

    d->collectionId = m_collection->collectionId();
//    d->matcher = 0;
    d->job = 0;
    d->type = QueryMaker::None;
    d->returnDataPtrs = false;
    d->maxsize = -1;
    d->containerFilters.push( new AndContainerMemoryFilter() );
    d->usingFilters = false;
    d->orderByField = 0;
    d->orderDescending = false;
    d->orderByNumberField = false;
    d->albumQueryMode = AllAlbums;
    d->artistQueryMode = TrackArtists;
    d->labelQueryMode = QueryMaker::NoConstraint;

}

NepomukQueryMaker::~NepomukQueryMaker()
{
    DEBUG_BLOCK
    disconnect();
    abortQuery();
    if( !d->containerFilters.isEmpty() )
        delete d->containerFilters.first();
    delete d;
}

void
NepomukQueryMaker::run()
{
    if( d->type == QueryMaker::None )
        //TODO error handling
        return;
    else if( d->job && !d->job->isFinished() )
    {
        //the worker thread seems to be running
        //TODO: wait or job to complete
    }
    else
    {
        NepomukQueryMakerInternal *qmi = new NepomukQueryMakerInternal( m_collection );

        if( d->usingFilters )
        {
            qmi->setFilters( d->containerFilters.first() );
            d->containerFilters.clear(); //will be deleted by MemoryQueryMakerInternal
        }

        //TODO
        // set matchers here after implementation of them in nqmi and nqm

        qmi->setMaxSize( d->maxsize );
        qmi->setType( d->type );

        qmi->setCustomReturnFunctions( d->returnFunctions );
        d->returnFunctions.clear(); //will be deleted by NepomukQueryMakerInternal
        qmi->setCustomReturnValues( d->returnValues );
        d->returnValues.clear(); //will be deleted by NepomukQueryMakerInternal

        qmi->setAlbumQueryMode( d->albumQueryMode );
        qmi->setArtistQueryMode( d->artistQueryMode );
        qmi->setLabelQueryMode( d->labelQueryMode );
        qmi->setOrderDescending( d->orderDescending );
        qmi->setOrderByNumberField( d->orderByNumberField );
        qmi->setOrderByField( d->orderByField );
        qmi->setCollectionId( d->collectionId );

        connect( qmi, SIGNAL( newResultReady( Meta::AlbumList ) ),
                 SIGNAL( newResultReady( Meta::AlbumList ) ), Qt::DirectConnection );
        connect( qmi, SIGNAL( newResultReady( Meta::ArtistList ) ),
                 SIGNAL( newResultReady( Meta::ArtistList ) ), Qt::DirectConnection );
        connect( qmi, SIGNAL( newResultReady( Meta::GenreList ) ),
                 SIGNAL( newResultReady( Meta::GenreList ) ), Qt::DirectConnection );
        connect( qmi, SIGNAL( newResultReady( Meta::ComposerList ) ),
                 SIGNAL( newResultReady( Meta::ComposerList ) ), Qt::DirectConnection );
        connect( qmi, SIGNAL( newResultReady( Meta::YearList ) ),
                 SIGNAL( newResultReady( Meta::YearList ) ), Qt::DirectConnection );
        connect( qmi, SIGNAL( newResultReady( Meta::TrackList ) ),
                 SIGNAL( newResultReady( Meta::TrackList ) ), Qt::DirectConnection );
        connect( qmi, SIGNAL( newResultReady( QStringList ) ),
                 SIGNAL( newResultReady( QStringList ) ), Qt::DirectConnection );
        connect( qmi, SIGNAL( newResultReady( Meta::LabelList ) ),
                 SIGNAL( newResultReady( Meta::LabelList ) ), Qt::DirectConnection );

        d->job = new NepomukQueryJob( qmi );
        connect( d->job, SIGNAL( done( ThreadWeaver::Job * ) ), SLOT( done( ThreadWeaver::Job * ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( d->job );
    }
}

void
NepomukQueryMaker::abortQuery()
{
    DEBUG_BLOCK
    if( d->job )
    {
        d->job->requestAbort();
        d->job->disconnect( this );
        if( d->job->queryMakerInternal )
            d->job->queryMakerInternal->disconnect( this );
    }
}

QueryMaker*
NepomukQueryMaker::setQueryType( QueryType type )
{
    DEBUG_BLOCK

    debug() << "setting queryType :" << type;

    switch( type )
    {
    case QueryMaker::Track:
        if( d->type == QueryMaker::None )
            d->type = QueryMaker::Track;
        return this;

    case QueryMaker::Artist:
        if( d->type == QueryMaker::None )
            d->type = QueryMaker::Artist;
        return this;

    case QueryMaker::Album:
        if( d->type == QueryMaker::None )
            d->type = QueryMaker::Album;
        return this;

    case QueryMaker::AlbumArtist:
        if( d->type == QueryMaker::None )
            d->type = QueryMaker::AlbumArtist;
        return this;

    case QueryMaker::Composer:
        if( d->type == QueryMaker::None )
            d->type = QueryMaker::Composer;
        return this;

    case QueryMaker::Genre:
        if( d->type == QueryMaker::None )
            d->type = QueryMaker::Genre;
        return this;

    case QueryMaker::Year:
        if( d->type == QueryMaker::None )
            d->type = QueryMaker::Year;
        return this;

    case QueryMaker::Custom:
        if( d->type == QueryMaker::None )
            d->type = QueryMaker::Custom;
        return this;

    case QueryMaker::Label:
        if( d->type == QueryMaker::None )
            d->type = QueryMaker::Label;
        return this;

    case QueryMaker::None:
        return this;
    }
    return this;
}

QueryMaker*
NepomukQueryMaker::addReturnValue( qint64 value )
{
    //QM can not deliver sensible results if both a custom return value and a return
    //function is selected
    if( d->returnFunctions.empty() )
    {
        CustomReturnValue *returnValue = CustomValueFactory::returnValue( value );
        if( returnValue )
            d->returnValues.append( returnValue );
    }
    return this;
}

QueryMaker*
NepomukQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    //QM can not deliver sensible results if both a custom return value and a return
    //function is selected
    if( d->returnValues.empty() )
    {
        CustomReturnFunction *returnFunction =
            CustomValueFactory::returnFunction( function, value );
        if( returnFunction )
            d->returnFunctions.append( returnFunction );
    }
    return this;
}

QueryMaker*
NepomukQueryMaker::orderBy( qint64 value, bool descending )
{
    DEBUG_BLOCK
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
    //TODO: what about Meta::valFirstPlayed, Meta::valCreateDate or
    //Meta::valLastPlayed??

    default:
        d->orderByNumberField = false;
    }
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK
    debug() << "unimplemented";
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::ArtistPtr &artist )
{
    DEBUG_BLOCK
    debug() << "unimplemented";
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
    DEBUG_BLOCK
    debug() << "unimplemented";
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
    DEBUG_BLOCK
    debug() << "unimplemented";
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    DEBUG_BLOCK
    debug() << "unimplemented";
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::YearPtr &year )
{
    DEBUG_BLOCK
    debug() << "unimplemented";
    return this;
}

QueryMaker*
NepomukQueryMaker::addMatch( const Meta::LabelPtr &label )
{
    DEBUG_BLOCK
    debug() << "unimplemented";
    return this;
}

QueryMaker*
NepomukQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin,
                              bool matchEnd )
{
    DEBUG_BLOCK
    d->containerFilters.top()->addFilter(
        FilterFactory::filter( value, filter, matchBegin, matchEnd ) );
    d->usingFilters = true;
    return this;
}

QueryMaker*
NepomukQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin,
                                  bool matchEnd )
{
    DEBUG_BLOCK
    MemoryFilter *tmp = FilterFactory::filter( value, filter, matchBegin, matchEnd );
    d->containerFilters.top()->addFilter( new NegateMemoryFilter( tmp ) );
    d->usingFilters = true;
    return this;
}

QueryMaker*
NepomukQueryMaker::addNumberFilter( qint64 value, qint64 filter,
                                    NumberComparison compare )
{
    DEBUG_BLOCK
    d->containerFilters.top()->addFilter(
        FilterFactory::numberFilter( value, filter, compare ) );
    d->usingFilters = true;
    return this;
}

QueryMaker*
NepomukQueryMaker::excludeNumberFilter( qint64 value, qint64 filter,
                                        NumberComparison compare )
{
    DEBUG_BLOCK
    MemoryFilter *tmp = FilterFactory::numberFilter( value, filter, compare );
    d->containerFilters.top()->addFilter( new NegateMemoryFilter( tmp ) );
    d->usingFilters = true;
    return this;
}


QueryMaker*
NepomukQueryMaker::limitMaxResultSize( int size )
{
    DEBUG_BLOCK
    d->maxsize = size;
    return this;
}

QueryMaker*
NepomukQueryMaker::beginAnd()
{
    DEBUG_BLOCK
    ContainerMemoryFilter *filter = new AndContainerMemoryFilter();
    d->containerFilters.top()->addFilter( filter );
    d->containerFilters.push( filter );
    return this;
}

QueryMaker*
NepomukQueryMaker::beginOr()
{
    DEBUG_BLOCK
    ContainerMemoryFilter *filter = new OrContainerMemoryFilter();
    d->containerFilters.top()->addFilter( filter );
    d->containerFilters.push( filter );
    return this;
}

QueryMaker*
NepomukQueryMaker::endAndOr()
{
    DEBUG_BLOCK
    d->containerFilters.pop();
    return this;
}

void
NepomukQueryMaker::done( ThreadWeaver::Job *job )
{
    DEBUG_BLOCK
    ThreadWeaver::Weaver::instance()->dequeue( job );
    job->deleteLater();
    d->job = 0;
    emit queryDone();
}

QueryMaker*
NepomukQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    DEBUG_BLOCK
    d->albumQueryMode = mode;
    return this;
}

QueryMaker*
NepomukQueryMaker::setArtistQueryMode( QueryMaker::ArtistQueryMode mode )
{
    DEBUG_BLOCK
    d->artistQueryMode = mode;
    return this;
}

QueryMaker*
NepomukQueryMaker::setLabelQueryMode( LabelQueryMode mode )
{
    DEBUG_BLOCK
    d->labelQueryMode = mode;
    return this;
}
