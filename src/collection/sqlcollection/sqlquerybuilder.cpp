/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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
#include "sqlquerybuilder.h"

#define DEBUG_PREFIX "SqlQueryBuilder"

#include "debug.h"

#include "mountpointmanager.h"
#include "sqlcollection.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

class SqlWorkerThread : public Job
{
    public:
        SqlWorkerThread( SqlQueryBuilder *queryMaker )
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
        SqlQueryBuilder *m_queryMaker;

        bool m_aborted;
};

struct SqlQueryBuilder::Private
{
    enum QueryType { NONE, TRACK, ARTIST, ALBUM, COMPOSER, YEAR, GENRE, CUSTOM };
    enum { TAGS_TAB = 1, ARTIST_TAB = 2, ALBUM_TAB = 4, GENRE_TAB = 8, COMPOSER_TAB = 16, YEAR_TAB = 32, STATISTICS_TAB = 64 };
    int linkedTables;
    QueryType queryType;
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
    SqlWorkerThread *worker;
};

SqlQueryBuilder::SqlQueryBuilder( SqlCollection* collection )
    : QueryMaker()
    , m_collection( collection )
    , d( new Private )
{
    d->includedBuilder = true;
    d->collectionRestriction = false;
    d->worker = 0;
    reset();
}

SqlQueryBuilder::~SqlQueryBuilder()
{
    delete d;
}

QueryMaker*
SqlQueryBuilder::reset()
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
    d->resultAsDataPtrs = false;
    d->withoutDuplicates = false;
    d->maxResultSize = -1;
    return this;
}

void
SqlQueryBuilder::abortQuery()
{
    if( d->worker )
        d->worker->requestAbort();
}

QueryMaker*
SqlQueryBuilder::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->resultAsDataPtrs = resultAsDataPtrs;
    return this;
}

void
SqlQueryBuilder::run()
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
        d->worker = new SqlWorkerThread( this );
        connect( d->worker, SIGNAL( done( Job* ) ), SLOT( done( Job* ) ) );
        ThreadWeaver::Weaver::instance()->enqueue( d->worker );
    }
}

void
SqlQueryBuilder::done( Job *job )
{
    Weaver::instance()->dequeue( job );
    job->deleteLater();
    d->worker = 0;
    emit queryDone();
}

