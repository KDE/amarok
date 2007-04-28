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
#ifndef AMAROK_COLLECTION_METAQUERYBUILDER_H
#define AMAROK_COLLECTION_METAQUERYBUILDER_H

#include "querymaker.h"
#include "collection.h"

#include <QList>
#include <QMutex>

using namespace Meta;

class MetaQueryBuilder : public QueryMaker
{
    Q_OBJECT

    public:
        MetaQueryBuilder( const QList<Collection*> &collections);
        ~MetaQueryBuilder();

        virtual QueryMaker* reset();
        virtual void run();
        virtual void abortQuery();

        virtual QueryMaker* startTrackQuery();
        virtual QueryMaker* startArtistQuery();
        virtual QueryMaker* startAlbumQuery();
        virtual QueryMaker* startGenreQuery();
        virtual QueryMaker* startComposerQuery();
        virtual QueryMaker* startYearQuery();
        virtual QueryMaker* startCustomQuery();

        virtual QueryMaker* returnResultAsDataPtrs( bool resultAsDataPtrs );

        virtual QueryMaker* addReturnValue( qint64 value);
        virtual QueryMaker* orderBy( qint64 value, bool descending = false );

        virtual QueryMaker* addMatch( const TrackPtr &track );
        virtual QueryMaker* addMatch( const ArtistPtr &artist );
        virtual QueryMaker* addMatch( const AlbumPtr &album );
        virtual QueryMaker* addMatch( const ComposerPtr &composer );
        virtual QueryMaker* addMatch( const GenrePtr &genre );
        virtual QueryMaker* addMatch( const YearPtr &year );
        virtual QueryMaker* addMatch( const DataPtr &data );

        virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );
        virtual QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );

        virtual QueryMaker* includeCollection( const QString &collectionId );
        virtual QueryMaker* excludeCollection( const QString &collectionId );

        virtual QueryMaker* limitMaxResultSize( int size );

    private slots:
        void slotQueryDone();

    private:
        QList<QueryMaker*> builders;
        int m_queryDoneCount;
        QMutex m_queryDoneCountMutex;

};

#endif /* AMAROK_COLLECTION_METAQUERYBUILDER_H */
