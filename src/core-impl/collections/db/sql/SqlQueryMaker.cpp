/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
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

#define DEBUG_PREFIX "SqlQueryMaker"

#include "SqlQueryMaker.h"

#include "SqlCollection.h"
#include "SqlQueryMakerInternal.h"
#include <core/storage/SqlStorage.h>
#include "core/support/Debug.h"
#include "core-impl/collections/db/MountPointManager.h"

#include <QWeakPointer>
#include <QStack>

#include <ThreadWeaver/Job>
#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Queue>

using namespace Collections;

class SqlWorkerThread : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        SqlWorkerThread( SqlQueryMakerInternal *queryMakerInternal )
            : QObject()
            , ThreadWeaver::Job()
            , m_queryMakerInternal( queryMakerInternal )
            , m_aborted( false )
        {
            //nothing to do
        }

        ~SqlWorkerThread() override
        {
            delete m_queryMakerInternal;
        }

        void deleteWhenReady()
        {
            // If the thread is deleted before completion, some flaky segfaults are encountered e.g. when defaultEnd of
            // an already deleted thread is called. This fixes at least TestSqlQueryMaker::testDeleteQueryMakerWithRunningQuery,
            // and might actually fix real-world crashes, too.
            if( status() == Status_Queued || status() == Status_Running )
                connect(this, &SqlWorkerThread::done, this, &SqlWorkerThread::deleteLater);
            else
                deleteLater();
        }

        void requestAbort() override
        {
            m_aborted = true;
            m_queryMakerInternal->requestAbort();
        }

        SqlQueryMakerInternal* queryMakerInternal() const
        {
            return m_queryMakerInternal;
        }

    protected:
        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = nullptr) override
        {
            Q_UNUSED(self);
            Q_UNUSED(thread);
            m_queryMakerInternal->run();
            if( m_aborted )
                setStatus(Status_Aborted);
            else
                setStatus(Status_Running);
        }
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

    private:
        SqlQueryMakerInternal *m_queryMakerInternal;

        bool m_aborted;
    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);
};

struct SqlQueryMaker::Private
{
    enum { TAGS_TAB = 1, ARTIST_TAB = 2, ALBUM_TAB = 4, GENRE_TAB = 8, COMPOSER_TAB = 16, YEAR_TAB = 32, STATISTICS_TAB = 64, URLS_TAB = 128, ALBUMARTIST_TAB = 256, LABELS_TAB = 1024 };
    int linkedTables;
    QueryMaker::QueryType queryType;
    QString query;
    QString queryReturnValues;
    QString queryFrom;
    QString queryMatch;
    QString queryFilter;
    QString queryOrderBy;
    bool withoutDuplicates;
    int maxResultSize;
    AlbumQueryMode albumMode;
    LabelQueryMode labelMode;
    SqlWorkerThread *worker;

    QStack<bool> andStack;

    QStringList blockingCustomData;
    Meta::TrackList blockingTracks;
    Meta::AlbumList blockingAlbums;
    Meta::ArtistList blockingArtists;
    Meta::GenreList blockingGenres;
    Meta::ComposerList blockingComposers;
    Meta::YearList blockingYears;
    Meta::LabelList blockingLabels;
    bool blocking;
    bool used;
    qint64 returnValueType;
};

SqlQueryMaker::SqlQueryMaker( SqlCollection* collection )
    : QueryMaker()
    , m_collection( collection )
    , d( new Private )
{
    d->worker = nullptr;
    d->queryType = QueryMaker::None;
    d->linkedTables = 0;
    d->withoutDuplicates = false;
    d->albumMode = AllAlbums;
    d->labelMode = QueryMaker::NoConstraint;
    d->maxResultSize = -1;
    d->andStack.clear();
    d->andStack.push( true );   //and is default
    d->blocking = false;
    d->used = false;
    d->returnValueType = 0;
}

SqlQueryMaker::~SqlQueryMaker()
{
    disconnect();
    abortQuery();
    if( d->worker )
    {
        d->worker->deleteWhenReady();
        d->worker = nullptr;
    }
    delete d;
}

void
SqlQueryMaker::abortQuery()
{
    if( d->worker )
    {
        d->worker->requestAbort();
        d->worker->disconnect( this );
        if( d->worker->queryMakerInternal() )
            d->worker->queryMakerInternal()->disconnect( this );
    }
}

