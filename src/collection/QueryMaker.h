/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_COLLECTION_QUERYMAKER_H
#define AMAROK_COLLECTION_QUERYMAKER_H

#include "amarok_export.h"
#include "meta/Meta.h"
#include "meta/MetaConstants.h"

#include <QObject>
#include <QStringList>
#include <QtGlobal>

class AMAROK_EXPORT QueryMaker : public QObject
{
    Q_OBJECT

    public:
        enum AlbumQueryMode { AllAlbums = 0
                              , OnlyCompilations = 1
                              , OnlyNormalAlbums = 2 };

        //Filters that the QueryMaker accepts for searching.
        //not all implementations will accept all filter levels, so make it possible to
        //specify which ones make sense for a given qm. Add to this as needed
        enum ValidFilters { TitleFilter     =     1,
                            AlbumFilter     =     2,
                            ArtistFilter    =     4,
                            GenreFilter     =     8,
                            ComposerFilter  =    16,
                            YearFilter      =    32,
                            AllFilters      = 65535 };

        enum ReturnFunction { Count = 0,
                              Sum = 1,
                              Max = 2,
                              Min = 3 };

        enum NumberComparison { Equals = 0,
                               GreaterThan = 1,
                               LessThan = 2 };

        enum QueryType { None = 0, // Set to faciliate using this in subclasses 
                         Track,
                         Artist,
                         Album,
                         Genre,
                         Composer,
                         Year,
                         Custom
        };
        QueryMaker();
        virtual ~QueryMaker();

        /**
            resets all internal data to the default values. Calling this method is the same
            as creating a new QueryMaker.
            @return this
        */
        virtual QueryMaker* reset() = 0;
        /**
            starts the query. This method returns immediately. All processing is done in one or more
            separate worker thread(s). One of the newResultReady signals will be emitted at least once,
            followed by the queryDone() signal exactly once.
        */
        virtual void run() = 0;
        /**
            aborts a running query. Calling this method aborts a running query as soon as possible. This method returns immediately. No signals will be emitted after calling this method. This
            method has no effect if no query is running.
        */
        virtual void abortQuery() = 0;

        /**
            returns the number of times one of the newResultReady signals will be emitted
            by the QueryMaker instance.
         */
        virtual int resultCount() const;

        /**
         * Sets the type of objects the querymaker will query for.  These are mutually
         * exclusive.  The results of the query will be returned as objects of the
         * appropriate type, therefore it is necessary to connect the client to the
         * newResultReady( QString, Meta::Type ) signal (unless you are after Meta::Data
         * pointers, see setReturnResultAsDataPtrs( bool ) for details.
         *
         * if you set QueryType custom, this starts a custom query. Unlike other query types, you have to set up the return
         * values yourself using addReturnValue( qint64 ) and addReturnFunction(). The results will
         * be returned as a QStringList. Threfore you have to connect to the
         * newResultReady( QString, QStringList ) signal to receive the results. Calling
         * setReturnResultAsDataPtrs( bool ) has no effect when using a custom query.
         * @return this
         */
        virtual QueryMaker* setQueryType( QueryType type ) = 0;
        /**
            sets the QueryMaker instance to return Meta::Data objects instead of the actual type.
            In some cases it can be useful to ignore the actual type of the result and just work with
            the method provided by Meta::Data. Calling this method with resultAsDataPtrs = true causes
            the QueryMaker instance to emit the newResultReady( QString, Meta::DataList ) signal
            for all query types (except a custom query). Calling the method with resultAsDataPtrs = false
            switches back to the normal behaviour.

            @return this
        */
        virtual QueryMaker* setReturnResultAsDataPtrs( bool resultAsDataPtrs ) = 0;

        /**
            only works after starting a custom query with startCustomQuery()
            Use this to inform the query maker you are looking for results of value @param value.
            @return this
          */
        virtual QueryMaker* addReturnValue( qint64 value ) = 0;
        /**
         * Returns the output of the function specified by function.
         * Only works after starting a custom query
         * @return this
         */
        virtual QueryMaker* addReturnFunction( ReturnFunction function, qint64 value ) = 0;
        /**
         * Return results sorted by @p value.
         * @return this
         */
        virtual QueryMaker* orderBy( qint64 value, bool descending = false ) = 0;
        /**
         * return results in a random order.
         * @return this
         */
        virtual QueryMaker* orderByRandom() = 0;

        virtual QueryMaker* includeCollection( const QString &collectionId ) = 0;
        virtual QueryMaker* excludeCollection( const QString &collectionId ) = 0;

        virtual QueryMaker* addMatch( const Meta::TrackPtr &track ) = 0;
        virtual QueryMaker* addMatch( const Meta::ArtistPtr &artist ) = 0;
        virtual QueryMaker* addMatch( const Meta::AlbumPtr &album ) = 0;
        virtual QueryMaker* addMatch( const Meta::ComposerPtr &composer ) = 0;
        virtual QueryMaker* addMatch( const Meta::GenrePtr &genre ) = 0;
        virtual QueryMaker* addMatch( const Meta::YearPtr &year ) = 0;
        virtual QueryMaker* addMatch( const Meta::DataPtr &data ) = 0;

        /**
            Add a filter of type @p value and value @p filter. The querymaker applies this to all queries.
            @return this
        */
        virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false ) = 0;
        /**
            Exclude @p filter of type @p value from all queries made by this query maker.
            @return this
        */
        virtual QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false ) = 0;

        virtual QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare ) = 0;
        virtual QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare ) = 0;

        /**
            limit the maximum number of items in a result. the result will have [0..@p size ] items. When this function
            is not used, the result size is unbounded. Note: the maximum size applies to each result individually, so if
            the newResultReady signal is emitted multiple times, each result may have up to @p size items.
        */
        virtual QueryMaker* limitMaxResultSize( int size ) = 0;

        /**
         * select the mode for querying albums. If this method is not called,
         * QueryMaker defaults to AlbumQueryMode::AllAlbums.
         */
        virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );

        virtual QueryMaker* beginAnd() = 0;
        virtual QueryMaker* beginOr() = 0;
        virtual QueryMaker* endAndOr() = 0;
                                                     
        /**
         * Choose whether the query maker instance should delete itself after the query.
         * By passing true the query maker instance will delete itself after emitting queryDone().
         * Otherwise it is the responsibility of the owner (the code which called ::queryMaker() usually) to delete the instance
         * when it is not needed anymore.
         *
         * Defaults to false, i.e. the querymaker instance will not delete itself.
         */
        QueryMaker* setAutoDelete( bool autoDelete );

        virtual int validFilterMask();

    signals:
        void newResultReady( QString collectionId, Meta::TrackList );
        void newResultReady( QString collectionId, Meta::ArtistList );
        void newResultReady( QString collectionId, Meta::AlbumList );
        void newResultReady( QString collectionId, Meta::GenreList );
        void newResultReady( QString collectionId, Meta::ComposerList );
        void newResultReady( QString collectionId, Meta::YearList );
        void newResultReady( QString collectionId, Meta::DataList );
        void newResultReady( QString collectionId, QStringList );

        void queryDone();
};

#endif /* AMAROK_COLLECTION_QUERYMAKER_H */

