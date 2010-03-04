/****************************************************************************************
 * Copyright (c) 2007-2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2007-2009 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#include "MemoryQueryMaker.h"
#include "MemoryCustomValue.h"
#include "MemoryFilter.h"
#include "MemoryMatcher.h"
#include "MemoryQueryMakerHelper.h"
#include "MemoryQueryMakerInternal.h"
#include "core/support/Debug.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QList>
#include <QSet>
#include <QStack>
#include <QtAlgorithms>

#include <KRandomSequence>
#include <KSortableList>

using namespace Collections;

//QueryJob

class QueryJob : public ThreadWeaver::Job
{
    public:
        QueryJob( MemoryQueryMakerInternal *qmInternal )
            : ThreadWeaver::Job()
            , queryMakerInternal( qmInternal )
        {
            //nothing to do
        }

        ~QueryJob()
        {
            delete queryMakerInternal;
        }

    protected:
        void run()
        {
            queryMakerInternal->runQuery();
            setFinished( true );
        }

    public:
        MemoryQueryMakerInternal *queryMakerInternal;
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
    QString collectionId;
};

MemoryQueryMaker::MemoryQueryMaker( QWeakPointer<MemoryCollection> mc, const QString &collectionId )
    : QueryMaker()
    , m_collection( mc )
    , d( new Private )
{
    d->collectionId = collectionId;
    d->matcher = 0;
    d->job = 0;
    reset();
}

MemoryQueryMaker::~MemoryQueryMaker()
{
    disconnect();
    abortQuery();
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
        MemoryQueryMakerInternal *qmi = new MemoryQueryMakerInternal( m_collection );
        if( d->usingFilters )
        {
            qmi->setFilters( d->containerFilters.first() );
            d->containerFilters.clear(); //will be deleted by MemoryQueryMakerInternal
        }
        qmi->setMatchers( d->matcher );
        d->matcher = 0; //will be deleted by MemoryQueryMakerInternal
        qmi->setRandomize( d->randomize );
        qmi->setMaxSize( d->maxsize );
        qmi->setReturnAsDataPtrs( d->returnDataPtrs );
        qmi->setType( d->type );
        qmi->setCustomReturnFunctions( d->returnFunctions );
        d->returnFunctions.clear(); //will be deleted by MemoryQueryMakerInternal
        qmi->setCustomReturnValues( d->returnValues );
        d->returnValues.clear(); //will be deleted by MemoryQueryMakerInternal
        qmi->setAlbumQueryMode( d->albumQueryMode );
        qmi->setOrderDescending( d->orderDescending );
        qmi->setOrderByNumberField( d->orderByNumberField );
        qmi->setOrderByField( d->orderByField );
        qmi->setCollectionId( d->collectionId );

        connect( qmi, SIGNAL(newResultReady(QString,Meta::AlbumList)), SIGNAL(newResultReady(QString,Meta::AlbumList)), Qt::DirectConnection );
        connect( qmi, SIGNAL(newResultReady(QString,Meta::ArtistList)), SIGNAL(newResultReady(QString,Meta::ArtistList)), Qt::DirectConnection );
        connect( qmi, SIGNAL(newResultReady(QString,Meta::GenreList)), SIGNAL(newResultReady(QString,Meta::GenreList)), Qt::DirectConnection );
        connect( qmi, SIGNAL(newResultReady(QString,Meta::ComposerList)), SIGNAL(newResultReady(QString,Meta::ComposerList)), Qt::DirectConnection );
        connect( qmi, SIGNAL(newResultReady(QString,Meta::YearList)), SIGNAL(newResultReady(QString,Meta::YearList)), Qt::DirectConnection );
        connect( qmi, SIGNAL(newResultReady(QString,Meta::TrackList)), SIGNAL(newResultReady(QString,Meta::TrackList)), Qt::DirectConnection );
        connect( qmi, SIGNAL(newResultReady(QString,Meta::DataList)), SIGNAL(newResultReady(QString,Meta::DataList)), Qt::DirectConnection );
        connect( qmi, SIGNAL(newResultReady(QString,QStringList)), SIGNAL(newResultReady(QString,QStringList)), Qt::DirectConnection );
        connect( qmi, SIGNAL(newResultReady(QString,Meta::LabelList)), SIGNAL(newResultReady(QString,Meta::LabelList)), Qt::DirectConnection );

        d->job = new QueryJob( qmi );
        connect( d->job, SIGNAL( done( ThreadWeaver::Job * ) ), SLOT( done( ThreadWeaver::Job * ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( d->job );
    }
}

void
MemoryQueryMaker::abortQuery()
{
    if( d->job )
    {
        d->job->requestAbort();
        d->job->disconnect( this );
        if( d->job->queryMakerInternal )
            d->job->queryMakerInternal->disconnect( this );
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
MemoryQueryMaker::addMatch( const Meta::TrackPtr &track )
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
MemoryQueryMaker::addMatch( const Meta::ArtistPtr &artist )
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
MemoryQueryMaker::addMatch( const Meta::AlbumPtr &album )
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
MemoryQueryMaker::addMatch( const Meta::GenrePtr &genre )
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
MemoryQueryMaker::addMatch( const Meta::ComposerPtr &composer )
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
MemoryQueryMaker::addMatch( const Meta::YearPtr &year )
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
MemoryQueryMaker::addMatch( const Meta::DataPtr &data )
{
    ( const_cast<Meta::DataPtr&>(data) )->addMatchTo( this );
    return this;
}

QueryMaker*
MemoryQueryMaker::addMatch( const Meta::LabelPtr &label )
{
    MemoryMatcher *labelMatcher = new LabelMatcher( label );
    if( d->matcher == 0 )
    {
        d->matcher = labelMatcher;
    }
    else
    {
        MemoryMatcher *tmp = d->matcher;
        while( !tmp->isLast() )
        {
            tmp = tmp->next();
        }
        tmp->setNext( labelMatcher );
    }
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