void
SqlQueryMaker::run()
{
    if( d->queryType == QueryMaker::None || (d->blocking && d->used) )
    {
        debug() << "sql querymaker used without reset or initialization" << Qt::endl;
        return; //better error handling?
    }
    if( d->worker && !d->worker->isFinished() )
    {
        //the worker thread seems to be running
        //TODO: wait or job to complete

    }
    else
    {
        SqlQueryMakerInternal *qmi = new SqlQueryMakerInternal( m_collection );
        qmi->setQuery( query() );
        qmi->setQueryType( d->queryType );

        if ( !d->blocking )
        {
            connect( qmi, &Collections::SqlQueryMakerInternal::newAlbumsReady, this, &SqlQueryMaker::newAlbumsReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newArtistsReady, this, &SqlQueryMaker::newArtistsReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newGenresReady, this, &SqlQueryMaker::newGenresReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newComposersReady, this, &SqlQueryMaker::newComposersReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newYearsReady, this, &SqlQueryMaker::newYearsReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newTracksReady, this, &SqlQueryMaker::newTracksReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newResultReady, this, &SqlQueryMaker::newResultReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newLabelsReady, this, &SqlQueryMaker::newLabelsReady, Qt::DirectConnection );
            d->worker = new SqlWorkerThread( qmi );
            connect( d->worker, &SqlWorkerThread::done, this, &SqlQueryMaker::done );
            ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(d->worker) );
        }
        else //use it blocking
        {
            connect( qmi, &Collections::SqlQueryMakerInternal::newAlbumsReady, this, &SqlQueryMaker::blockingNewAlbumsReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newArtistsReady, this, &SqlQueryMaker::blockingNewArtistsReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newGenresReady, this, &SqlQueryMaker::blockingNewGenresReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newComposersReady, this, &SqlQueryMaker::blockingNewComposersReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newYearsReady, this, &SqlQueryMaker::blockingNewYearsReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newTracksReady, this, &SqlQueryMaker::blockingNewTracksReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newResultReady, this, &SqlQueryMaker::blockingNewResultReady, Qt::DirectConnection );
            connect( qmi, &Collections::SqlQueryMakerInternal::newLabelsReady, this, &SqlQueryMaker::blockingNewLabelsReady, Qt::DirectConnection );
            qmi->run();
            delete qmi;
        }
    }
    d->used = true;
}

void
SqlQueryMaker::done( ThreadWeaver::JobPointer job )
{
    Q_UNUSED( job )

    d->worker = nullptr; // d->worker *is* the job, prevent stale pointer
    Q_EMIT queryDone();
}

QueryMaker*
SqlQueryMaker::setQueryType( QueryType type )
{
    // we need the unchanged m_queryType in the blocking result methods so prevent
    // reseting queryType without reseting the QM
    if ( d->blocking && d->used )
        return this;

    switch( type ) {
    case QueryMaker::Track:
        //make sure to keep this method in sync with handleTracks(QStringList) and the SqlTrack ctor
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Track;
            d->linkedTables |= Private::URLS_TAB;
            d->linkedTables |= Private::TAGS_TAB;
            d->linkedTables |= Private::GENRE_TAB;
            d->linkedTables |= Private::ARTIST_TAB;
            d->linkedTables |= Private::ALBUM_TAB;
            d->linkedTables |= Private::COMPOSER_TAB;
            d->linkedTables |= Private::YEAR_TAB;
            d->linkedTables |= Private::STATISTICS_TAB;
            d->queryReturnValues = Meta::SqlTrack::getTrackReturnValues();
        }
        return this;

    case QueryMaker::Artist:
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Artist;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::ARTIST_TAB;
            //reading the ids from the database means we don't have to query for them later
            d->queryReturnValues = QStringLiteral("artists.name, artists.id");
        }
        return this;

    case QueryMaker::Album:
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Album;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::ALBUM_TAB;
            //add whatever is necessary to identify compilations
            d->queryReturnValues = QStringLiteral("albums.name, albums.id, albums.artist");
        }
        return this;

    case QueryMaker::AlbumArtist:
      if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::AlbumArtist;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::ALBUMARTIST_TAB;
            d->linkedTables |= Private::ALBUM_TAB;
            d->queryReturnValues = QStringLiteral("albumartists.name, albumartists.id");
        }
        return this;

    case QueryMaker::Composer:
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Composer;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::COMPOSER_TAB;
            d->queryReturnValues = QStringLiteral("composers.name, composers.id");
        }
        return this;

    case QueryMaker::Genre:
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Genre;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::GENRE_TAB;
            d->queryReturnValues = QStringLiteral("genres.name, genres.id");
        }
        return this;

    case QueryMaker::Year:
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Year;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::YEAR_TAB;
            d->queryReturnValues = QStringLiteral("years.name, years.id");
        }
        return this;

    case QueryMaker::Custom:
        if( d->queryType == QueryMaker::None )
            d->queryType = QueryMaker::Custom;
        return this;

    case QueryMaker::Label:
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Label;
            d->withoutDuplicates = true;
            d->queryReturnValues = QStringLiteral("labels.label,labels.id");
            d->linkedTables |= Private::LABELS_TAB;
        }
        return this;

    case QueryMaker::None:
        return this;
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::TrackPtr &track )
{
    QString url = track->uidUrl();
    if( !url.isEmpty() )
    /*
    QUrl kurl( url );
    if( kurl.scheme() == "amarok-sqltrackuid" )
    */
    {
        d->queryMatch += QStringLiteral( " AND urls.uniqueid = '%1' " ).arg( url /*kurl.url()*/ );
    }
    else
    {
        QString path;
        /*
        if( kurl.isLocalFile() )
        {
            path = kurl.path();
        }
        else
        */
        {
            path = track->playableUrl().path();
        }
        int deviceid = m_collection->mountPointManager()->getIdForUrl( QUrl::fromUserInput(path) );
        QString rpath = m_collection->mountPointManager()->getRelativePath( deviceid, path );
        d->queryMatch += QStringLiteral( " AND urls.deviceid = %1 AND urls.rpath = '%2'" )
                        .arg( QString::number( deviceid ), escape( rpath ) );
    }
    return this;
}


