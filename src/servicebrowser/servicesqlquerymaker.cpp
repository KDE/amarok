/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *  Copyright (c) 2007 Nikolaj Hald Nielsenn <nhnFreespirit@gmail.com>
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
#include "servicesqlquerymaker.h"

#define DEBUG_PREFIX "SqlQueryBuilder"

#include "debug.h"

#include "mountpointmanager.h"
//#include "servicesqlcollection.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

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
            DEBUG_BLOCK
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
    enum QueryType { NONE, TRACK, ARTIST, ALBUM, GENRE/*, COMPOSER, YEAR, CUSTOM*/ };
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
    bool resultAsDataPtrs;
    bool withoutDuplicates;
    int maxResultSize;
    ServiceSqlWorkerThread *worker;
};

ServiceSqlQueryMaker::ServiceSqlQueryMaker( ServiceSqlCollection* collection, ServiceMetaFactory * metaFactory  )
    : QueryMaker()
    , m_collection( collection )
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
   // d->linkedTables = 0;
    if( d->worker && d->worker->isFinished() )
        delete d->worker;   //TODO error handling
    //d->resultAsDataPtrs = false;
    d->withoutDuplicates = false;
    d->maxResultSize = -1;
    return this;
}

void
ServiceSqlQueryMaker::abortQuery()
{
    if( d->worker )
        d->worker->requestAbort();
}

QueryMaker*
ServiceSqlQueryMaker::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->resultAsDataPtrs = resultAsDataPtrs;
    return this;
}

