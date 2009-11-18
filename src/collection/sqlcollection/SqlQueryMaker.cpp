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

#include "Debug.h"

#include "MountPointManager.h"
#include "SqlCollection.h"

#include <QStack>

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

using namespace Meta;

class SqlWorkerThread : public ThreadWeaver::Job
{
    public:
        SqlWorkerThread( SqlQueryMaker *queryMaker )
            : ThreadWeaver::Job()
            , m_queryMaker( queryMaker )
            , m_aborted( false )
        {
            //nothing to do
        }

        virtual void requestAbort()
        {
            m_aborted = true;
        }

    protected:
        virtual void run()
        {
            QString query = m_queryMaker->query();
            QStringList result = m_queryMaker->runQuery( query );
            if( !m_aborted )
                m_queryMaker->handleResult( result );
            setFinished( !m_aborted );
        }
    private:
        SqlQueryMaker *m_queryMaker;

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

    Meta::DataList data;
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
    d->data.clear();
    return this;
}

void
SqlQueryMaker::abortQuery()
{
    if( d->worker )
        d->worker->requestAbort();
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
    else if ( ! d->blocking )
    {
        //debug() << "Query is " << query();
        d->worker = new SqlWorkerThread( this );
        connect( d->worker, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( done( ThreadWeaver::Job* ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( d->worker );
    }
    else //use it blocking
    {
        QString query = this->query();
        QStringList result = runQuery( query );
        handleResult( result );
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
            d->queryReturnValues = SqlTrack::getTrackReturnValues();
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
SqlQueryMaker::addMatch( const TrackPtr &track )
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
        int deviceid = MountPointManager::instance()->getIdForUrl( path );
        QString rpath = MountPointManager::instance()->getRelativePath( deviceid, path );
        d->queryMatch += QString( " AND urls.deviceid = %1 AND urls.rpath = '%2'" )
                        .arg( QString::number( deviceid ), escape( rpath ) );
    }
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const ArtistPtr &artist )
{
    d->linkedTables |= Private::ARTIST_TAB;
    d->queryMatch += QString( " AND artists.name = '%1'" ).arg( escape( artist->name() ) );
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const AlbumPtr &album )
{
    d->linkedTables |= Private::ALBUM_TAB;
    //handle compilations
    d->queryMatch += QString( " AND albums.name = '%1'" ).arg( escape( album->name() ) );
    ArtistPtr albumArtist = album->albumArtist();
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
SqlQueryMaker::addMatch( const GenrePtr &genre )
{
    d->linkedTables |= Private::GENRE_TAB;
    d->queryMatch += QString( " AND genres.name = '%1'" ).arg( escape( genre->name() ) );
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const ComposerPtr &composer )
{
    d->linkedTables |= Private::COMPOSER_TAB;
    d->queryMatch += QString( " AND composers.name = '%1'" ).arg( escape( composer->name() ) );
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const YearPtr &year )
{
    d->linkedTables |= Private::YEAR_TAB;
    d->queryMatch += QString( " AND years.name = '%1'" ).arg( escape( year->name() ) );
    return this;
}

QueryMaker*
SqlQueryMaker::addMatch( const DataPtr &data )
{
    ( const_cast<DataPtr&>(data) )->addMatchTo( this );
    return this;
}

QueryMaker*
SqlQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    //special casing for albumartist...
    if( value == Meta::valAlbumArtist && filter.isEmpty() )
    {
        d->linkedTables |= Private::ALBUMARTIST_TAB;
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
    d->queryOrderBy = " ORDER BY " + m_collection->randomFunc();
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
    linkTables();
    QString query = "SELECT ";
    if ( d->withoutDuplicates )
        query += "DISTINCT ";
    query += d->queryReturnValues;
    query += " FROM ";
    query += d->queryFrom;
    query += " WHERE 1 ";
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
    return m_collection->query( query );
}

void
SqlQueryMaker::handleResult( const QStringList &result )
{
    if( !result.isEmpty() )
    {
        switch( d->queryType ) {
        case QueryMaker::Custom:
            emit newResultReady( m_collection->collectionId(), result );
            break;
        case QueryMaker::Track:
            handleTracks( result );
            break;
        case QueryMaker::Artist:
            handleArtists( result );
            break;
        case QueryMaker::Album:
            handleAlbums( result );
            break;
        case QueryMaker::Genre:
            handleGenres( result );
            break;
        case QueryMaker::Composer:
            handleComposers( result );
            break;
        case QueryMaker::Year:
            handleYears( result );
            break;

        case QueryMaker::None:
            debug() << "Warning: queryResult with queryType == NONE";
        }
    }
    else
    {
        if( d->resultAsDataPtrs )
        {
            emit newResultReady( m_collection->collectionId(), Meta::DataList() );
        }
        else
        {
            switch( d->queryType ) {
                case QueryMaker::Custom:
                    emit newResultReady( m_collection->collectionId(), QStringList() );
                    break;
                case QueryMaker::Track:
                    emit newResultReady( m_collection->collectionId(), Meta::TrackList() );
                    break;
                case QueryMaker::Artist:
                    emit newResultReady( m_collection->collectionId(), Meta::ArtistList() );
                    break;
                case QueryMaker::Album:
                    emit newResultReady( m_collection->collectionId(), Meta::AlbumList() );
                    break;
                case QueryMaker::Genre:
                    emit newResultReady( m_collection->collectionId(), Meta::GenreList() );
                    break;
                case QueryMaker::Composer:
                    emit newResultReady( m_collection->collectionId(), Meta::ComposerList() );
                    break;
                case QueryMaker::Year:
                    emit newResultReady( m_collection->collectionId(), Meta::YearList() );
                    break;

            case QueryMaker::None:
                debug() << "Warning: queryResult with queryType == NONE";
            }
        }
    }

    //queryDone will be emitted in done(Job*)
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
    if ( d->blocking && d->used && d->resultAsDataPtrs && m_collection->collectionId() == id )
        return d->data;
    else
        return Meta::DataList();
}

Meta::TrackList
SqlQueryMaker::tracks( const QString &id ) const
{
    if ( d->blocking && d->used && d->queryType == QueryMaker::Track && m_collection->collectionId() == id  )
    {
        Meta::TrackList list;
        foreach( DataPtr p, d->data )
        {
            list << Meta::TrackPtr::staticCast( p );
        }
        return list;
    }
    else
        return Meta::TrackList();
}

Meta::AlbumList
SqlQueryMaker::albums( const QString &id ) const
{
    if ( d->blocking && d->used && d->queryType == QueryMaker::Album && m_collection->collectionId() == id  )
    {
        Meta::AlbumList list;
        foreach( DataPtr p, d->data )
        {
            list << Meta::AlbumPtr::staticCast( p );
        }
        return list;
    }
    else
        return Meta::AlbumList();
}

Meta::ArtistList
SqlQueryMaker::artists( const QString &id ) const
{
    if ( d->blocking && d->used && d->queryType == QueryMaker::Artist && m_collection->collectionId() == id  )
    {
        Meta::ArtistList list;
        foreach( DataPtr p, d->data )
        {
            list << Meta::ArtistPtr::staticCast( p );
        }
        return list;
    }
    else
        return Meta::ArtistList();
}

Meta::GenreList
SqlQueryMaker::genres( const QString &id ) const
{
    if ( d->blocking && d->used && d->queryType == QueryMaker::Genre && m_collection->collectionId() == id  )
    {
        Meta::GenreList list;
        foreach( DataPtr p, d->data )
        {
            list << Meta::GenrePtr::staticCast( p );
        }
        return list;
    }
    else
        return Meta::GenreList();
}

Meta::ComposerList
SqlQueryMaker::composers( const QString &id ) const
{
    if ( d->blocking && d->used && d->queryType == QueryMaker::Composer && m_collection->collectionId() == id  )
    {
        Meta::ComposerList list;
        foreach( DataPtr p, d->data )
        {
            list << Meta::ComposerPtr::staticCast( p );
        }
        return list;
    }
    else
        return Meta::ComposerList();
}

Meta::YearList
SqlQueryMaker::years( const QString &id ) const
{
    if ( d->blocking && d->used && d->queryType == QueryMaker::Year && m_collection->collectionId() == id  )
    {
        Meta::YearList list;
        foreach( DataPtr p, d->data )
        {
            list << Meta::YearPtr::staticCast( p );
        }
        return list;
    }
    else
        return Meta::YearList();
}

QStringList
SqlQueryMaker::customData( const QString &id ) const
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( id )
    // not implemented yet
    return QStringList();
}

QString
SqlQueryMaker::nameForValue( qint64 value )
{
    switch( value )
    {
        case valUrl:
            d->linkedTables |= Private::URLS_TAB;
            return "urls.rpath";  //TODO figure out how to handle deviceid
        case valTitle:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.title";
        case valArtist:
            d->linkedTables |= Private::ARTIST_TAB;
            return "artists.name";
        case valAlbum:
            d->linkedTables |= Private::ALBUM_TAB;
            return "albums.name";
        case valGenre:
            d->linkedTables |= Private::GENRE_TAB;
            return "genres.name";
        case valComposer:
            d->linkedTables |= Private::COMPOSER_TAB;
            return "composers.name";
        case valYear:
            d->linkedTables |= Private::YEAR_TAB;
            return "years.name";
        case valComment:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.comment";
        case valTrackNr:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.tracknumber";
        case valDiscNr:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.discnumber";
        case valLength:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.length";
        case valBitrate:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.bitrate";
        case valSamplerate:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.samplerate";
        case valFilesize:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.filesize";
        case valFormat:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.filetype";
        case valCreateDate:
            d->linkedTables |= Private::TAGS_TAB;
            return "tracks.createdate";
        case valScore:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.score";
        case valRating:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.rating";
        case valFirstPlayed:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.createdate";
        case valLastPlayed:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.accessdate";
        case valPlaycount:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.playcount";
        case valUniqueId:
            d->linkedTables |= Private::URLS_TAB;
            return "urls.uniqueid";
        case valAlbumArtist:
            d->linkedTables |= Private::ALBUMARTIST_TAB;
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

// What's worse, a bunch of almost identical repeated code, or a not so obvious macro? :-)
// The macro below will emit the proper result signal. If m_resultAsDataPtrs is true,
// it'll emit the signal that takes a list of DataPtrs. Otherwise, it'll call the
// signal that takes the list of the specific class.
// If qm is used blocking it only stores the result ptrs into data as DataPtrs

#define emitOrStoreProperResult( PointerType, list ) { \
            if ( d->resultAsDataPtrs || d->blocking ) { \
                foreach( PointerType p, list ) { \
                    d->data << DataPtr::staticCast( p ); \
                } \
                if ( !d->blocking ) \
                emit newResultReady( m_collection->collectionId(), d->data ); \
            } \
            else { \
                emit newResultReady( m_collection->collectionId(), list ); \
            } \
       }

void
SqlQueryMaker::handleTracks( const QStringList &result )
{
    TrackList tracks;
    SqlRegistry* reg = m_collection->registry();
    //there are 29 columns in the result set as generated by startTrackQuery()
    int returnCount = SqlTrack::getTrackReturnValueCount();
    int resultRows = result.size() / returnCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*returnCount, returnCount );
        tracks.append( reg->getTrack( row ) );
    }
    emitOrStoreProperResult( TrackPtr, tracks );
}

void
SqlQueryMaker::handleArtists( const QStringList &result )
{
    ArtistList artists;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        artists.append( reg->getArtist( name, id.toInt() ) );
    }
    emitOrStoreProperResult( ArtistPtr, artists );
}

void
SqlQueryMaker::handleAlbums( const QStringList &result )
{
    AlbumList albums;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        QString artist = iter.next();
        albums.append( reg->getAlbum( name, id.toInt(), artist.toInt() ) );
    }
    emitOrStoreProperResult( AlbumPtr, albums );
}