QueryMaker*
SqlQueryMaker::addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour )
{
    d->linkedTables |= Private::ARTIST_TAB;
    if( behaviour == AlbumArtists || behaviour == AlbumOrTrackArtists )
        d->linkedTables |= Private::ALBUMARTIST_TAB;

    QString artistQuery;
    QString albumArtistQuery;

    if( artist && !artist->name().isEmpty() )
    {
        artistQuery = QStringLiteral("artists.name = '%1'").arg( escape( artist->name() ) );
        albumArtistQuery = QStringLiteral("albumartists.name = '%1'").arg( escape( artist->name() ) );
    }
    else
    {
        artistQuery = QStringLiteral("( artists.name IS NULL OR artists.name = '')");
        albumArtistQuery = QStringLiteral("( albumartists.name IS NULL OR albumartists.name = '')");
    }

    switch( behaviour )
    {
    case TrackArtists:
        d->queryMatch += QStringLiteral(" AND ") + artistQuery;
        break;
    case AlbumArtists:
        d->queryMatch += QStringLiteral(" AND ") + albumArtistQuery;
        break;
    case AlbumOrTrackArtists:
        d->queryMatch += QStringLiteral(" AND ( (") + artistQuery + QStringLiteral(" ) OR ( ") + albumArtistQuery + QStringLiteral(" ) )");
        break;
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
    d->linkedTables |= Private::ALBUM_TAB;

    // handle singles
    if( !album || album->name().isEmpty() )
        d->queryMatch += QStringLiteral( " AND ( albums.name IS NULL OR albums.name = '' )" );
    else
        d->queryMatch += QStringLiteral( " AND albums.name = '%1'" ).arg( escape( album->name() ) );

    if( album )
    {
        //handle compilations
        Meta::ArtistPtr albumArtist = album->albumArtist();
        if( albumArtist )
        {
            d->linkedTables |= Private::ALBUMARTIST_TAB;
            d->queryMatch += QStringLiteral( " AND albumartists.name = '%1'" ).arg( escape( albumArtist->name() ) );
        }
        else
        {
            d->queryMatch += QStringLiteral(" AND albums.artist IS NULL");
        }
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    d->linkedTables |= Private::GENRE_TAB;
    d->queryMatch += QStringLiteral( " AND genres.name = '%1'" ).arg( escape( genre->name() ) );
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
    d->linkedTables |= Private::COMPOSER_TAB;
    d->queryMatch += QStringLiteral( " AND composers.name = '%1'" ).arg( escape( composer->name() ) );
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::YearPtr &year )
{
    // handle tracks without a year
    if( !year )
    {
        d->queryMatch += QStringLiteral(" AND year IS NULL");
    }
    else
    {
        d->linkedTables |= Private::YEAR_TAB;
        d->queryMatch += QStringLiteral( " AND years.name = '%1'" ).arg( escape( year->name() ) );
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::LabelPtr &label )
{
    AmarokSharedPointer<Meta::SqlLabel> sqlLabel = AmarokSharedPointer<Meta::SqlLabel>::dynamicCast( label );
    QString labelSubQuery;
    if( sqlLabel )
    {
        labelSubQuery = QStringLiteral("SELECT url FROM urls_labels WHERE label = %1");
        labelSubQuery = labelSubQuery.arg( sqlLabel->id() );
    }
    else
    {
        labelSubQuery = QStringLiteral("SELECT a.url FROM urls_labels a INNER JOIN labels b ON a.label = b.id WHERE b.label = '%1'");
        labelSubQuery = labelSubQuery.arg( escape( label->name() ) );
    }
    d->linkedTables |= Private::TAGS_TAB;
    QString match = QStringLiteral(" AND tracks.url in (%1)");
    d->queryMatch += match.arg( labelSubQuery );
    return this;
}

QueryMaker*
SqlQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    // special case for albumartist...
    if( value == Meta::valAlbumArtist && filter.isEmpty() )
    {
        d->linkedTables |= Private::ALBUMARTIST_TAB;
        d->linkedTables |= Private::ALBUM_TAB;
        d->queryFilter += QStringLiteral( " %1 ( albums.artist IS NULL or albumartists.name = '') " ).arg( andOr() );
    }
    else if( value == Meta::valLabel )
    {
        d->linkedTables |= Private::TAGS_TAB;
        QString like = likeCondition( filter, !matchBegin, !matchEnd );
        QString filter = QStringLiteral(" %1 tracks.url IN (SELECT a.url FROM urls_labels a INNER JOIN labels b ON a.label = b.id WHERE b.label %2) ");
        d->queryFilter += filter.arg( andOr(), like );
    }
    else
    {
        QString like = likeCondition( filter, !matchBegin, !matchEnd );
        d->queryFilter += QStringLiteral( " %1 %2 %3 " ).arg( andOr(), nameForValue( value ), like );
    }
    return this;
}

QueryMaker*
SqlQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    // special case for album...
    if( value == Meta::valAlbumArtist && filter.isEmpty() )
    {
        d->linkedTables |= Private::ALBUMARTIST_TAB;
        d->queryFilter += QStringLiteral( " %1 NOT ( albums.artist IS NULL or albumartists.name = '') " ).arg( andOr() );
    }
    else if( value == Meta::valLabel )
    {
        d->linkedTables |= Private::TAGS_TAB;
        QString like = likeCondition( filter, !matchBegin, !matchEnd );
        QString filter = QStringLiteral(" %1 tracks.url NOT IN (SELECT a.url FROM urls_labels a INNER JOIN labels b ON a.label = b.id WHERE b.label %2) ");
        d->queryFilter += filter.arg( andOr(), like );
    }
    else
    {
        QString like = likeCondition( filter, !matchBegin, !matchEnd );
        d->queryFilter += QStringLiteral( " %1 NOT %2 %3 " ).arg( andOr(), nameForValue( value ), like );
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    QString comparison;
    switch( compare )
    {
        case QueryMaker::Equals:
            comparison = QLatin1Char('=');
            break;
        case QueryMaker::GreaterThan:
            comparison = QLatin1Char('>');
            break;
        case QueryMaker::LessThan:
            comparison = QLatin1Char('<');
            break;
    }

    // note: a NULL value in the database means undefined and not 0!
    d->queryFilter += QStringLiteral( " %1 %2 %3 %4 " ).arg( andOr(), nameForValue( value ), comparison, QString::number( filter ) );

    return this;
}

QueryMaker*
SqlQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    QString comparison;
    switch( compare )
    {
        case QueryMaker::Equals:
            comparison = QStringLiteral("!=");
            break;
        case QueryMaker::GreaterThan:   //negating greater than is less or equal
            comparison = QStringLiteral("<=");
            break;
        case QueryMaker::LessThan:      //negating less than is greater or equal
            comparison = QStringLiteral(">=");
            break;
    }

    // note: a NULL value in the database means undefined and not 0!
    // We can't exclude NULL values here because they are not defined!
    d->queryFilter += QStringLiteral( " %1 (%2 %3 %4 or %2 is null)" ).arg( andOr(), nameForValue( value ), comparison, QString::number( filter ) );

    return this;
}

QueryMaker*
SqlQueryMaker::addReturnValue( qint64 value )
{
    if( d->queryType == QueryMaker::Custom )
    {
        if ( !d->queryReturnValues.isEmpty() )
            d->queryReturnValues += QLatin1Char(',');
        d->queryReturnValues += nameForValue( value );
        d->returnValueType = value;
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    if( d->queryType == QueryMaker::Custom )
    {
        if( !d->queryReturnValues.isEmpty() )
            d->queryReturnValues += QLatin1Char(',');
        QString sqlfunction;
        switch( function )
        {
            case QueryMaker::Count:
                sqlfunction = QStringLiteral("COUNT");
                break;
            case QueryMaker::Sum:
                sqlfunction = QStringLiteral("SUM");
                break;
            case QueryMaker::Max:
                sqlfunction = QStringLiteral("MAX");
                break;
            case QueryMaker::Min:
                sqlfunction = QStringLiteral("MIN");
                break;
            default:
                sqlfunction = QStringLiteral("Unknown function in SqlQueryMaker::addReturnFunction, function was: ") + QString::number( function );
        }
        d->queryReturnValues += QStringLiteral( "%1(%2)" ).arg( sqlfunction, nameForValue( value ) );
        d->returnValueType = value;
    }
    return this;
}

QueryMaker*
SqlQueryMaker::orderBy( qint64 value, bool descending )
{
    if ( d->queryOrderBy.isEmpty() )
        d->queryOrderBy = QStringLiteral(" ORDER BY ");
    else
        d->queryOrderBy += QLatin1Char(',');
    d->queryOrderBy += nameForValue( value );
    d->queryOrderBy += QStringLiteral( " %1 " ).arg( descending ? QStringLiteral("DESC") : QStringLiteral("ASC") );
    return this;
}

QueryMaker*
SqlQueryMaker::limitMaxResultSize( int size )
{
    d->maxResultSize = size;
    return this;
}

QueryMaker*
SqlQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    if( mode != AllAlbums )
    {
        d->linkedTables |= Private::ALBUM_TAB;
    }
    d->albumMode = mode;
    return this;
}

QueryMaker*
SqlQueryMaker::setLabelQueryMode( LabelQueryMode mode )
{
    d->labelMode = mode;
    return this;
}

QueryMaker*
SqlQueryMaker::beginAnd()
{
    d->queryFilter += andOr();
    d->queryFilter += QStringLiteral(" ( 1 ");
    d->andStack.push( true );
    return this;
}

QueryMaker*
SqlQueryMaker::beginOr()
{
    d->queryFilter += andOr();
    d->queryFilter += QStringLiteral(" ( 0 ");
    d->andStack.push( false );
    return this;
}

QueryMaker*
SqlQueryMaker::endAndOr()
{
    d->queryFilter += QLatin1Char(')');
    d->andStack.pop();
    return this;
}

void
SqlQueryMaker::linkTables()
{
    switch( d->queryType )
    {
        case QueryMaker::Track:
        {
            d->queryFrom += QStringLiteral(" tracks");
            if( d->linkedTables & Private::TAGS_TAB )
                d->linkedTables ^= Private::TAGS_TAB;
            break;
        }
        case QueryMaker::Artist:
        {
            d->queryFrom += QStringLiteral(" artists");
            if( d->linkedTables != Private::ARTIST_TAB )
                d->queryFrom += QStringLiteral(" JOIN tracks ON tracks.artist = artists.id");
            if( d->linkedTables & Private::ARTIST_TAB )
                d->linkedTables ^= Private::ARTIST_TAB;
            break;
        }
        case QueryMaker::Album:
        case QueryMaker::AlbumArtist:
        {
            d->queryFrom += QStringLiteral(" albums");
            if( d->linkedTables != Private::ALBUM_TAB && d->linkedTables != ( Private::ALBUM_TAB | Private::ALBUMARTIST_TAB ) )
                d->queryFrom += QStringLiteral(" JOIN tracks ON tracks.album = albums.id");
            if( d->linkedTables & Private::ALBUM_TAB )
                d->linkedTables ^= Private::ALBUM_TAB;
            break;
        }
        case QueryMaker::Genre:
        {
            d->queryFrom += QStringLiteral(" genres");
            if( d->linkedTables != Private::GENRE_TAB )
                d->queryFrom += QStringLiteral(" INNER JOIN tracks ON tracks.genre = genres.id");
            if( d->linkedTables & Private::GENRE_TAB )
                d->linkedTables ^= Private::GENRE_TAB;
            break;
        }
        case QueryMaker::Composer:
        {
            d->queryFrom += QStringLiteral(" composers");
            if( d->linkedTables != Private::COMPOSER_TAB )
                d->queryFrom += QStringLiteral(" JOIN tracks ON tracks.composer = composers.id");
            if( d->linkedTables & Private::COMPOSER_TAB )
                d->linkedTables ^= Private::COMPOSER_TAB;
            break;
        }
        case QueryMaker::Year:
        {
            d->queryFrom += QStringLiteral(" years");
            if( d->linkedTables != Private::YEAR_TAB )
                d->queryFrom += QStringLiteral(" JOIN tracks on tracks.year = years.id");
            if( d->linkedTables & Private::YEAR_TAB )
                d->linkedTables ^= Private::YEAR_TAB;
            break;
        }
        case QueryMaker::Label:
        {
            d->queryFrom += QStringLiteral(" labels");
            if( d->linkedTables != Private::LABELS_TAB )
                d->queryFrom += QStringLiteral(" INNER JOIN urls_labels ON labels.id = urls_labels.label"
                                " INNER JOIN tracks ON urls_labels.url = tracks.url");
            if( d->linkedTables & Private::LABELS_TAB )
                d->linkedTables ^= Private::LABELS_TAB;
            break;
        }
        case QueryMaker::Custom:
        {
            switch( d->returnValueType )
            {
                default:
                case Meta::valUrl:
                {
                    d->queryFrom += QStringLiteral(" tracks");
                    if( d->linkedTables & Private::TAGS_TAB )
                        d->linkedTables ^= Private::TAGS_TAB;
                    break;
                }
                case Meta::valAlbum:
                {
                    d->queryFrom += QStringLiteral(" albums");
                    if( d->linkedTables & Private::ALBUM_TAB )
                        d->linkedTables ^= Private::ALBUM_TAB;
                    if( d->linkedTables & Private::URLS_TAB )
                        d->linkedTables ^= Private::URLS_TAB;
                    break;
                }
                case Meta::valArtist:
               {
                    d->queryFrom += QStringLiteral(" artists");
                    if( d->linkedTables & Private::ARTIST_TAB )
                        d->linkedTables ^= Private::ARTIST_TAB;
                    if( d->linkedTables & Private::URLS_TAB )
                        d->linkedTables ^= Private::URLS_TAB;
                    break;
                }
                case Meta::valGenre:
                {
                    d->queryFrom += QStringLiteral(" genres");
                    if( d->linkedTables & Private::GENRE_TAB )
                        d->linkedTables ^= Private::GENRE_TAB;
                    if( d->linkedTables & Private::URLS_TAB )
                        d->linkedTables ^= Private::URLS_TAB;
                    break;
                }
            }
        }
        case QueryMaker::None:
        {
            //???
            break;
        }
    }
    if( !d->linkedTables )
        return;

    if( d->linkedTables & Private::URLS_TAB )
        d->queryFrom += QStringLiteral(" INNER JOIN urls ON tracks.url = urls.id");
    if( d->linkedTables & Private::ARTIST_TAB )
        d->queryFrom += QStringLiteral(" LEFT JOIN artists ON tracks.artist = artists.id");
    if( d->linkedTables & Private::ALBUM_TAB )
        d->queryFrom += QStringLiteral(" LEFT JOIN albums ON tracks.album = albums.id");
    if( d->linkedTables & Private::ALBUMARTIST_TAB )
        d->queryFrom += QStringLiteral(" LEFT JOIN artists AS albumartists ON albums.artist = albumartists.id");
    if( d->linkedTables & Private::GENRE_TAB )
        d->queryFrom += QStringLiteral(" LEFT JOIN genres ON tracks.genre = genres.id");
    if( d->linkedTables & Private::COMPOSER_TAB )
        d->queryFrom += QStringLiteral(" LEFT JOIN composers ON tracks.composer = composers.id");
    if( d->linkedTables & Private::YEAR_TAB )
        d->queryFrom += QStringLiteral(" LEFT JOIN years ON tracks.year = years.id");
    if( d->linkedTables & Private::STATISTICS_TAB )
    {
        if( d->linkedTables & Private::URLS_TAB )
        {
            d->queryFrom += QStringLiteral(" LEFT JOIN statistics ON urls.id = statistics.url");
        }
        else
        {
            d->queryFrom += QStringLiteral(" LEFT JOIN statistics ON tracks.url = statistics.url");
        }
    }
}

void
SqlQueryMaker::buildQuery()
{
    //URLS is always required for dynamic collection
    d->linkedTables |= Private::URLS_TAB;
    linkTables();
    QString query = QStringLiteral("SELECT ");
    if ( d->withoutDuplicates )
        query += QStringLiteral("DISTINCT ");
    query += d->queryReturnValues;
    query += QStringLiteral(" FROM ");
    query += d->queryFrom;

    // dynamic collection (only mounted file systems are considered)
    if( (d->linkedTables & Private::URLS_TAB) && m_collection->mountPointManager() )
    {
        query += QStringLiteral(" WHERE 1 ");
        IdList list = m_collection->mountPointManager()->getMountedDeviceIds();
        if( !list.isEmpty() )
        {
            QString commaSeparatedIds;
            for( int id : list )
            {
                if( !commaSeparatedIds.isEmpty() )
                    commaSeparatedIds += QLatin1Char(',');
                commaSeparatedIds += QString::number( id );
            }
            query += QStringLiteral( " AND urls.deviceid in (%1)" ).arg( commaSeparatedIds );
        }
    }

    switch( d->albumMode )
    {
        case OnlyNormalAlbums:
            query += QStringLiteral(" AND albums.artist IS NOT NULL ");
            break;
        case OnlyCompilations:
            query += QStringLiteral(" AND albums.artist IS NULL ");
            break;
        case AllAlbums:
            //do nothing
            break;
    }
    if( d->labelMode != QueryMaker::NoConstraint )
    {
        switch( d->labelMode )
        {
        case QueryMaker::OnlyWithLabels:
            query += QStringLiteral(" AND tracks.url IN ");
            break;

        case QueryMaker::OnlyWithoutLabels:
            query += QStringLiteral(" AND tracks.url NOT IN ");
            break;

        case QueryMaker::NoConstraint:
            //do nothing, will never be called
            break;
        }
        query += QStringLiteral(" (SELECT DISTINCT url FROM urls_labels) ");
    }

    query += d->queryMatch;
    if ( !d->queryFilter.isEmpty() )
    {
        query += QStringLiteral(" AND ( 1 ");
        query += d->queryFilter;
        query += QStringLiteral(" ) ");
    }
    query += d->queryOrderBy;
    if ( d->maxResultSize > -1 )
        query += QStringLiteral( " LIMIT %1 OFFSET 0 " ).arg( d->maxResultSize );
    query += QLatin1Char(';');
    d->query = query;
}

QString
SqlQueryMaker::query()
{
    if ( d->query.isEmpty() )
        buildQuery();
    return d->query;
}

QStringList
SqlQueryMaker::runQuery( const QString &query )
{
    return m_collection->sqlStorage()->query( query );
}

void
SqlQueryMaker::setBlocking( bool enabled )
{
    d->blocking = enabled;
}

QStringList
SqlQueryMaker::collectionIds() const
{
    QStringList list;
    list << m_collection->collectionId();
    return list;
}

Meta::TrackList
SqlQueryMaker::tracks() const
{
    return d->blockingTracks;
}

Meta::AlbumList
SqlQueryMaker::albums() const
{
    return d->blockingAlbums;
}

Meta::ArtistList
SqlQueryMaker::artists() const
{
    return d->blockingArtists;
}

Meta::GenreList
SqlQueryMaker::genres() const
{
    return d->blockingGenres;
}

Meta::ComposerList
SqlQueryMaker::composers() const
{
    return d->blockingComposers;
}

Meta::YearList
SqlQueryMaker::years() const
{
    return d->blockingYears;
}

QStringList
SqlQueryMaker::customData() const
{
    return d->blockingCustomData;
}

Meta::LabelList
SqlQueryMaker::labels() const
{
    return d->blockingLabels;
}

QString
SqlQueryMaker::nameForValue( qint64 value )
{
    switch( value )
    {
        case Meta::valUrl:
            d->linkedTables |= Private::URLS_TAB;
            return QStringLiteral("urls.rpath");  //TODO figure out how to handle deviceid
        case Meta::valTitle:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.title");
        case Meta::valArtist:
            d->linkedTables |= Private::ARTIST_TAB;
            return QStringLiteral("artists.name");
        case Meta::valAlbum:
            d->linkedTables |= Private::ALBUM_TAB;
            return QStringLiteral("albums.name");
        case Meta::valGenre:
            d->linkedTables |= Private::GENRE_TAB;
            return QStringLiteral("genres.name");
        case Meta::valComposer:
            d->linkedTables |= Private::COMPOSER_TAB;
            return QStringLiteral("composers.name");
        case Meta::valYear:
            d->linkedTables |= Private::YEAR_TAB;
            return QStringLiteral("years.name");
        case Meta::valBpm:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.bpm");
        case Meta::valComment:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.comment");
        case Meta::valTrackNr:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.tracknumber");
        case Meta::valDiscNr:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.discnumber");
        case Meta::valLength:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.length");
        case Meta::valBitrate:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.bitrate");
        case Meta::valSamplerate:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.samplerate");
        case Meta::valFilesize:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.filesize");
        case Meta::valFormat:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.filetype");
        case Meta::valCreateDate:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.createdate");
        case Meta::valScore:
            d->linkedTables |= Private::STATISTICS_TAB;
            return QStringLiteral("statistics.score");
        case Meta::valRating:
            d->linkedTables |= Private::STATISTICS_TAB;
            return QStringLiteral("statistics.rating");
        case Meta::valFirstPlayed:
            d->linkedTables |= Private::STATISTICS_TAB;
            return QStringLiteral("statistics.createdate");
        case Meta::valLastPlayed:
            d->linkedTables |= Private::STATISTICS_TAB;
            return QStringLiteral("statistics.accessdate");
        case Meta::valPlaycount:
            d->linkedTables |= Private::STATISTICS_TAB;
            return QStringLiteral("statistics.playcount");
        case Meta::valUniqueId:
            d->linkedTables |= Private::URLS_TAB;
            return QStringLiteral("urls.uniqueid");
        case Meta::valAlbumArtist:
            d->linkedTables |= Private::ALBUMARTIST_TAB;
            //albumartist_tab means that the artist table is joined to the albums table
            //so add albums as well
            d->linkedTables |= Private::ALBUM_TAB;
            return QStringLiteral("albumartists.name");
        case Meta::valModified:
            d->linkedTables |= Private::TAGS_TAB;
            return QStringLiteral("tracks.modifydate");
        default:
            return QStringLiteral("ERROR: unknown value in SqlQueryMaker::nameForValue(qint64): value=") + QString::number( value );
    }
}

QString
SqlQueryMaker::andOr() const
{
    return d->andStack.top() ? QStringLiteral(" AND ") : QStringLiteral(" OR ");
}

QString
SqlQueryMaker::escape( const QString &text ) const           //krazy:exclude=constref
{
    return m_collection->sqlStorage()->escape( text );
}

QString
SqlQueryMaker::likeCondition( const QString &text, bool anyBegin, bool anyEnd ) const
{
    if( anyBegin || anyEnd )
    {
        QString escaped = text;
        //according to http://dev.mysql.com/doc/refman/5.0/en/string-comparison-functions.html
        //the escape character (\ as we are using the default) is escaped twice when using like.
        //mysql_real_escape will escape it once, so we have to escape it another time here
        escaped = escaped.replace( QLatin1Char('\\'), QStringLiteral("\\\\") ); // "////" will result in two backslahes
        escaped = escape( escaped );
        //as we are in pattern matching mode '_' and '%' have to be escaped
        //mysql_real_excape_string does not do that for us
        //see http://dev.mysql.com/doc/refman/5.0/en/string-syntax.html
        //and http://dev.mysql.com/doc/refman/5.0/en/mysql-real-escape-string.html
        //replace those characters after calling escape(), which calls the mysql
        //function in turn, so that mysql does not escape the escape backslashes
        escaped.replace( QLatin1Char('%'), QStringLiteral("\\%") ).replace( QLatin1Char('_'), QStringLiteral("\\_") );

        QString ret = QStringLiteral(" LIKE ");

        ret += QLatin1Char('\'');
        if ( anyBegin )
            ret += QLatin1Char('%');
        ret += escaped;
        if ( anyEnd )
            ret += QLatin1Char('%');
        ret += QLatin1Char('\'');

        //Case insensitive collation for queries
        ret += QStringLiteral(" COLLATE utf8_unicode_ci ");

        //Use \ as the escape character
        //ret += " ESCAPE '\\' ";

        return ret;
    }
    else
    {
        return QStringLiteral( " = '%1' COLLATE utf8_unicode_ci " ).arg( escape( text ) );
    }
}

void
SqlQueryMaker::blockingNewAlbumsReady(const Meta::AlbumList &albums)
{
    d->blockingAlbums = albums;
}

void
SqlQueryMaker::blockingNewArtistsReady(const Meta::ArtistList &artists)
{
    d->blockingArtists = artists;
}

void
SqlQueryMaker::blockingNewGenresReady(const Meta::GenreList &genres)
{
    d->blockingGenres = genres;
}

void
SqlQueryMaker::blockingNewComposersReady(const Meta::ComposerList &composers)
{
    d->blockingComposers = composers;
}

void
SqlQueryMaker::blockingNewYearsReady(const Meta::YearList &years)
{
    d->blockingYears = years;
}

void
SqlQueryMaker::blockingNewTracksReady(const Meta::TrackList &tracks)
{
    d->blockingTracks = tracks;
}

void
SqlQueryMaker::blockingNewResultReady(const QStringList &customData)
{
    d->blockingCustomData = customData;
}

void
SqlQueryMaker::blockingNewLabelsReady(const Meta::LabelList &labels )
{
    d->blockingLabels = labels;
}

#include "SqlQueryMaker.moc"
