/*
 *  Copyright (c) 2006-2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#include "querybuilder.h"
#include "collection.h"

#include <QList>
#include <QMutex>

class MetaQueryBuilder : public QueryBuilder
{
    public:
        MetaQueryBuilder( const QList<Collection*> &collections);
        ~MetaQueryBuilder();

        virtual QueryBuilder* reset();
        virtual void run();
        virtual void abortQuery();

        virtual QueryBuilder* startTrackQuery();
        virtual QueryBuilder* startArtistQuery();
        virtual QueryBuilder* startAlbumQuery();
        virtual QueryBuilder* startGenreQuery();
        virtual QueryBuilder* startComposerQuery();
        virtual QueryBuilder* startYearQuery();
        virtual QueryBuilder* startCustomQuery();

        virtual QueryBuilder* addReturnValue();
        virtual QueryBuilder* orderBy();

        virtual QueryBuilder* includeCollection( const QString &collectionId );
        virtual QueryBuilder* excludeCollection( const QString &collectionId );

    private slots:
        void slotQueryDone();

    private:
        QList<QueryBuilder*> builders;
        int m_queryDoneCount;
        QMutex m_queryDoneCountMutex;

};

#endif /* AMAROK_COLLECTION_METAQUERYBUILDER_H */
