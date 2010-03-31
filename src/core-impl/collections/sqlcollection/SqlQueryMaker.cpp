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

#include "SqlQueryMaker.h"

#define DEBUG_PREFIX "SqlQueryMaker"

#include "core/support/Debug.h"

#include "SqlCollection.h"
#include "SqlQueryMakerInternal.h"
#include "core/collections/support/SqlStorage.h"

#include <QPointer>
#include <QStack>

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

using namespace Collections;

class SqlWorkerThread : public ThreadWeaver::Job
{
    public:
        SqlWorkerThread( SqlQueryMakerInternal *queryMakerInternal )
            : ThreadWeaver::Job()
            , m_queryMakerInternal( queryMakerInternal )
            , m_aborted( false )
        {
            //nothing to do
        }

        virtual ~SqlWorkerThread()
        {
            delete m_queryMakerInternal;
        }

        virtual void requestAbort()
        {
            m_aborted = true;
        }

        SqlQueryMakerInternal* queryMakerInternal() const
        {
            return m_queryMakerInternal;
        }

    protected:
        virtual void run()
        {
            m_queryMakerInternal->run();
            setFinished( !m_aborted );
        }
    private:
        SqlQueryMakerInternal *m_queryMakerInternal;

        bool m_aborted;
};

struct SqlQueryMaker::Private
{
    enum { TAGS_TAB = 1, ARTIST_TAB = 2, ALBUM_TAB = 4, GENRE_TAB = 8, COMPOSER_TAB = 16, YEAR_TAB = 32, STATISTICS_TAB = 64, URLS_TAB = 128, ALBUMARTIST_TAB = 256 };
    int linkedTables;
    QueryMaker::QueryType queryType;
    QString query;
    QString queryReturnValues;
    QString queryFrom;
    QString queryMatch;
    QString queryFilter;
    QString queryOrderBy;
    bool includedBuilder;
    bool collectionRestriction;
    bool resultAsDataPtrs;
    bool withoutDuplicates;
    int maxResultSize;
    AlbumQueryMode albumMode;
    SqlWorkerThread *worker;

    QStack<bool> andStack;

    QStringList blockingCustomData;
    Meta::DataList blockingData;
    Meta::TrackList blockingTracks;
    Meta::AlbumList blockingAlbums;
    Meta::ArtistList blockingArtists;
    Meta::GenreList blockingGenres;
    Meta::ComposerList blockingComposers;
    Meta::YearList blockingYears;
    bool blocking;
    bool used;
};

SqlQueryMaker::SqlQueryMaker( SqlCollection* collection )
    : QueryMaker()
    , m_collection( collection )
    , d( new Private )
{
    d->includedBuilder = true;
    d->collectionRestriction = false;
    d->worker = 0;
    reset();
}

SqlQueryMaker::~SqlQueryMaker()
{
    disconnect();
    abortQuery();
    delete d;
}

QueryMaker*
SqlQueryMaker::reset()
{
    d->query.clear();
    d->queryType = QueryMaker::None;
    d->queryReturnValues.clear();
    d->queryFrom.clear();
    d->queryMatch.clear();
    d->queryFilter.clear();
    d->queryOrderBy.clear();
    d->linkedTables = 0;
    if( d->worker && d->worker->isFinished() )
        delete d->worker;   //TODO error handling
    d->resultAsDataPtrs = false;
    d->withoutDuplicates = false;
    d->albumMode = AllAlbums;
    d->maxResultSize = -1;
    d->andStack.clear();
    d->andStack.push( true );   //and is default
    d->blocking = false;
    d->used = false;
    d->blockingCustomData.clear();
    d->blockingData.clear();
    d->blockingAlbums.clear();
    d->blockingArtists.clear();
    d->blockingComposers.clear();
    d->blockingGenres.clear();
    d->blockingTracks.clear();
    d->blockingYears.clear();

    return this;
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

QueryMaker*
SqlQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
    // we need the unchanged resulttype in the blocking result methods so prevent
    // reseting result type without reseting the QM
    if ( d->blocking && d->used )
        return this;

    d->resultAsDataPtrs = resultAsDataPtrs;
    return this;
}

