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

#include <ThreadWeaver/Queue>

#include <QList>
#include <QSet>
#include <QStack>
#include <QtAlgorithms>

using namespace Collections;

//QueryJob

class QueryJob : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        QueryJob( MemoryQueryMakerInternal *qmInternal )
            : QObject()
            , ThreadWeaver::Job()
            , queryMakerInternal( qmInternal )
        {
            //nothing to do
        }

        ~QueryJob() override
        {
            delete queryMakerInternal;
        }

    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override
        {
            Q_EMIT started(self);
            ThreadWeaver::Job::defaultBegin(self, thread);
        }

        void defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override
        {
            ThreadWeaver::Job::defaultEnd(self, thread);
            if (!self->success()) {
                Q_EMIT failed(self);
            }
            Q_EMIT done(self);
        }
        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread=nullptr) override
        {
            Q_UNUSED(self);
            Q_UNUSED(thread);
            queryMakerInternal->runQuery();
//            setFinished( true );
            setStatus(Status_Success);
        }

    public:
        MemoryQueryMakerInternal *queryMakerInternal;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

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
    qint64 orderByField;
    bool orderDescending;
    bool orderByNumberField;
    AlbumQueryMode albumQueryMode;
    LabelQueryMode labelQueryMode;
    QString collectionId;
};

MemoryQueryMaker::MemoryQueryMaker( const QWeakPointer<MemoryCollection> &mc, const QString &collectionId )
    : QueryMaker()
    , m_collection( mc )
    , d( new Private )
{
    d->collectionId = collectionId;
    d->matcher = nullptr;
    d->job = nullptr;
    d->type = QueryMaker::None;
    d->returnDataPtrs = false;
    d->job = nullptr;
    d->job = nullptr;
    d->maxsize = -1;
    d->containerFilters.push( new AndContainerMemoryFilter() );
    d->usingFilters = false;
    d->orderByField = 0;
    d->orderDescending = false;
    d->orderByNumberField = false;
    d->albumQueryMode = AllAlbums;
    d->labelQueryMode = QueryMaker::NoConstraint;
}

MemoryQueryMaker::~MemoryQueryMaker()
{
    disconnect();
    abortQuery();
    if( !d->containerFilters.isEmpty() )
        delete d->containerFilters.first();
    delete d;
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
        d->matcher = nullptr; //will be deleted by MemoryQueryMakerInternal
        qmi->setMaxSize( d->maxsize );
        qmi->setType( d->type );
        qmi->setCustomReturnFunctions( d->returnFunctions );
        d->returnFunctions.clear(); //will be deleted by MemoryQueryMakerInternal
        qmi->setCustomReturnValues( d->returnValues );
        d->returnValues.clear(); //will be deleted by MemoryQueryMakerInternal
        qmi->setAlbumQueryMode( d->albumQueryMode );
        qmi->setLabelQueryMode( d->labelQueryMode );
        qmi->setOrderDescending( d->orderDescending );
        qmi->setOrderByNumberField( d->orderByNumberField );
        qmi->setOrderByField( d->orderByField );
        qmi->setCollectionId( d->collectionId );

        connect( qmi, &Collections::MemoryQueryMakerInternal::newAlbumsReady, this, &MemoryQueryMaker::newAlbumsReady, Qt::DirectConnection );
        connect( qmi, &Collections::MemoryQueryMakerInternal::newArtistsReady, this, &MemoryQueryMaker::newArtistsReady, Qt::DirectConnection );
        connect( qmi, &Collections::MemoryQueryMakerInternal::newGenresReady, this, &MemoryQueryMaker::newGenresReady, Qt::DirectConnection );
        connect( qmi, &Collections::MemoryQueryMakerInternal::newComposersReady, this, &MemoryQueryMaker::newComposersReady, Qt::DirectConnection );
        connect( qmi, &Collections::MemoryQueryMakerInternal::newYearsReady, this, &MemoryQueryMaker::newYearsReady, Qt::DirectConnection );
        connect( qmi, &Collections::MemoryQueryMakerInternal::newTracksReady, this, &MemoryQueryMaker::newTracksReady, Qt::DirectConnection );
        connect( qmi, &Collections::MemoryQueryMakerInternal::newResultReady, this, &MemoryQueryMaker::newResultReady, Qt::DirectConnection );
        connect( qmi, &Collections::MemoryQueryMakerInternal::newLabelsReady, this, &MemoryQueryMaker::newLabelsReady, Qt::DirectConnection );

        d->job = new QueryJob( qmi );
        connect( d->job, &QueryJob::done, this, &MemoryQueryMaker::done );
        ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(d->job) );
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

    case QueryMaker::AlbumArtist:
        if ( d->type == QueryMaker::None )
            d->type = QueryMaker::AlbumArtist;
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
MemoryQueryMaker::addMatch( const Meta::TrackPtr &track )
{
    MemoryMatcher *trackMatcher = new TrackMatcher( track );
    if ( d->matcher == nullptr )
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
MemoryQueryMaker::addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour )
{
    MemoryMatcher *artistMatcher = new ArtistMatcher( artist, behaviour );
    if ( d->matcher == nullptr )
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
    if ( d->matcher == nullptr )
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
    if ( d->matcher == nullptr )
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
    if ( d->matcher == nullptr )
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
    if ( d->matcher == nullptr )
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
MemoryQueryMaker::addMatch( const Meta::LabelPtr &label )
{
    MemoryMatcher *labelMatcher = new LabelMatcher( label );
    if( d->matcher == nullptr )
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
MemoryQueryMaker::done( ThreadWeaver::JobPointer job )
{
    ThreadWeaver::Queue::instance()->dequeue( job );
    d->job = nullptr;
    Q_EMIT queryDone();
}

QueryMaker * MemoryQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    d->albumQueryMode = mode;
    return this;
}

QueryMaker*
MemoryQueryMaker::setLabelQueryMode( LabelQueryMode mode )
{
    d->labelQueryMode = mode;
    return this;
}

#include "MemoryQueryMaker.moc"
