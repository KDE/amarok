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

#include "sqlcollection.h"
#include "threadmanager.h"

//=================== SqlWorkerThread ===============================
class SqlWorkerThread : public ThreadManager::DependentJob
{
public:

    SqlWorkerThread( SqlQueryBuilder *queryBuilder )
        : DependentJob( queryBuilder, "SqlWorkerThread" )
        , m_queryBuilder( queryBuilder )
    { };

    SqlQueryBuilder *m_queryBuilder;

    virtual bool doJob()
    {
        QString query = m_queryBuilder->query();
        QStringList result = m_queryBuilder->runQuery( query );
        if( !this->isAborted() )
            m_queryBuilder->handleResult( result );

        return !this->isAborted();
    };
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
    QString queryWhere;
    bool includedBuilder;
    bool collectionRestriction;
    SqlWorkerThread *worker;
};

SqlQueryBuilder::SqlQueryBuilder( SqlCollection* collection )
    : QueryMaker()
    , m_collection( collection )
    , d( new Private )
{
    d->includedBuilder = true;
    d->collectionRestriction = false;
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
    d->queryWhere.clear();
    d->linkedTables = 0;
    d->worker = 0;   //ThreadWeaver deletes the Job
}

void
SqlQueryBuilder::abortQuery()
{
    if( d->worker )
        d->worker->abort();
}

void
SqlQueryBuilder::run()
{
    if( d->queryType == Private::NONE )
        return; //better error handling?
    if( d->worker )
    {
        //the worker thread seems to be running
        //TODO: abort Job or do nothing?
    }
    else
    {
        d->worker = new SqlWorkerThread( this );
        ThreadManager::instance()->queueJob( d->worker );
    }
}

QueryMaker*
SqlQueryBuilder::startTrackQuery()
{
    //make sure to keep this method in sync with handleTracks(QStringList) and the SqlTrack ctor
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::TRACK;
        d->queryReturnValues =  "tags.deviceid, tags.url, "
                                "tags.title, tags.comment, "
                                "tags.track, tags.discnumber, "
                                "statistics.percentage, statistics.rating, "
                                "tags.bitrate, tags.length, "
                                "tags.filesize, tags.samplerate, "
                                "statistics.createdate, statistics.accessdate, "
                                "statistics.playcounter, tags.filetype, tags.bpm, "
                                "artist.name, artist.id, "
                                "album.name, album.id, tags.sampler"
                                "genre.name, genre.id, "
                                "composer.name, composer.id, "
                                "year.name, year.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startArtistQuery()
{
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::ARTIST;
        //reading the ids from the database means we don't have to query for them later
        d->queryReturnValues = "artist.name, artist.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startAlbumQuery()
{
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::ALBUM;
        //add whatever is necessary to identify compilations
        d->queryReturnValues = "album.name, album.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startComposerQuery()
{
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::COMPOSER;
        d->queryReturnValues = "composer.name, composer.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startGenreQuery()
{
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::GENRE;
        d->queryReturnValues = "genre.name, genre.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startYearQuery()
{
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::YEAR;
        d->queryReturnValues = "year.name, year.id";
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::startCustomQuery()
{
    if( d->queryType == Private::NONE )
        d->queryType == Private::CUSTOM;
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
    //TODO
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const ArtistPtr &artist )
{
    d->linkedTables |= Private::ARTIST_TAB;
    d->queryWhere += QString( " AND artist.name = '%1'" ).arg( artist->name() );
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const AlbumPtr &album )
{
    d->linkedTables |= Private::ALBUM_TAB;
    //handle compilations
    d->queryWhere += QString( " AND album.name = '%1'" ).arg( album->name() );
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const GenrePtr &genre )
{
    d->linkedTables |= Private::GENRE_TAB;
    d->queryWhere += QString( " AND genre.name = '%1'" ).arg( genre->name() );
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const ComposerPtr &composer )
{
    d->linkedTables |= Private::COMPOSER_TAB;
    d->queryWhere += QString( " AND composer.name = '%1'" ).arg( composer->name() );
    return this;
}

QueryMaker*
SqlQueryBuilder::addMatch( const YearPtr &year )
{
    d->linkedTables |= Private::YEAR_TAB;
    d->queryWhere += QString( " AND year.name = '%1'" ).arg( year->name() );
    return this;
}

QueryMaker*
SqlQueryBuilder::addFilter( qint64 value, const QString &filter )
{
    //TODO
    return this;
}

QueryMaker*
SqlQueryBuilder::excludeFilter( qint64 value, const QString &filter )
{
    //TODO
    return this;
}

QueryMaker*
SqlQueryBuilder::addReturnValue( qint64 value )
{
    if( d->queryType == Private::CUSTOM )
    {
        //TODO
    }
    return this;
}

QueryMaker*
SqlQueryBuilder::orderBy( qint64 value, bool descending )
{
    //TODO
    return this;
}

void
SqlQueryBuilder::linkTables()
{
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
        d->queryFrom += " LEFT JOIN statistics ON tags.deviced = statistics.deviceid AND tags.url = statistics.url";
}

void
SqlQueryBuilder::buildQuery()
{
    linkTables();
    QString query = "SELECT ";
    query += d->queryReturnValues;
    query += " FROM ";
    query += d->queryFrom;
    query += " WHERE 1 ";
    query += d->queryWhere;
    query += ';';
    d->query = query;
}

QString
SqlQueryBuilder::query()
{
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

        //handle default?
        }
    }
    //the worker thread will be deleted by ThreadManager
    d->worker = 0;

    emit queryDone();
}

// What's worse, a bunch of almost identical repeated code, or a not so obvious macro? :-)
// The macro below will emit the proper result signal. If m_resultAsDataPtrs is true,
// it'll emit the signal that takes a list of DataPtrs. Otherwise, it'll call the
// signal that takes the list of the specific class.

#define emitProperResult( PointerType, list ) { \
            if ( m_resultAsDataPtrs ) { \
                DataList data; \
                foreach( PointerType p, list ) { \
                    data << DataPtr::staticCast( p ); \
                } \
                emit newResultReady( m_collection->collectionId(), data ); \
            } \
            emit newResultReady( m_collection->collectionId(), list ); \
        }

void
SqlQueryBuilder::handleTracks( const QStringList &result )
{
    TrackList tracks;
    SqlRegistry* reg = m_collection->registry();
    //there are 28 columns in the result set as generated by startTrackQuery()
    int resultRows = result.size() % 28;
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
    ArtistList artists;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        artists.append( reg->getArtist( iter.next(), iter.next().toInt() ) );
    }
    emitProperResult( ArtistPtr, artists );
}

void
SqlQueryBuilder::handleAlbums( const QStringList &result )
{
    AlbumList albums;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        albums.append( reg->getAlbum( iter.next(), iter.next().toInt() ) );
        iter.next(); //contains tags.isCompilation, not handled at the moment
    }
    emitProperResult( AlbumPtr, albums );
}

void
SqlQueryBuilder::handleGenres( const QStringList &result )
{
    GenreList genres;
    SqlRegistry* reg = m_collection->registry();
    for( QStringListIterator iter( result ); iter.hasNext(); )
    {
        genres.append( reg->getGenre( iter.next(), iter.next().toInt() ) );
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
        composers.append( reg->getComposer( iter.next(), iter.next().toInt() ) );
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
        years.append( reg->getYear( iter.next(), iter.next().toInt() ) );
    }
    emitProperResult( YearPtr, years );
}

#include "sqlquerybuilder.moc"