void
SqlQueryMaker::run()
{
    if( d->queryType == QueryMaker::None || (d->blocking && d->used) )
    {
        debug() << "sql querymaker used without reset or initialization" << endl;
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
        qmi->setResultAsDataPtrs( d->resultAsDataPtrs );


        if ( !d->blocking )
        {
            connect( qmi, SIGNAL(newResultReady(QString,Meta::AlbumList)), SIGNAL(newResultReady(QString,Meta::AlbumList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::ArtistList)), SIGNAL(newResultReady(QString,Meta::ArtistList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::GenreList)), SIGNAL(newResultReady(QString,Meta::GenreList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::ComposerList)), SIGNAL(newResultReady(QString,Meta::ComposerList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::YearList)), SIGNAL(newResultReady(QString,Meta::YearList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::TrackList)), SIGNAL(newResultReady(QString,Meta::TrackList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::DataList)), SIGNAL(newResultReady(QString,Meta::DataList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,QStringList)), SIGNAL(newResultReady(QString,QStringList)), Qt::DirectConnection );
            d->worker = new SqlWorkerThread( qmi );
            connect( d->worker, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( done( ThreadWeaver::Job* ) ) );
            ThreadWeaver::Weaver::instance()->enqueue( d->worker );
        }
        else //use it blocking
        {
            connect( qmi, SIGNAL(newResultReady(QString,Meta::AlbumList)), SLOT(blockingNewResultReady(QString,Meta::AlbumList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::ArtistList)), SLOT(blockingNewResultReady(QString,Meta::ArtistList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::GenreList)), SLOT(blockingNewResultReady(QString,Meta::GenreList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::ComposerList)), SLOT(blockingNewResultReady(QString,Meta::ComposerList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::YearList)), SLOT(blockingNewResultReady(QString,Meta::YearList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::TrackList)), SLOT(blockingNewResultReady(QString,Meta::TrackList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,Meta::DataList)), SLOT(blockingNewResultReady(QString,Meta::DataList)), Qt::DirectConnection );
            connect( qmi, SIGNAL(newResultReady(QString,QStringList)), SLOT(blockingNewResultReady(QString,QStringList)), Qt::DirectConnection );
            qmi->run();
            delete qmi;
        }
    }
    d->used = true;
}

void
SqlQueryMaker::done( ThreadWeaver::Job *job )
{
    ThreadWeaver::Weaver::instance()->dequeue( job );
    job->deleteLater();
    d->worker = 0;
    emit queryDone();
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
            d->queryReturnValues = "artists.name, artists.id";
        }
        return this;

    case QueryMaker::Album:
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Album;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::ALBUM_TAB;
            //add whatever is necessary to identify compilations
            d->queryReturnValues = "albums.name, albums.id, albums.artist";
        }
        return this;

    case QueryMaker::Composer:
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Composer;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::COMPOSER_TAB;
            d->queryReturnValues = "composers.name, composers.id";
        }
        return this;

    case QueryMaker::Genre:
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Genre;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::GENRE_TAB;
            d->queryReturnValues = "genres.name, genres.id";
        }
        return this;

    case QueryMaker::Year:
        if( d->queryType == QueryMaker::None )
        {
            d->queryType = QueryMaker::Year;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::YEAR_TAB;
            d->queryReturnValues = "years.name, years.id";
        }
        return this;

    case QueryMaker::Custom:
        if( d->queryType == QueryMaker::None )
            d->queryType = QueryMaker::Custom;
        return this;

        case QueryMaker::None:
            return this;
    }
    return this;
}

QueryMaker*
SqlQueryMaker::includeCollection( const QString &collectionId )
{
    if( !d->collectionRestriction )
    {
        d->includedBuilder = false;
        d->collectionRestriction = true;
    }
    if( m_collection->collectionId() == collectionId )
        d->includedBuilder = true;
    return this;
}

