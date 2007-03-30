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
#ifndef AMAROK_COLLECTION_SQLQUERYBUILDER_H
#define AMAROK_COLLECTION_SQLQUERYBUILDER_H

#include "querybuilder.h"

class SqlCollection;

class SqlQueryBuilder : public QueryBuilder
{
    public:
        SqlQueryBuilder( SqlCollection* collection );
        virtual ~SqlQueryBuilder();

        virtual QueryBuilder* reset();

        virtual QueryBuilder* startTrackQuery();
        virtual QueryBuilder* startArtistQuery();
        virtual QueryBuilder* startAlbumQuery();
        virtual QueryBuilder* startGenreQuery();
        virtual QueryBuilder* startComposerQuery();
        virtual QueryBuilder* startYearQuery();
        virtual QueryBuilder* startCustomQuery();

        virtual QueryBuilder* includeCollection( const QString &collectionId );
        virtual QueryBuilder* excludeCollection( const QString &collectionId );

        virtual QueryBuilder* addMatch( const TrackPtr &track );
        virtual QueryBuilder* addMatch( const ArtistPtr &artist );
        virtual QueryBuilder* addMatch( const AlbumPtr &album );
        virtual QueryBuilder* addMatch( const ComposerPtr &composer );
        virtual QueryBuilder* addMatch( const GenrePtr &genre );
        virtual QueryBuilder* addMatch( const YearPtr &year );

        virtual QueryBuilder* addFilter( qint64 value, const QString &filter );
        virtual QueryBuilder* excludeFilter( qint64 value, const Qstring &filter );

        virtual QueryBuilder* addReturnValue( qint64 value );
        virtual QueryBuilder* orderBy( qint64 value, bool descending = false );

    private:
        SqlCollection *m_collection;

        class Private;
        Private * const d;
};

#endif /* AMAROK_COLLECTION_SQLQUERYBUILDER_H */
