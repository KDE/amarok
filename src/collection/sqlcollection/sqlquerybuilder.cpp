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

struct SqlQueryBuilder::Private
{
    enum QueryType { NONE, TRACK, ARTIST, ALBUM, COMPOSER, YEAR, GENRE, CUSTOM };
    enum { TAGS = 1, ARTIST = 2, ALBUM = 4, GENRE = 8, COMPOSER = 16, YEAR = 32, STATISTICS = 64 };
    int linkedTables;
    QueryType queryType;
    Qstring query;
    QString queryReturnValues;
    QString queryFrom;
    QString queryWhere;
    bool included = true;
    bool collectionRestriction = false;
};

SqlQueryBuilder::SqlQueryBuilder( SqlCollection* collection )
    : QueryBuilder()
    , m_collection( collection )
    , d( new Private )
{
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
    d->linkedTable = 0;
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
        d_queryType = Private::ARTIST;
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
    if( !d->collectionRestrictions )
    {
        d->included = false;
        d->collectionRestrictions = true;
    }
    if( m_collection->collectionId() == collectionId )
        d->included = true;
    return this;
}

QueryBuilder*
SqlQueryBuilder::excludeCollection( const QString &collectionId )
{
    d->collectionRestriction = true;
    if( m_collection->collectionId() == collectionId )
        d->included = false;
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
    d->linkedTables |= Private::ARTIST;
    d->queryWhere += QString( " AND artist.name = '%1'" ).arg( artist->name() );
    return this;
}

QueryBuilder*
SqlQueryBuilder::addMatch( const AlbumPtr &album )
{
    d->linkedTables |= Private::ALBUM;
    //handle compilations
    d->queryWhere += QString( " AND album.name = '%1'" ).arg( album.name() );
    return this;
}

QueryBuilder*
SqlQueryBuilder::addMatch( const GenrePtr &genre )
{
    d->linkedTables |= Private::GENRE;
    d->queryWhere += QString( " AND genre.name = '%1'" ).arg( genre.name() );
    return this;
}

QueryBuilder*
SqlQueryBuilder::addMatch( const ComposerPtr &composer )
{
    d->linkedTables |= Private::COMPOSER;
    d->queryWhere += QString( " AND composer.name = '%1'" ).arg( composer.name() );
    return this;
}

QueryBuilder*
SqlQueryBuilder::addMatch( const YearPtr &year )
{
    d->linkedTables |= Private::YEAR;
    d->queryWhere += QString( " AND year.name = '%1'" ).arg( year.name() );
    return this;
}

QueryBuilder*
SqlQueryBuilder::addFilter( qint64 value, const QString &filter )
{
    //TODO
    return this;
}

QueryBuilder*
SqlQueryBuilder::addFilter( qint64 value, const QString &filter )
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
SqlQueryBuilder::orderBy( qint64 value, bool descending = false )
{
    //TODO
    return this;
}

