/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MEMORYQUERYMAKER_H
#define MEMORYQUERYMAKER_H

#include "amarok_export.h"

#include "MemoryCollection.h"
#include "core/collections/QueryMaker.h"

#include <QWeakPointer>


namespace ThreadWeaver
{
    class Job;
}

class AMAROK_EXPORT MemoryQueryMaker : public QueryMaker
{
    Q_OBJECT
    public:
    /**
      * Creates a new MemoryQueryMaker that will query a memory collection.
      * This class implements the QueryMaker interface and can be used as a generic
      * query maker for all collections that use MemoryCollection.
      * @param mc the MemoryCollection instance that the query should be run on.
      * @param collectionId the collectionid that has to be emitted by this querymaker.
      */
        MemoryQueryMaker( QWeakPointer<MemoryCollection> mc, const QString &collectionId );
        virtual ~MemoryQueryMaker();

        virtual QueryMaker* reset();
        virtual void run();
        virtual void abortQuery();

        virtual QueryMaker* setQueryType( QueryType type );

        virtual QueryMaker* setReturnResultAsDataPtrs( bool resultAsDataPtrs );

        virtual QueryMaker* addReturnValue( qint64 value );
        virtual QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );
        virtual QueryMaker* orderBy( qint64 value, bool descending = false );
        virtual QueryMaker* orderByRandom();

        virtual QueryMaker* includeCollection( const QString &collectionId );
        virtual QueryMaker* excludeCollection( const QString &collectionId );

        virtual QueryMaker* addMatch( const Meta::TrackPtr &track );
        virtual QueryMaker* addMatch( const Meta::ArtistPtr &artist );
        virtual QueryMaker* addMatch( const Meta::AlbumPtr &album );
        virtual QueryMaker* addMatch( const Meta::ComposerPtr &composer );
        virtual QueryMaker* addMatch( const Meta::GenrePtr &genre );
        virtual QueryMaker* addMatch( const Meta::YearPtr &year );
        virtual QueryMaker* addMatch( const Meta::DataPtr &data );

        virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );
        virtual QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );

        virtual QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare );
        virtual QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare );

        virtual QueryMaker* limitMaxResultSize( int size );

        virtual QueryMaker* beginAnd();
        virtual QueryMaker* beginOr();
        virtual QueryMaker* endAndOr();

        virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );

    private slots:
        void done( ThreadWeaver::Job * job );

    protected:

        QWeakPointer<MemoryCollection> m_collection;
        struct Private;
        Private * const d;
};


#endif