void
ServiceSqlQueryMaker::run()
{
    DEBUG_BLOCK
    if( d->queryType == Private::NONE )
        return; //better error handling?
    if( d->worker && !d->worker->isFinished() )
    {
        //the worker thread seems to be running
        //TODO: wait or job to complete
        
    }
    else
    {
        debug() << "Query is " << query() << endl;
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
ServiceSqlQueryMaker::startTrackQuery()
{
    DEBUG_BLOCK
    //make sure to keep this method in sync with handleTracks(QStringList) and the SqlTrack ctor
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::TRACK;
        d->linkedTables |= Private::TRACKS_TABLE;
        //d->queryFrom = m_metaFactory->tablePrefix() + "_tracks";
        d->queryReturnValues =  m_metaFactory->getTrackSqlRows();
    }
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::startArtistQuery()
{
    DEBUG_BLOCK
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::ARTIST;
        d->linkedTables |= Private::ARTISTS_TABLE;
        d->withoutDuplicates = true;
        //d->queryFrom = m_metaFactory->tablePrefix() + "_artists";
        d->queryReturnValues = m_metaFactory->getArtistSqlRows();
    }
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::startAlbumQuery()
{
    DEBUG_BLOCK
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::ALBUM;
        d->linkedTables |= Private::ALBUMS_TABLE;
        d->withoutDuplicates = true;
        //d->queryFrom = m_metaFactory->tablePrefix() + "_albums";
        d->queryReturnValues = m_metaFactory->getAlbumSqlRows();
    }
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::startComposerQuery()
{
    DEBUG_BLOCK
   /* if( d->queryType == Private::NONE )
    {
        d->queryType = Private::COMPOSER;
        d->withoutDuplicates = true;
        d->linkedTables |= Private::COMPOSER_TAB;
        d->queryReturnValues = "composer.name, composer.id";
    }*/
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::startGenreQuery()
{
    DEBUG_BLOCK
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::GENRE;
        d->withoutDuplicates = true;
        //d->linkedTables |= Private::ALBUM_TABLE;
        d->linkedTables |= Private::GENRE_TABLE;
        d->queryReturnValues = m_metaFactory->getGenreSqlRows();
        d->queryFilter = " GROUP BY name HAVING COUNT ( name ) > 10 ";
    }
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::startYearQuery()
{
    DEBUG_BLOCK
    /*if( d->queryType == Private::NONE )
    {
        d->queryType = Private::YEAR;
        d->withoutDuplicates = true;
        d->linkedTables |= Private::YEAR_TAB;
        d->queryReturnValues = "year.name, year.id";
    }*/
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::startCustomQuery()
{
    DEBUG_BLOCK
   /* if( d->queryType == Private::NONE )
        d->queryType = Private::CUSTOM;*/
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
    DEBUG_BLOCK

    Q_UNUSED( track );
    //TODO still pondereing this one...
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const ArtistPtr &artist )
{
    DEBUG_BLOCK
    QString prefix = m_metaFactory->tablePrefix(); 
    if (d->queryType == Private::TRACK ) // a service track does not generally know its artist
        return this;
    const ServiceArtist * serviceArtist = dynamic_cast<const ServiceArtist *>( artist.data() );
    d->queryMatch += QString( " AND " + prefix + "_albums.artist_id= '%1'" ).arg( serviceArtist->id() );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const AlbumPtr &album )
{
    DEBUG_BLOCK
    QString prefix = m_metaFactory->tablePrefix(); 
    const ServiceAlbum * serviceAlbum = dynamic_cast<const ServiceAlbum *>( album.data() );
    d->queryMatch += QString( " AND " + prefix + "_tracks.album_id = '%1'" ).arg( serviceAlbum->id() );
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addMatch( const GenrePtr &genre )
{
    DEBUG_BLOCK
    QString prefix = m_metaFactory->tablePrefix(); 

    const ServiceGenre* serviceGenre = dynamic_cast<const ServiceGenre *>( genre.data() );
    d->linkedTables |= Private::GENRE_TABLE;
    if ( d->queryType == Private::ARTIST ) //HACK!
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
    DEBUG_BLOCK
    ( const_cast<DataPtr&>(data) )->addMatchTo( this );
    //TODO needed at all?
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    Q_UNUSED( value );
    Q_UNUSED( filter );
    Q_UNUSED( matchBegin );
    Q_UNUSED( matchEnd );
    //TODO
    return this;
}

QueryMaker*
ServiceSqlQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    Q_UNUSED( value );
    Q_UNUSED( filter );
    Q_UNUSED( matchBegin );
    Q_UNUSED( matchEnd );
    //TODO
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
ServiceSqlQueryMaker::orderBy( qint64 value, bool descending )
{
    Q_UNUSED( value );
    if ( d->queryOrderBy.isEmpty() )
    d->queryOrderBy = " ORDER BY name "; //TODO FIX!!
    d->queryOrderBy += QString( " %1 " ).arg( descending ? "DESC" : "ASC" );
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
    //d->linkedTables |= Private::TAGS_TAB; //HACK!!!
    //assuming that tags is always included for now
    if( !d->linkedTables )
        return;

    QString prefix = m_metaFactory->tablePrefix();

    if( d->linkedTables & Private::TRACKS_TABLE )
        d->queryFrom += " " + prefix + "_tracks";
    if( d->linkedTables & Private::ALBUMS_TABLE )
       d->queryFrom += " " + prefix + "_albums";
    if( d->linkedTables & Private::ARTISTS_TABLE )
       d->queryFrom += " " + prefix + "_artists";

    //There must be a better way....
    if( d->linkedTables & Private::GENRE_TABLE ) {
        if ( d->queryType == Private::GENRE ) {
            d->queryFrom += " " + prefix + "_genre";
        } else if ( d->queryType == Private::ARTIST ) {

             d->queryFrom += " LEFT JOIN " + prefix + "_albums ON " + prefix + "_albums.artist_id = " + prefix + "_artists.id";
             d->queryFrom += " LEFT JOIN " + prefix + "_genre ON " + prefix + "_genre.album_id = " + prefix + "_albums.id";

        } else {
            //HACK! for now only allow genre matches on artists
       }
    }

}

void
ServiceSqlQueryMaker::buildQuery()
{
    linkTables();
    QString query = "SELECT ";
    if ( d->withoutDuplicates )
        query += "DISTINCT ";
    query += d->queryReturnValues;
    query += " FROM ";
    query += d->queryFrom;
    query += " WHERE 1 "; // oh... to not have to bother with the leadig "AND"
    query += d->queryMatch;
    query += d->queryFilter;
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
    DEBUG_BLOCK
    debug() << "Query string: " << query << endl;
    return m_collection->query( query );
}

void
ServiceSqlQueryMaker::handleResult( const QStringList &result )
{
    DEBUG_BLOCK
    debug() << "Result length: " << result.count() << endl;
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
            debug() << "Warning: queryResult with queryType == NONE" << endl;
        }
    }

    //queryDone will be emitted in done(Job*)
}

/*QString
ServiceSqlQueryMaker::nameForValue( qint64 value )
{
    switch( value )
    {
        case valUrl:

            return "tags.url";  //TODO figure out how to handle deviceid
        case valTitle:

            return "tags.title";
        case valArtist:

            return "artist.name";
        case valAlbum:

            return "album.name";
        case valGenre:
   
            return "genre.name";
        case valComposer:


            return "statistics.playcounter";
        default:
            return "ERROR: unknown value in SqlQueryBuilder::nameForValue(qint64): value=" + value;
    }
}*/

// What's worse, a bunch of almost identical repeated code, or a not so obvious macro? :-)
// The macro below will emit the proper result signal. If m_resultAsDataPtrs is true,
// it'll emit the signal that takes a list of DataPtrs. Otherwise, it'll call the
// signal that takes the list of the specific class.

#define emitProperResult( PointerType, list ) { \
            if ( d->resultAsDataPtrs ) { \
                DataList data; \
                foreach( PointerType p, list ) { \
                    data << DataPtr::staticCast( p ); \
                } \
                emit newResultReady( m_collection->collectionId(), data ); \
            } \
            else { \
                emit newResultReady( m_collection->collectionId(), list ); \
            } \
        }

void
ServiceSqlQueryMaker::handleTracks( const QStringList &result )
{
    DEBUG_BLOCK
    TrackList tracks;
    //SqlRegistry* reg = m_collection->registry();
    //there are 28 columns in the result set as generated by startTrackQuery()
    int rowCount = m_metaFactory->getTrackSqlRowCount();
    int resultRows = result.size() / rowCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );
        tracks.append( m_metaFactory->createTrack( row ) );
    }
    emitProperResult( TrackPtr, tracks );
}