QueryMaker*
SqlQueryMaker::excludeCollection( const QString &collectionId )
{
    d->collectionRestriction = true;
    if( m_collection->collectionId() == collectionId )
        d->includedBuilder = false;
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::TrackPtr &track )
{
    QString url = track->uidUrl();
    KUrl kurl( url );
    if( kurl.protocol() == "amarok-sqltrackuid" )
    {
        d->queryMatch += QString( " AND urls.uniqueid = '%1' " ).arg( kurl.url() );
    }
    else
    {
        QString path;
        if( kurl.isLocalFile() )
        {
            path = kurl.path();
        }
        else
        {
            path = track->playableUrl().path();
        }
        int deviceid = m_collection->mountPointManager()->getIdForUrl( path );
        QString rpath = m_collection->mountPointManager()->getRelativePath( deviceid, path );
        d->queryMatch += QString( " AND urls.deviceid = %1 AND urls.rpath = '%2'" )
                        .arg( QString::number( deviceid ), escape( rpath ) );
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::ArtistPtr &artist )
{
    d->linkedTables |= Private::ARTIST_TAB;
    d->queryMatch += QString( " AND artists.name = '%1'" ).arg( escape( artist->name() ) );
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
    d->linkedTables |= Private::ALBUM_TAB;
    //handle compilations
    d->queryMatch += QString( " AND albums.name = '%1'" ).arg( escape( album->name() ) );
    Meta::ArtistPtr albumArtist = album->albumArtist();
    if( albumArtist )
    {
        d->linkedTables |= Private::ALBUMARTIST_TAB;
        d->queryMatch += QString( " AND albumartists.name = '%1'" ).arg( escape( albumArtist->name() ) );
    }
    else
    {
        d->queryMatch += " AND albums.artist IS NULL";
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    d->linkedTables |= Private::GENRE_TAB;
    d->queryMatch += QString( " AND genres.name = '%1'" ).arg( escape( genre->name() ) );
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
    d->linkedTables |= Private::COMPOSER_TAB;
    d->queryMatch += QString( " AND composers.name = '%1'" ).arg( escape( composer->name() ) );
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::YearPtr &year )
{
    d->linkedTables |= Private::YEAR_TAB;
    d->queryMatch += QString( " AND years.name = '%1'" ).arg( escape( year->name() ) );
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const Meta::DataPtr &data )
{
    ( const_cast<Meta::DataPtr&>(data) )->addMatchTo( this );
    return this;
}

QueryMaker*
SqlQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    //special casing for albumartist...
    if( value == Meta::valAlbumArtist && filter.isEmpty() )
    {
        d->linkedTables |= Private::ALBUMARTIST_TAB;
        d->linkedTables |= Private::ALBUM_TAB;
        d->queryFilter += QString( " %1 ( albums.artist IS NULL or albumartists.name = '') " ).arg( andOr() );
    }
    else
    {
        QString like = likeCondition( filter, !matchBegin, !matchEnd );
        d->queryFilter += QString( " %1 %2 %3 " ).arg( andOr(), nameForValue( value ), like );
    }
    return this;
}

QueryMaker*
SqlQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    if( value == Meta::valAlbumArtist && filter.isEmpty() )
    {
        d->linkedTables |= Private::ALBUMARTIST_TAB;
        d->queryFilter += QString( " %1 NOT ( albums.artist IS NULL or albumartists.name = '') " ).arg( andOr() );
    }
    else
    {
        QString like = likeCondition( filter, !matchBegin, !matchEnd );
        d->queryFilter += QString( " %1 NOT %2 %3 " ).arg( andOr(), nameForValue( value ), like );
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
            comparison = '=';
            break;
        case QueryMaker::GreaterThan:
            comparison = '>';
            break;
        case QueryMaker::LessThan:
            comparison = '<';
            break;
    }

    if( (filter == 0 && compare == QueryMaker::Equals)
     || (filter <  0 && compare == QueryMaker::GreaterThan)
     || (filter >  0 && compare == QueryMaker::LessThan) )
    {
        d->queryFilter += QString( " %1 (%2 %3 %4 or %2 is null)" ).arg( andOr(), nameForValue( value ), comparison, QString::number( filter ) );
    }
    else
    {
        d->queryFilter += QString( " %1 %2 %3 %4 " ).arg( andOr(), nameForValue( value ), comparison, QString::number( filter ) );
    }

    return this;
}

