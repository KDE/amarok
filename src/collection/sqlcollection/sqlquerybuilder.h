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

#include "querymaker.h"

class SqlCollection;

class SqlQueryBuilder : public QueryMaker
{
    Q_OBJECT

    public:
        SqlQueryBuilder( SqlCollection* collection );
        virtual ~SqlQueryBuilder();

        virtual QueryMaker* reset();
        virtual void abortQuery();
        virtual void run();

        virtual QueryMaker* startTrackQuery();
        virtual QueryMaker* startArtistQuery();
        virtual QueryMaker* startAlbumQuery();
        virtual QueryMaker* startGenreQuery();
        virtual QueryMaker* startComposerQuery();
        virtual QueryMaker* startYearQuery();
        virtual QueryMaker* startCustomQuery();

        virtual QueryMaker* includeCollection( const QString &collectionId );
        virtual QueryMaker* excludeCollection( const QString &collectionId );

        virtual QueryMaker* addMatch( const TrackPtr &track );
        virtual QueryMaker* addMatch( const ArtistPtr &artist );
        virtual QueryMaker* addMatch( const AlbumPtr &album );
        virtual QueryMaker* addMatch( const ComposerPtr &composer );
        virtual QueryMaker* addMatch( const GenrePtr &genre );
        virtual QueryMaker* addMatch( const YearPtr &year );

        virtual QueryMaker* addFilter( qint64 value, const QString &filter );
        virtual QueryMaker* excludeFilter( qint64 value, const QString &filter );

        virtual QueryMaker* addReturnValue( qint64 value );
        virtual QueryMaker* orderBy( qint64 value, bool descending = false );

        QString query();
        QStringList runQuery( const QString &query );
        void handleResult( const QStringList &result );

    private:

        void linkTables();
        void buildQuery();

        SqlCollection *m_collection;

        class Private;
        Private * const d;

};

#endif /* AMAROK_COLLECTION_SQLQUERYBUILDER_H */
