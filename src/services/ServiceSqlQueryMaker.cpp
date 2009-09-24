/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#include "ServiceSqlQueryMaker.h"

#define DEBUG_PREFIX "ServiceSqlQueryMaker"

#include "Debug.h"

#include "MetaConstants.h"
#include "ServiceSqlCollection.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QStack>

using namespace Meta;

class ServiceSqlWorkerThread : public ThreadWeaver::Job
{
    public:
        ServiceSqlWorkerThread( ServiceSqlQueryMaker *queryMaker )
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
        ServiceSqlQueryMaker *m_queryMaker;

        bool m_aborted;
};

struct ServiceSqlQueryMaker::Private
{
    enum QueryType { NONE, TRACK, ARTIST, ALBUM, GENRE, COMPOSER, YEAR, CUSTOM };
    enum {TRACKS_TABLE = 1, ALBUMS_TABLE = 2, ARTISTS_TABLE = 4, GENRE_TABLE = 8};
    int linkedTables;
    QueryType queryType;
    QString query;
    QString queryReturnValues;
    QString queryFrom;
    QString queryMatch;
    QString queryFilter;
    QString queryOrderBy;
    //bool includedBuilder;
    //bool collectionRestriction;
    AlbumQueryMode albumMode;
    bool returnDataPtrs;
    bool withoutDuplicates;
    int maxResultSize;
    ServiceSqlWorkerThread *worker;
    QStack<bool> andStack;
};

ServiceSqlQueryMaker::ServiceSqlQueryMaker( ServiceSqlCollection* collection, ServiceMetaFactory * metaFactory, ServiceSqlRegistry * registry )
    : QueryMaker()
    , m_collection( collection )
    , m_registry( registry )
    , m_metaFactory( metaFactory )
    , d( new Private )
{
    //d->includedBuilder = true;
    //d->collectionRestriction = false;
    d->worker = 0;
    reset();
}

ServiceSqlQueryMaker::~ServiceSqlQueryMaker()
{
    delete d;
}

QueryMaker*
ServiceSqlQueryMaker::reset()
{
    d->query.clear();
    d->queryType = Private::NONE;
    d->queryReturnValues.clear();
    d->queryFrom.clear();
    d->queryMatch.clear();
    d->queryFilter.clear();
    d->queryOrderBy.clear();
    d->linkedTables = 0;
    if( d->worker && d->worker->isFinished() )
        delete d->worker;   //TODO error handling
    d->returnDataPtrs = false;
    d->withoutDuplicates = false;
    d->maxResultSize = -1;
    d->andStack.clear();
    d->andStack.push( true );
    return this;
}

void
ServiceSqlQueryMaker::abortQuery()
{
    if( d->worker )
        d->worker->requestAbort();
}

QueryMaker*
ServiceSqlQueryMaker::setReturnResultAsDataPtrs( bool returnDataPtrs )
{
    d->returnDataPtrs = returnDataPtrs;
    return this;
}