QueryMaker*
SqlQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    QString comparison;
    switch( compare )
    {
        case QueryMaker::Equals:
            comparison = "!=";
            break;
        case QueryMaker::GreaterThan:   //negating greater than is less or equal
            comparison = "<=";
            break;
        case QueryMaker::LessThan:      //negating less than is greater or equal
            comparison = ">=";
            break;
    }

    if( (filter == 0 && compare == QueryMaker::Equals)
     || (filter >= 0 && compare == QueryMaker::GreaterThan)
     || (filter <= 0 && compare == QueryMaker::LessThan) )
    {
        d->queryFilter += QString( " %1 (%2 %3 %4 and %2 is not null)" ).arg( andOr(), nameForValue( value ), comparison, QString::number( filter ) );
    }
    else
    {
        d->queryFilter += QString( " %1 %2 %3 %4 " ).arg( andOr(), nameForValue( value ), comparison, QString::number( filter ) );
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addReturnValue( qint64 value )
{
    if( d->queryType == QueryMaker::Custom )
    {
        if ( !d->queryReturnValues.isEmpty() )
            d->queryReturnValues += ',';
        d->queryReturnValues += nameForValue( value );
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    if( d->queryType == QueryMaker::Custom )
    {
        if( !d->queryReturnValues.isEmpty() )
            d->queryReturnValues += ',';
        QString sqlfunction;
        switch( function )
        {
            case QueryMaker::Count:
                sqlfunction = "COUNT";
                break;
            case QueryMaker::Sum:
                sqlfunction = "SUM";
                break;
            case QueryMaker::Max:
                sqlfunction = "MAX";
                break;
            case QueryMaker::Min:
                sqlfunction = "MIN";
                break;
            default:
                sqlfunction = "Unknown function in SqlQueryMaker::addReturnFunction, function was: " + function;
        }
        d->queryReturnValues += QString( "%1(%2)" ).arg( sqlfunction, nameForValue( value ) );
    }
    return this;
}

QueryMaker*
SqlQueryMaker::orderBy( qint64 value, bool descending )
{
    if ( d->queryOrderBy.isEmpty() )
        d->queryOrderBy = " ORDER BY ";
    else
        d->queryOrderBy += ',';
    d->queryOrderBy += nameForValue( value );
    d->queryOrderBy += QString( " %1 " ).arg( descending ? "DESC" : "ASC" );
    return this;
}

QueryMaker*
SqlQueryMaker::orderByRandom()
{
    d->queryOrderBy = " ORDER BY " + m_collection->sqlStorage()->randomFunc();
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
SqlQueryMaker::beginAnd()
{
    d->queryFilter += andOr();
    d->queryFilter += " ( 1 ";
    d->andStack.push( true );
    return this;
}

QueryMaker*
SqlQueryMaker::beginOr()
{
    d->queryFilter += andOr();
    d->queryFilter += " ( 0 ";
    d->andStack.push( false );
    return this;
}

QueryMaker*
SqlQueryMaker::endAndOr()
{
    d->queryFilter += ')';
    d->andStack.pop();
    return this;
}

void
SqlQueryMaker::linkTables()
{
    switch( d->queryType )
    {
        case QueryMaker::Custom:
        case QueryMaker::Track:
        {
            d->queryFrom += " tracks";
            if( d->linkedTables & Private::TAGS_TAB )
                d->linkedTables ^= Private::TAGS_TAB;
            break;
        }
        case QueryMaker::Artist:
        {
            d->queryFrom += " artists";
            if( d->linkedTables != Private::ARTIST_TAB )
                d->queryFrom += " INNER JOIN tracks ON tracks.artist = artists.id";
            if( d->linkedTables & Private::ARTIST_TAB )
                d->linkedTables ^= Private::ARTIST_TAB;
            break;
        }
        case QueryMaker::Album:
        {
            d->queryFrom += " albums";
            if( d->linkedTables != Private::ALBUM_TAB && d->linkedTables != ( Private::ALBUM_TAB | Private::ALBUMARTIST_TAB ) )
                d->queryFrom += " INNER JOIN tracks ON tracks.album = albums.id";
            if( d->linkedTables & Private::ALBUM_TAB )
                d->linkedTables ^= Private::ALBUM_TAB;
            break;
        }
        case QueryMaker::Genre:
        {
            d->queryFrom += " genres";
            if( d->linkedTables != Private::GENRE_TAB )
                d->queryFrom += " INNER JOIN tracks ON tracks.genre = genres.id";
            if( d->linkedTables & Private::GENRE_TAB )
                d->linkedTables ^= Private::GENRE_TAB;
            break;
        }
        case QueryMaker::Composer:
        {
            d->queryFrom += " composers";
            if( d->linkedTables != Private::COMPOSER_TAB )
                d->queryFrom += " INNER JOIN tracks ON tracks.composer = composers.id";
            if( d->linkedTables & Private::COMPOSER_TAB )
                d->linkedTables ^= Private::COMPOSER_TAB;
            break;
        }
        case QueryMaker::Year:
        {
            d->queryFrom += " years";
            if( d->linkedTables != Private::YEAR_TAB )
                d->queryFrom += " INNER JOIN tracks on tracks.year = years.id";
            if( d->linkedTables & Private::YEAR_TAB )
                d->linkedTables ^= Private::YEAR_TAB;
            break;
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
        d->queryFrom += " INNER JOIN urls ON tracks.url = urls.id";
    if( d->linkedTables & Private::ARTIST_TAB )
        d->queryFrom += " LEFT JOIN artists ON tracks.artist = artists.id";
    if( d->linkedTables & Private::ALBUM_TAB )
        d->queryFrom += " LEFT JOIN albums ON tracks.album = albums.id";
    if( d->linkedTables & Private::ALBUMARTIST_TAB )
        d->queryFrom += " LEFT JOIN artists AS albumartists ON albums.artist = albumartists.id";
    if( d->linkedTables & Private::GENRE_TAB )
        d->queryFrom += " LEFT JOIN genres ON tracks.genre = genres.id";
    if( d->linkedTables & Private::COMPOSER_TAB )
        d->queryFrom += " LEFT JOIN composers ON tracks.composer = composers.id";
    if( d->linkedTables & Private::YEAR_TAB )
        d->queryFrom += " LEFT JOIN years ON tracks.year = years.id";
    if( d->linkedTables & Private::STATISTICS_TAB )
    {
        if( d->linkedTables & Private::URLS_TAB )
        {
            d->queryFrom += " LEFT JOIN statistics ON urls.id = statistics.url";
        }
        else
        {
            d->queryFrom += " LEFT JOIN statistics ON tracks.url = statistics.url";
        }
    }
}

void
SqlQueryMaker::buildQuery()
{
    //URLS is always required for dynamic collection
    d->linkedTables |= Private::URLS_TAB;
    linkTables();
    QString query = "SELECT ";
    if ( d->withoutDuplicates )
        query += "DISTINCT ";
    query += d->queryReturnValues;
    query += " FROM ";
    query += d->queryFrom;
    query += " WHERE 1 ";

    //dynamic collection
    Q_ASSERT( m_collection->mountPointManager() );
    IdList list = m_collection->mountPointManager()->getMountedDeviceIds();
    QString commaSeparatedIds;
    foreach( int id, list )
    {
        if( !commaSeparatedIds.isEmpty() )
            commaSeparatedIds += ',';
        commaSeparatedIds += QString::number( id );
    }
    if( !commaSeparatedIds.isEmpty() )
    {
        query += QString( " AND urls.deviceid in (%1)" ).arg( commaSeparatedIds );
    }

    switch( d->albumMode )
    {
        case OnlyNormalAlbums:
            query += " AND albums.artist IS NOT NULL ";
            break;
        case OnlyCompilations:
            query += " AND albums.artist IS NULL ";
            break;
        case AllAlbums:
            //do nothing
            break;
    }
    query += d->queryMatch;
    if ( !d->queryFilter.isEmpty() )
    {
        query += " AND ( 1 ";
        query += d->queryFilter;
        query += " ) ";
    }
    query += d->queryOrderBy;
    if ( d->maxResultSize > -1 )
        query += QString( " LIMIT %1 OFFSET 0 " ).arg( d->maxResultSize );
    query += ';';
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

Meta::DataList
SqlQueryMaker::data( const QString &id ) const
{
    Q_UNUSED( id );
    return d->blockingData;
}

Meta::TrackList
SqlQueryMaker::tracks( const QString &id ) const
{
    Q_UNUSED( id );
    return d->blockingTracks;
}

Meta::AlbumList
SqlQueryMaker::albums( const QString &id ) const
{
    Q_UNUSED( id );
    return d->blockingAlbums;
}

Meta::ArtistList
SqlQueryMaker::artists( const QString &id ) const
{
    Q_UNUSED( id );
    return d->blockingArtists;
}

Meta::GenreList
SqlQueryMaker::genres( const QString &id ) const
{
    Q_UNUSED( id );
    return d->blockingGenres;
}

Meta::ComposerList
SqlQueryMaker::composers( const QString &id ) const
{
    Q_UNUSED( id );
    return d->blockingComposers;
}

Meta::YearList
SqlQueryMaker::years( const QString &id ) const
{
    Q_UNUSED( id );
    return d->blockingYears;
}

QStringList
SqlQueryMaker::customData( const QString &id ) const
{
    Q_UNUSED( id );
    return d->blockingCustomData;
}

QString
SqlQueryMaker::nameForValue( qint64 value )
{
    switch( value )
    {
        case Meta::valUrl:
            d->linkedTables |= Private::URLS_TAB;
            return "urls.rpath";  //TODO figure out how to handle deviceid
        case Meta::valTitle:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.title";
        case Meta::valArtist:
            d->linkedTables |= Private::ARTIST_TAB;
            return "artists.name";
        case Meta::valAlbum:
            d->linkedTables |= Private::ALBUM_TAB;
            return "albums.name";
        case Meta::valGenre:
            d->linkedTables |= Private::GENRE_TAB;
            return "genres.name";
        case Meta::valComposer:
            d->linkedTables |= Private::COMPOSER_TAB;
            return "composers.name";
        case Meta::valYear:
            d->linkedTables |= Private::YEAR_TAB;
            return "years.name";
        case Meta::valBpm:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.bpm";
        case Meta::valComment:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.comment";
        case Meta::valTrackNr:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.tracknumber";
        case Meta::valDiscNr:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.discnumber";
        case Meta::valLength:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.length";
        case Meta::valBitrate:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.bitrate";
        case Meta::valSamplerate:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.samplerate";
        case Meta::valFilesize:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.filesize";
        case Meta::valFormat:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.filetype";
        case Meta::valCreateDate:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.createdate";
        case Meta::valScore:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.score";
        case Meta::valRating:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.rating";
        case Meta::valFirstPlayed:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.createdate";
        case Meta::valLastPlayed:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.accessdate";
        case Meta::valPlaycount:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.playcount";
        case Meta::valUniqueId:
            d->linkedTables |= Private::URLS_TAB;
            return "urls.uniqueid";
        case Meta::valAlbumArtist:
            d->linkedTables |= Private::ALBUMARTIST_TAB;
            //albumartist_tab means that the artist table is joined to the albums table
            //so add albums as well
            d->linkedTables |= Private::ALBUM_TAB;
            return "albumartists.name";
        default:
            return "ERROR: unknown value in SqlQueryMaker::nameForValue(qint64): value=" + value;
    }
}

QString
SqlQueryMaker::andOr() const
{
    return d->andStack.top() ? " AND " : " OR ";
}

QString
SqlQueryMaker::escape( QString text ) const           //krazy:exclude=constref
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
        escaped = escaped.replace( '\\', "\\\\" ); // "////" will result in two backslahes
        escaped = escape( escaped );
        //as we are in pattern matching mode '_' and '%' have to be escaped
        //mysql_real_excape_string does not do that for us
        //see http://dev.mysql.com/doc/refman/5.0/en/string-syntax.html
        //and http://dev.mysql.com/doc/refman/5.0/en/mysql-real-escape-string.html
        //replace those characters after calling escape(), which calls the mysql
        //function in turn, so that mysql does not escape the escape backslashes
        escaped.replace( '%', "\\%" ).replace( '_', "\\_" );

        QString ret = " LIKE ";

        ret += '\'';
        if ( anyBegin )
            ret += '%';
        ret += escaped;
        if ( anyEnd )
            ret += '%';
        ret += '\'';

        //Case insensitive collation for queries
        ret += " COLLATE utf8_unicode_ci ";

        //Use \ as the escape character
        //ret += " ESCAPE '\\' ";

        return ret;
    }
    else
    {
        return QString( " = '%1' " ).arg( escape( text ) );
    }
}

void
SqlQueryMaker::blockingNewResultReady(const QString &id, const Meta::AlbumList &albums)
{
    Q_UNUSED( id );
    d->blockingAlbums = albums;
}

void
SqlQueryMaker::blockingNewResultReady(const QString &id, const Meta::ArtistList &artists)
{
    Q_UNUSED( id );
    d->blockingArtists = artists;
}

void
SqlQueryMaker::blockingNewResultReady(const QString &id, const Meta::GenreList &genres)
{
    Q_UNUSED( id );
    d->blockingGenres = genres;
}

void
SqlQueryMaker::blockingNewResultReady(const QString &id, const Meta::ComposerList &composers)
{
    Q_UNUSED( id );
    d->blockingComposers = composers;
}

void
SqlQueryMaker::blockingNewResultReady(const QString &id, const Meta::YearList &years)
{
    Q_UNUSED( id );
    d->blockingYears = years;
}

void
SqlQueryMaker::blockingNewResultReady(const QString &id, const Meta::TrackList &tracks)
{
    Q_UNUSED( id );
    d->blockingTracks = tracks;
}

void
SqlQueryMaker::blockingNewResultReady(const QString &id, const Meta::DataList &data)
{
    Q_UNUSED( id );
    d->blockingData = data;
}

void
SqlQueryMaker::blockingNewResultReady(const QString &id, const QStringList &customData)
{
    Q_UNUSED( id );
    d->blockingCustomData = customData;
}

#include "SqlQueryMaker.moc"