QueryMaker*
SqlQueryBuilder::startTrackQuery()
{
    DEBUG_BLOCK
    //make sure to keep this method in sync with handleTracks(QStringList) and the SqlTrack ctor
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::TRACK;
        d->linkedTables |= Private::TAGS_TAB;
        d->linkedTables |= Private::GENRE_TAB;
        d->linkedTables |= Private::ARTIST_TAB;
        d->linkedTables |= Private::ALBUM_TAB;
        d->linkedTables |= Private::COMPOSER_TAB;
        d->linkedTables |= Private::YEAR_TAB;
        d->linkedTables |= Private::STATISTICS_TAB;
        d->queryReturnValues =  "tags.deviceid, tags.url, "
                                "tags.title, tags.comment, "
                                "tags.track, tags.discnumber, "
                                "statistics.percentage, statistics.rating, "
                                "tags.bitrate, tags.length, "
                                "tags.filesize, tags.samplerate, "
                                "statistics.createdate, statistics.accessdate, "
                                "statistics.playcounter, tags.filetype, tags.bpm, "
                                "artist.name, artist.id, "
                                "album.name, album.id, tags.sampler, "
                                "genre.name, genre.id, "
                                "composer.name, composer.id, "
                                "year.name, year.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startArtistQuery()
{
    DEBUG_BLOCK
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::ARTIST;
        d->withoutDuplicates = true;
        d->linkedTables |= Private::ARTIST_TAB;
        //reading the ids from the database means we don't have to query for them later
        d->queryReturnValues = "artist.name, artist.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startAlbumQuery()
{
    DEBUG_BLOCK
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::ALBUM;
        d->withoutDuplicates = true;
        d->linkedTables |= Private::ALBUM_TAB;
        //add whatever is necessary to identify compilations
        d->queryReturnValues = "album.name, album.id, tags.sampler";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startComposerQuery()
{
    DEBUG_BLOCK
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::COMPOSER;
        d->withoutDuplicates = true;
        d->linkedTables |= Private::COMPOSER_TAB;
        d->queryReturnValues = "composer.name, composer.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startGenreQuery()
{
    DEBUG_BLOCK
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::GENRE;
        d->withoutDuplicates = true;
        d->linkedTables |= Private::GENRE_TAB;
        d->queryReturnValues = "genre.name, genre.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startYearQuery()
{
    DEBUG_BLOCK
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::YEAR;
        d->withoutDuplicates = true;
        d->linkedTables |= Private::YEAR_TAB;
        d->queryReturnValues = "year.name, year.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startCustomQuery()
{
    DEBUG_BLOCK
    if( d->queryType == Private::NONE )
        d->queryType = Private::CUSTOM;
    return this;
}

QueryMaker*
SqlQueryBuilder::includeCollection( const QString &collectionId )
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
SqlQueryBuilder::excludeCollection( const QString &collectionId )
{
    d->collectionRestriction = true;
    if( m_collection->collectionId() == collectionId )
        d->includedBuilder = false;
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const TrackPtr &track )
{
    QString url = track->url();
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );
    d->queryMatch += QString( " AND tags.deviceid = %1 AND tags.url = '%2'" )
                        .arg( QString::number( deviceid ), escape( rpath ) );
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const ArtistPtr &artist )
{
    d->linkedTables |= Private::ARTIST_TAB;
    d->queryMatch += QString( " AND artist.name = '%1'" ).arg( escape( artist->name() ) );
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const AlbumPtr &album )
{
    d->linkedTables |= Private::ALBUM_TAB;
    //handle compilations
    d->queryMatch += QString( " AND album.name = '%1'" ).arg( escape( album->name() ) );
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const GenrePtr &genre )
{
    d->linkedTables |= Private::GENRE_TAB;
    d->queryMatch += QString( " AND genre.name = '%1'" ).arg( escape( genre->name() ) );
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const ComposerPtr &composer )
{
    d->linkedTables |= Private::COMPOSER_TAB;
    d->queryMatch += QString( " AND composer.name = '%1'" ).arg( escape( composer->name() ) );
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const YearPtr &year )
{
    d->linkedTables |= Private::YEAR_TAB;
    d->queryMatch += QString( " AND year.name = '%1'" ).arg( escape( year->name() ) );
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const DataPtr &data )
{
    ( const_cast<DataPtr&>(data) )->addMatchTo( this );
    return this;
}

QueryMaker*
SqlQueryBuilder::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    //TODO
    return this;
}

QueryMaker*
SqlQueryBuilder::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    //TODO
    return this;
}

QueryMaker*
SqlQueryBuilder::addReturnValue( qint64 value )
{
    if( d->queryType == Private::CUSTOM )
    {
        if ( !d->queryReturnValues.isEmpty() )
            d->queryReturnValues += ',';
        d->queryReturnValues += nameForValue( value );
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::orderBy( qint64 value, bool descending )
{
    if ( d->queryOrderBy.isEmpty() )
        d->queryOrderBy = " ORDER BY ";
    d->queryOrderBy += nameForValue( value );
    d->queryOrderBy += QString( " %1 " ).arg( descending ? "DESC" : "ASC" );
    return this;
}

QueryMaker*
SqlQueryBuilder::limitMaxResultSize( int size )
{
    d->maxResultSize = size;
    return this;
}

void
SqlQueryBuilder::linkTables()
{
    d->linkedTables |= Private::TAGS_TAB; //HACK!!!
    //assuming that tags is always included for now
    if( !d->linkedTables )
        return;

    if( d->linkedTables & Private::TAGS_TAB )
        d->queryFrom += " tags";
    if( d->linkedTables & Private::ARTIST_TAB )
        d->queryFrom += " LEFT JOIN artist ON tags.artist = artist.id";
    if( d->linkedTables & Private::ALBUM_TAB )
        d->queryFrom += " LEFT JOIN album ON tags.album = album.id";
    if( d->linkedTables & Private::GENRE_TAB )
        d->queryFrom += " LEFT JOIN genre ON tags.genre = genre.id";
    if( d->linkedTables & Private::COMPOSER_TAB )
        d->queryFrom += " LEFT JOIN composer ON tags.composer = composer.id";
    if( d->linkedTables & Private::YEAR_TAB )
        d->queryFrom += " LEFT JOIN year ON tags.year = year.id";
    if( d->linkedTables & Private::STATISTICS_TAB )
        d->queryFrom += " LEFT JOIN statistics ON tags.deviceid = statistics.deviceid AND tags.url = statistics.url";
}

void
SqlQueryBuilder::buildQuery()
{
    linkTables();
    QString query = "SELECT ";
    if ( d->withoutDuplicates )
        query += "DISTINCT ";
    query += d->queryReturnValues;
    query += " FROM ";
    query += d->queryFrom;
    query += " WHERE 1 ";
    query += d->queryMatch;
    query += d->queryFilter;
    query += d->queryOrderBy;
    if ( d->maxResultSize > -1 )
        query += QString( " LIMIT %1 OFFSET 0 " ).arg( d->maxResultSize );
    query += ';';
    d->query = query;
}

QString
SqlQueryBuilder::query()
{
    if ( d->query.isEmpty() )
        buildQuery();
    return d->query;
}

QStringList
SqlQueryBuilder::runQuery( const QString &query )
{
    return m_collection->query( query );
}

void
SqlQueryBuilder::handleResult( const QStringList &result )
{
    DEBUG_BLOCK
    debug() << "Result length: " << result.count() << endl;
    if( !result.isEmpty() )
    {
        switch( d->queryType ) {
        case Private::CUSTOM:
            emit newResultReady( m_collection->collectionId(), result );
            break;
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
        case Private::COMPOSER:
            handleComposers( result );
            break;
        case Private::YEAR:
            handleYears( result );
            break;

        case Private::NONE:
            debug() << "Warning: queryResult with queryType == NONE" << endl;
        }
    }

    //queryDone will be emitted in done(Job*)
}

QString
SqlQueryBuilder::nameForValue( qint64 value )
{
    switch( value )
    {
        case valUrl:
            d->linkedTables |= Private::TAGS_TAB;
            return "tags.url";  //TODO figure out how to handle deviceid
        case valTitle:
            d->linkedTables |= Private::TAGS_TAB;
            return "tags.title";
        case valArtist:
            d->linkedTables |= Private::ARTIST_TAB;
            return "artist.name";
        case valAlbum:
            d->linkedTables |= Private::ALBUM_TAB;
            return "album.name";
        case valGenre:
            d->linkedTables |= Private::GENRE_TAB;
            return "genre.name";
        case valComposer:
            d->linkedTables |= Private::COMPOSER_TAB;
            return "composer.name";
        case valYear:
            d->linkedTables |= Private::YEAR_TAB;
            return "year.name";
        case valComment:
            d->linkedTables |= Private::TAGS_TAB;
            return "tags.comment";
        case valTrackNr:
            d->linkedTables |= Private::TAGS_TAB;
            return "tags.track";
        case valDiscNr:
            d->linkedTables |= Private::TAGS_TAB;
            return "tags.discnumber";
        case valLength:
            d->linkedTables |= Private::TAGS_TAB;
            return "tags.length";
        case valBitrate:
            d->linkedTables |= Private::TAGS_TAB;
            return "tags.bitrate";
        case valSamplerate:
            d->linkedTables |= Private::TAGS_TAB;
            return "tags.samplerate";
        case valFilesize:
            d->linkedTables |= Private::TAGS_TAB;
            return "tags.filesize";
        case valFormat:
            d->linkedTables |= Private::TAGS_TAB;
            return "tags.filetype";
        case valScore:
            d->linkedTables |= Private::STATISTICS_TAB;
            return "statistics.percentage";
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
            return "statistics.playcounter";
        default:
            return "ERROR: unknown value in SqlQueryBuilder::nameForValue(qint64): value=" + value;
    }
}

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
SqlQueryBuilder::handleTracks( const QStringList &result )
{
    DEBUG_BLOCK
    TrackList tracks;
    SqlRegistry* reg = m_collection->registry();
    //there are 28 columns in the result set as generated by startTrackQuery()
    int resultRows = result.size() / 28;
    for( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid( i*28, 28 );
        tracks.append( reg->getTrack( row ) );
    }
    emitProperResult( TrackPtr, tracks );
}

void
SqlQueryBuilder::handleArtists( const QStringList &result )
{
    DEBUG_BLOCK
    ArtistList artists;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        artists.append( reg->getArtist( name, id.toInt() ) );
    }
    emitProperResult( ArtistPtr, artists );
}

void
SqlQueryBuilder::handleAlbums( const QStringList &result )
{
    DEBUG_BLOCK
    AlbumList albums;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        albums.append( reg->getAlbum( name, id.toInt() ) );
        iter.next(); //contains tags.isCompilation, not handled at the moment
    }
    debug() << "Returning " << albums.count() << " albums" << endl;
    emitProperResult( AlbumPtr, albums );
}

void
SqlQueryBuilder::handleGenres( const QStringList &result )
{
    GenreList genres;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        QString name = iter.next();
        QString id = iter.next();
        genres.append( reg->getGenre( name, id.toInt() ) );
    }
    emitProperResult( GenrePtr, genres );
}

void
SqlQueryBuilder::handleComposers( const QStringList &result )
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
SqlQueryBuilder::handleYears( const QStringList &result )
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
}

QString
SqlQueryBuilder::escape( QString text ) const           //krazy:exclude=constref
{
    return text.replace( '\'', "''" );;
}

QString
SqlQueryBuilder::likeCondition( const QString &text, bool anyBegin, bool anyEnd ) const
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

#include "sqlquerybuilder.moc"