void
SqlQueryMaker::handleGenres( const QStringList &result )
{
    GenreList genres;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        genres.append( reg->getGenre( name, id.toInt() ) );
    }
    emitOrStoreProperResult( GenrePtr, genres );
}

void
SqlQueryMaker::handleComposers( const QStringList &result )
{
    ComposerList composers;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        composers.append( reg->getComposer( name, id.toInt() ) );
    }
    emitOrStoreProperResult( ComposerPtr, composers );
}

void
SqlQueryMaker::handleYears( const QStringList &result )
{
    YearList years;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        years.append( reg->getYear( name, id.toInt() ) );
    }
    emitOrStoreProperResult( YearPtr, years );
}

QString
SqlQueryMaker::escape( QString text ) const           //krazy:exclude=constref
{
    return m_collection->escape( text );
}

QString
SqlQueryMaker::likeCondition( const QString &text, bool anyBegin, bool anyEnd ) const
{
    if( anyBegin || anyEnd )
    {
        QString escaped = text;
        escaped = escape( escaped );
        //as we are in pattern matching mode '_' and '%' have to be escaped
        //mysql_real_excape_string does not do that for us
        //see http://dev.mysql.com/doc/refman/5.0/en/string-syntax.html
        //and http://dev.mysql.com/doc/refman/5.0/en/mysql-real-escape-string.html
        //replace those characters after calling escape(), which calls the mysql
        //function in turn, so that mysql does not escape the escape backslashes
        escaped.replace( '%', "/%" ).replace( '_', "/_" );

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

        //Use / as the escape character
        ret += " ESCAPE '/' ";

        return ret;
    }
    else
    {
        return QString( " = '%1' " ).arg( escape( text ) );
    }
}

#include "SqlQueryMaker.moc"