void
ServiceSqlQueryMaker::handleArtists( const QStringList &result )
{
    DEBUG_BLOCK
    ArtistList artists;
   // SqlRegistry* reg = m_collection->registry();
    int rowCount = m_metaFactory->getArtistSqlRowCount();
    int resultRows = result.size() / rowCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );

        artists.append( m_metaFactory->createArtist( row ) );
    }
    emitProperResult( ArtistPtr, artists );
}

void
ServiceSqlQueryMaker::handleAlbums( const QStringList &result )
{
    DEBUG_BLOCK
    AlbumList albums;
    // SqlRegistry* reg = m_collection->registry();
    int rowCount = m_metaFactory->getAlbumSqlRowCount();
    int resultRows = result.size() / rowCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );
        albums.append( m_metaFactory->createAlbum( row ) );
    }
    emitProperResult( AlbumPtr, albums );
}

void
ServiceSqlQueryMaker::handleGenres( const QStringList &result )
{
    DEBUG_BLOCK
    GenreList genres;

    int rowCount = m_metaFactory->getGenreSqlRowCount();
    int resultRows = result.size() / rowCount;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*rowCount, rowCount );
        genres.append( m_metaFactory->createGenre( row ) );
    }
    emitProperResult( GenrePtr, genres );
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
    emitProperResult( ComposerPtr, composers );
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
    emitProperResult( YearPtr, years );
}*/

QString
ServiceSqlQueryMaker::escape( QString text ) const           //krazy:exclude=constref
{
    return text.replace( '\'', "''" );;
}

QString
ServiceSqlQueryMaker::likeCondition( const QString &text, bool anyBegin, bool anyEnd ) const
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

#include "servicesqlquerymaker.moc"

