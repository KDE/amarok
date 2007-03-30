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
    : QueryBuilder()
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

QueryBuilder*
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

QueryBuilder*
SqlQueryBuilder::startTrackQuery()
{
    if( d->queryType == Private::NONE )
        d->queryType = Private::TRACK;
    return this;
}

QueryBuilder*
SqlQueryBuilder::startArtistQuery()
{
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::ARTIST;
        //reading the ids from the database means we don't have to query for them later
        d->queryReturnValues = "artist.id, artist.name";
    }
    return this;
}

QueryBuilder*
SqlQueryBuilder::startAlbumQuery()
{
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::ALBUM;
        //add whatever is necessary to identify compilations
        d->queryReturnValues = "album.id, album.name";
    }
    return this;
}

QueryBuilder*
SqlQueryBuilder::startComposerQuery()
{
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::COMPOSER;
        d->queryReturnValues = "composer.id, composer.name";
    }
    return this;
}

QueryBuilder*
SqlQueryBuilder::startGenreQuery()
{
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::GENRE;
        d->queryReturnValues = "genre.id, genre.name";
    }
    return this;
}

QueryBuilder*
SqlQueryBuilder::startYearQuery()
{
    if( d->queryType == Private::NONE )
    {
        d->queryType = Private::YEAR;
        d->queryReturnValues = "year.id, year.name";
    }
    return this;
}

QueryBuilder*
SqlQueryBuilder::startCustomQuery()
{
    if( d->queryType == Private::NONE )
        d->queryType == Private::CUSTOM;
    return this;
}

QueryBuilder*
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

QueryBuilder*
SqlQueryBuilder::excludeCollection( const QString &collectionId )
{
    d->collectionRestriction = true;
    if( m_collection->collectionId() == collectionId )
        d->includedBuilder = false;
    return this;
}

QueryBuilder*
SqlQueryBuilder::addMatch( const TrackPtr &track )
{
    //TODO
    return this;
}

QueryBuilder*
SqlQueryBuilder::addMatch( const ArtistPtr &artist )
{
    d->linkedTables |= Private::ARTIST_TAB;
    d->queryWhere += QString( " AND artist.name = '%1'" ).arg( artist->name() );
    return this;
}

QueryBuilder*
SqlQueryBuilder::addMatch( const AlbumPtr &album )
{
    d->linkedTables |= Private::ALBUM_TAB;
    //handle compilations
    d->queryWhere += QString( " AND album.name = '%1'" ).arg( album->name() );
    return this;
}

QueryBuilder*
SqlQueryBuilder::addMatch( const GenrePtr &genre )
{
    d->linkedTables |= Private::GENRE_TAB;
    d->queryWhere += QString( " AND genre.name = '%1'" ).arg( genre->name() );
    return this;
}

QueryBuilder*
SqlQueryBuilder::addMatch( const ComposerPtr &composer )
{
    d->linkedTables |= Private::COMPOSER_TAB;
    d->queryWhere += QString( " AND composer.name = '%1'" ).arg( composer->name() );
    return this;
}

QueryBuilder*
SqlQueryBuilder::addMatch( const YearPtr &year )
{
    d->linkedTables |= Private::YEAR_TAB;
    d->queryWhere += QString( " AND year.name = '%1'" ).arg( year->name() );
    return this;
}

QueryBuilder*
SqlQueryBuilder::addFilter( qint64 value, const QString &filter )
{
    //TODO
    return this;
}

QueryBuilder*
SqlQueryBuilder::excludeFilter( qint64 value, const QString &filter )
{
    //TODO
    return this;
}

QueryBuilder*
SqlQueryBuilder::addReturnValue( qint64 value )
{
    if( d->queryType == Private::CUSTOM )
    {
        //TODO
    }
    return this;
}

QueryBuilder*
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

            //TODO
        }
    }
    //the worker thread will be deleted by ThreadManager
    d->worker = 0;

    emit queryDone();
}

#include "sqlquerybuilder.moc"

