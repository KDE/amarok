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
#ifndef AMAROK_COLLECTION_QUERYBUILDER_H
#define AMAROK_COLLECTION_QUERYBUILDER_H

#include <QObject>

class QueryBuilder : public QObject
{
    public:
        virtual QueryBuilder* reset() = 0;
        virtual void run() = 0;
        virtual void abortQuery() = 0;

        virtual QueryBuilder* startTrackQuery() = 0;
        virtual QueryBuilder* startArtistQuery() = 0;
        virtual QueryBuilder* startAlbumQuery() = 0;
        virtual QueryBuilder* startGenreQuery() = 0;
        virtual QueryBuilder* startComposerQuery() = 0;
        virtual QueryBuilder* startYearQuery() = 0;
        virtual QueryBuilder* startCustomQuery() = 0;

        /**
            only works after starting a custom query with startCustomQuery()
          */
        virtual QueryBuilder* addReturnValue() = 0;
        virtual QueryBuilder* orderBy() = 0;
        
}

#endif /* AMAROK_COLLECTION_QUERYBUILDER_H */