void
ServiceSqlQueryMaker::run()
{
    if( d->queryType == Private::NONE )
        return; //better error handling?
    if( d->worker && !d->worker->isFinished() )
    {
        //the worker thread seems to be running
        //TODO: wait or job to complete

    }
    else
    {
        d->worker = new ServiceSqlWorkerThread( this );
        connect( d->worker, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( done( ThreadWeaver::Job* ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( d->worker );
    }
}

void
ServiceSqlQueryMaker::done( ThreadWeaver::Job *job )
{
    ThreadWeaver::Weaver::instance()->dequeue( job );
    job->deleteLater();
    d->worker = 0;
    emit queryDone();
}

QueryMaker*
ServiceSqlQueryMaker::setQueryType( QueryType type)
{
    switch( type ) {
    case QueryMaker::Track:
        //make sure to keep this method in sync with handleTracks(QStringList) and the SqlTrack ctor
        if( d->queryType == Private::NONE )
        {
            QString prefix = m_metaFactory->tablePrefix();
            //d->queryFrom = ' ' + prefix + "_tracks";

            d->withoutDuplicates = true;
            d->queryFrom = ' ' + prefix + "_tracks";
            d->queryType = Private::TRACK;
            d->queryReturnValues =  m_metaFactory->getTrackSqlRows() + ',' +
            m_metaFactory->getAlbumSqlRows() + ',' +
            m_metaFactory->getArtistSqlRows() + ',' +
            m_metaFactory->getGenreSqlRows();

            d->linkedTables |= Private::GENRE_TABLE;
            d->linkedTables |= Private::ARTISTS_TABLE;
            d->linkedTables |= Private::ALBUMS_TABLE;

            d->queryOrderBy += " GROUP BY " + prefix + "_tracks.id"; //fixes the same track being shown several times due to being in several genres

            if ( d->linkedTables & Private::ARTISTS_TABLE )
            {
                d->queryOrderBy += " ORDER BY " + prefix + "_tracks.album_id"; //make sure items are added as album groups
            }
        }
        return this;

    case QueryMaker::Artist:
        if( d->queryType == Private::NONE )
        {
            QString prefix = m_metaFactory->tablePrefix();
            d->queryFrom = ' ' + prefix + "_tracks";
            d->linkedTables |= Private::ARTISTS_TABLE;
            d->linkedTables |= Private::ALBUMS_TABLE;
            d->queryType = Private::ARTIST;
            d->withoutDuplicates = true;
            d->queryReturnValues = m_metaFactory->getArtistSqlRows();

            d->queryOrderBy += " GROUP BY " + prefix + "_tracks.id"; //fixes the same track being shown several times due to being in several genres
        }
        return this;

    case QueryMaker::Album:
        if( d->queryType == Private::NONE )
        {
            QString prefix = m_metaFactory->tablePrefix();
            d->queryFrom = ' ' + prefix + "_tracks";
            d->queryType = Private::ALBUM;
            d->linkedTables |= Private::ALBUMS_TABLE;
            d->linkedTables |= Private::ARTISTS_TABLE;
            d->withoutDuplicates = true;
            d->queryReturnValues = m_metaFactory->getAlbumSqlRows() + ',' +
            m_metaFactory->getArtistSqlRows();

            d->queryOrderBy += " GROUP BY " + prefix + "_tracks.id"; //fixes the same track being shown several times due to being in several genres
        }
        return this;

    case QueryMaker::Composer:
        /* if( d->queryType == Private::NONE )
            {
                d->queryType = Private::COMPOSER;
                d->withoutDuplicates = true;
                d->linkedTables |= Private::COMPOSER_TAB;
                d->queryReturnValues = "composer.name, composer.id";
            }*/
        return this;

    case QueryMaker::Genre:
        if( d->queryType == Private::NONE )
        {
            QString prefix = m_metaFactory->tablePrefix();
            d->queryFrom = ' ' + prefix + "_genre";
            d->queryType = Private::GENRE;
            //d->linkedTables |= Private::ALBUMS_TABLE;
            //d->linkedTables |= Private::GENRE_TABLE;
            d->withoutDuplicates = true;
            d->queryReturnValues = m_metaFactory->getGenreSqlRows();
            d->queryOrderBy = " GROUP BY " + prefix +"_genre.name"; // HAVING COUNT ( " + prefix +"_genre.name ) > 10 ";
        }
        return this;

    case QueryMaker::Year:
        /*if( d->queryType == Private::NONE )
        {
            d->queryType = Private::YEAR;
            d->withoutDuplicates = true;
            d->linkedTables |= Private::YEAR_TAB;
            d->queryReturnValues = "year.name, year.id";
        }*/
        return this;

    case QueryMaker::Custom:
        /* if( d->queryType == Private::NONE )
        d->queryType = Private::CUSTOM;*/
        return this;
    
    case QueryMaker::None:
        return this;
    }

    return this;
}

QueryMaker*
ServiceSqlQueryMaker::includeCollection( const QString &collectionId )
{
    Q_UNUSED( collectionId );
  /*  if( !d->collectionRestriction )
    {
        d->includedBuilder = false;
        d->collectionRestriction = true;
    }
    if( m_collection->collectionId() == collectionId )
        d->includedBuilder = true;*/
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::excludeCollection( const QString &collectionId )
{
     Q_UNUSED( collectionId );
    /*d->collectionRestriction = true;
    if( m_collection->collectionId() == collectionId )
        d->includedBuilder = false;*/
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const TrackPtr &track )
{
    //DEBUG_BLOCK
    Q_UNUSED( track );
    //TODO still pondereing this one...
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const ArtistPtr &artist )
{
    QString prefix = m_metaFactory->tablePrefix();

    if( !d )
        return this;

    //this should NOT be made into a static cast as this might get called with an incompatible type!
    const ServiceArtist * serviceArtist = dynamic_cast<const ServiceArtist *>( artist.data() );
    d->linkedTables |= Private::ARTISTS_TABLE;
    if( serviceArtist )
    {
        d->queryMatch += QString( " AND " + prefix + "_artists.id= '%1'" ).arg( serviceArtist->id() );
    }
    else
    {
        d->queryMatch += QString( " AND " + prefix + "_artists.name='%1'" ).arg( escape( artist->name() ) );
    }
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const AlbumPtr &album )
{
    QString prefix = m_metaFactory->tablePrefix();

    if( !d )
        return this;

    //this should NOT be made into a static cast as this might get called with an incompatible type!
    const ServiceAlbumPtr serviceAlbum = ServiceAlbumPtr::dynamicCast( album );

    d->linkedTables |= Private::ALBUMS_TABLE;
    d->linkedTables |= Private::ARTISTS_TABLE;
    if( d->queryType == Private::GENRE )
        d->linkedTables |= Private::GENRE_TABLE;
    if( serviceAlbum )
    {
        d->queryMatch += QString( " AND " + prefix + "_albums.id = '%1'" ).arg( serviceAlbum->id() );
    }
    else
    {
        d->queryMatch += QString( " AND " + prefix + "_albums.name='%1'" ).arg( escape( album->name() ) );
    }
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const GenrePtr &genre )
{
    QString prefix = m_metaFactory->tablePrefix();

    //this should NOT be made into a static cast as this might get called with an incompatible type!
    const ServiceGenre* serviceGenre = static_cast<const ServiceGenre *>( genre.data() );
    if( !d || !serviceGenre )
        return this;

    //genres link to albums in the database, so we need to start from here unless soig a track query

   // if (  d->queryType == Private::TRACK ) {
        //d->queryFrom = ' ' + prefix + "_tracks";
        d->linkedTables |= Private::ALBUMS_TABLE;
    //} else
        //d->queryFrom = ' ' + prefix + "_albums";

        //if ( d->queryType == Private::ARTIST )
        //d->linkedTables |= Private::ARTISTS_TABLE;
    d->linkedTables |= Private::GENRE_TABLE;
    d->queryMatch += QString( " AND " + prefix + "_genre.name = '%1'" ).arg( serviceGenre->name() );

    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const ComposerPtr &composer )
{
    Q_UNUSED( composer );
    //TODO
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const YearPtr &year )
{
    Q_UNUSED( year );
    //TODO
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const DataPtr &data )
{
    ( const_cast<DataPtr&>(data) )->addMatchTo( this );
    //TODO needed at all?
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    //a few hacks needed by some of the speedup code:
    if ( d->queryType == Private::GENRE )
    {
        QString prefix = m_metaFactory->tablePrefix();
        d->queryFrom = ' ' + prefix + "_tracks";
        d->linkedTables |= Private::ALBUMS_TABLE;
        d->linkedTables |= Private::ARTISTS_TABLE;
        d->linkedTables |= Private::GENRE_TABLE;
    }
    QString like = likeCondition( escape( filter ), !matchBegin, !matchEnd );
    d->queryFilter += QString( " %1 %2 %3 " ).arg( andOr(), nameForValue( value ), like );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    QString like = likeCondition( escape( filter ), !matchBegin, !matchEnd );
    d->queryFilter += QString( " %1 NOT %2 %3 " ).arg( andOr(), nameForValue( value ), like );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( value )
    Q_UNUSED( filter )
    Q_UNUSED( compare )
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( value )
    Q_UNUSED( filter )
    Q_UNUSED( compare )
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addReturnValue( qint64 value )
{
    Q_UNUSED( value );
    /*if( d->queryType == Private::CUSTOM )
    {
        if ( !d->queryReturnValues.isEmpty() )
            d->queryReturnValues += ',';
        d->queryReturnValues += nameForValue( value );
    }*/
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    Q_UNUSED( value )
    Q_UNUSED( function )
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::orderBy( qint64 value, bool descending )
{
    Q_UNUSED( value );
    if ( d->queryOrderBy.isEmpty() )
    d->queryOrderBy = " ORDER BY name "; //TODO FIX!!
    d->queryOrderBy += QString( " %1 " ).arg( descending ? "DESC" : "ASC" );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::orderByRandom()
{
    // TODO
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::limitMaxResultSize( int size )
{
    d->maxResultSize = size;
    return this;
}

void
ServiceSqlQueryMaker::linkTables()
{
    if( !d->linkedTables )
        return;

    QString prefix = m_metaFactory->tablePrefix();

    //d->queryFrom = ' ' + prefix + "_tracks";

    if( d->linkedTables & Private::ALBUMS_TABLE )
       d->queryFrom += " LEFT JOIN " + prefix + "_albums ON " + prefix + "_tracks.album_id = " + prefix + "_albums.id";
    if( d->linkedTables & Private::ARTISTS_TABLE )
       d->queryFrom += " LEFT JOIN " + prefix + "_artists ON " + prefix + "_albums.artist_id = " + prefix + "_artists.id";
    if( d->linkedTables & Private::GENRE_TABLE )
       d->queryFrom += " LEFT JOIN " + prefix + "_genre ON " + prefix + "_genre.album_id = " + prefix + "_albums.id";
}

void
ServiceSqlQueryMaker::buildQuery()
{
    if( d->albumMode == OnlyCompilations )
        return;

    linkTables();
    QString query = "SELECT ";
    if ( d->withoutDuplicates )
        query += "DISTINCT ";
    query += d->queryReturnValues;
    query += " FROM ";
    query += d->queryFrom;
    query += " WHERE 1 "; // oh... to not have to bother with the leadig "AND"
                          // that may or may not be needed
    query += d->queryMatch;
    if ( !d->queryFilter.isEmpty() )
    {
        query += " AND ( 1 ";
        query += d->queryFilter;
        query += " ) ";
    }
    //query += d->queryFilter;
    query += d->queryOrderBy;
    if ( d->maxResultSize > -1 )
        query += QString( " LIMIT %1 OFFSET 0 " ).arg( d->maxResultSize );
    query += ';';
    d->query = query;
}

QString
ServiceSqlQueryMaker::query()
{
    if ( d->query.isEmpty() )
        buildQuery();
    return d->query;
}

QStringList
ServiceSqlQueryMaker::runQuery( const QString &query )
{
   if( d->albumMode == OnlyCompilations )
       return QStringList();

    return m_collection->query( query );
}

void
ServiceSqlQueryMaker::handleResult( const QStringList &result )
{
    if( !result.isEmpty() )
    {
        switch( d->queryType ) {
        /*case Private::CUSTOM:
            emit newResultReady( m_collection->collectionId(), result );
            break;*/
        case Private::TRACK:
            handleTracks( result );
            break;
        case Private::ARTIST:
            handleArtists( result );
            break;
        case Private::ALBUM:
            handleAlbums( result );
            break;
        case Private::GENRE:
            handleGenres( result );
            break;
      /*  case Private::COMPOSER:
            handleComposers( result );
            break;
        case Private::YEAR:
            handleYears( result );
            break;*/
        case Private::NONE:
            debug() << "Warning: queryResult with queryType == NONE";

        default:
            break;
        }
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

    //queryDone will be emitted in done(Job*)
}

QString
ServiceSqlQueryMaker::nameForValue(qint64 value)
{
    QString prefix = m_metaFactory->tablePrefix();

    switch( value )
    {
        case Meta::valTitle:
            d->linkedTables |= Private::TRACKS_TABLE;
            return prefix + "_tracks.name";
        case Meta::valArtist:
            d->linkedTables |= Private::ARTISTS_TABLE;
            return prefix + "_artists.name";
        case Meta::valAlbum:
            d->linkedTables |= Private::ALBUMS_TABLE;
            return prefix + "_albums.name";
        case Meta::valGenre:
            d->queryFrom = prefix + "_tracks";
            d->linkedTables |= Private::ALBUMS_TABLE;
            d->linkedTables |= Private::GENRE_TABLE;
            return prefix + "_genre.name";
        default:
            return "ERROR: unknown value in ServiceSqlQueryMaker::nameForValue(qint64): value=" + value;
    }
}

template<class PointerType, class ListType>
void ServiceSqlQueryMaker::emitProperResult( const ListType& list )
{
    if ( d->returnDataPtrs ) {
        DataList data;
        foreach( PointerType p, list )
            data << DataPtr::staticCast( p );

        emit newResultReady( m_collection->collectionId(), data );
    }
    else
        emit newResultReady( m_collection->collectionId(), list );
}

void
ServiceSqlQueryMaker::handleTracks( const QStringList &result )
{
    TrackList tracks;
    //SqlRegistry* reg = m_collection->registry();
    int rowCount = ( m_metaFactory->getTrackSqlRowCount() +
                   m_metaFactory->getAlbumSqlRowCount() +
                   m_metaFactory->getArtistSqlRowCount() +
                   m_metaFactory->getGenreSqlRowCount() );

    int resultRows = result.count() / rowCount;

    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );

        TrackPtr trackptr =  m_registry->getTrack( row );
        tracks.append( trackptr );
    }

    emitProperResult<TrackPtr, TrackList>( tracks );
}

void
ServiceSqlQueryMaker::handleArtists( const QStringList &result )
{
    ArtistList artists;
   // SqlRegistry* reg = m_collection->registry();
    int rowCount = m_metaFactory->getArtistSqlRowCount();
    int resultRows = result.size() / rowCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );
        artists.append( m_registry->getArtist( row ) );
    }
    emitProperResult<ArtistPtr, ArtistList>( artists );
}

void
ServiceSqlQueryMaker::handleAlbums( const QStringList &result )
{
    AlbumList albums;
    int rowCount = m_metaFactory->getAlbumSqlRowCount() +  m_metaFactory->getArtistSqlRowCount();
    int resultRows = result.size() / rowCount;

    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );
        albums.append( m_registry->getAlbum( row ) );
    }
    emitProperResult<AlbumPtr, AlbumList>( albums );
}

void
ServiceSqlQueryMaker::handleGenres( const QStringList &result )
{
    GenreList genres;

    int rowCount = m_metaFactory->getGenreSqlRowCount();
    int resultRows = result.size() / rowCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );
        genres.append( m_registry->getGenre( row ) );
    }
    emitProperResult<GenrePtr, GenreList>( genres );
}

/*void
ServiceSqlQueryMaker::handleComposers( const QStringList &result )
{
    ComposerList composers;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        composers.append( reg->getComposer( name, id.toInt() ) );
    }
    emitProperResult<ComposerPtr, ComposerList>( composers );
}

void
ServiceSqlQueryMaker::handleYears( const QStringList &result )
{
    YearList years;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        years.append( reg->getYear( name, id.toInt() ) );
    }
    emitProperResult<YearPtr, YearList>( years );
}*/

QString
ServiceSqlQueryMaker::escape( QString text ) const //krazy2:exclude=constref
{
    return text.replace( '\'', "''" );;
}

QString
ServiceSqlQueryMaker::likeCondition( const QString &text, bool anyBegin, bool anyEnd ) const
{
    if( anyBegin || anyEnd )
    {
        QString escaped = text;
        escaped.replace( '/', "//" ).replace( '%', "/%" ).replace( '_', "/_" );
        escaped = escape( escaped );

        QString ret = " LIKE ";

        ret += '\'';
        if ( anyBegin )
            ret += '%';
        ret += escaped;
        if ( anyEnd )
            ret += '%';
        ret += '\'';

        //Use / as the escape character
        ret += " ESCAPE '/' ";

        return ret;
    }
    else
    {
        return QString( " = '%1' " ).arg( escape( text ) );
    }
}

QueryMaker*
ServiceSqlQueryMaker::beginAnd()
{
    d->queryFilter += andOr();
    d->queryFilter += " ( 1 ";
    d->andStack.push( true );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::beginOr()
{
    d->queryFilter += andOr();
    d->queryFilter += " ( 0 ";
    d->andStack.push( false );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::endAndOr()
{
    d->queryFilter += ')';
    d->andStack.pop();
    return this;
}

QString
ServiceSqlQueryMaker::andOr() const
{
    return d->andStack.top() ? " AND " : " OR ";
}

QueryMaker *
ServiceSqlQueryMaker::setAlbumQueryMode(AlbumQueryMode mode)
{
    d->albumMode = mode;
    return this;
}


#include "ServiceSqlQueryMaker.moc"

