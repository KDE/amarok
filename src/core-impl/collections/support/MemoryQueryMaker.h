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
#include <ThreadWeaver/Job>

namespace ThreadWeaver
{
    class Job;
}

namespace Collections {

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
        MemoryQueryMaker( const QWeakPointer<MemoryCollection> &mc, const QString &collectionId );
        ~MemoryQueryMaker() override;

        void run() override;
        void abortQuery() override;

        QueryMaker* setQueryType( QueryType type ) override;

        QueryMaker* addReturnValue( qint64 value ) override;
        QueryMaker* addReturnFunction( ReturnFunction function, qint64 value ) override;
        QueryMaker* orderBy( qint64 value, bool descending = false ) override;

        QueryMaker* addMatch( const Meta::TrackPtr &track ) override;
        QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists ) override;
        QueryMaker* addMatch( const Meta::AlbumPtr &album ) override;
        QueryMaker* addMatch( const Meta::ComposerPtr &composer ) override;
        QueryMaker* addMatch( const Meta::GenrePtr &genre ) override;
        QueryMaker* addMatch( const Meta::YearPtr &year ) override;
        QueryMaker* addMatch( const Meta::LabelPtr &label ) override;

        QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd ) override;
        QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd ) override;

        QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare ) override;
        QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare ) override;

        QueryMaker* limitMaxResultSize( int size ) override;

        QueryMaker* beginAnd() override;
        QueryMaker* beginOr() override;
        QueryMaker* endAndOr() override;

        QueryMaker* setAlbumQueryMode( AlbumQueryMode mode ) override;
        QueryMaker* setLabelQueryMode( LabelQueryMode mode ) override;

    private Q_SLOTS:
        void done( ThreadWeaver::JobPointer job );

    protected:

        QWeakPointer<MemoryCollection> m_collection;
        struct Private;
        Private * const d;
};

} //namespace Collections

#endif
