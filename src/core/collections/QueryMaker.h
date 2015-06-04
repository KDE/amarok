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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_COLLECTION_QUERYMAKER_H
#define AMAROK_COLLECTION_QUERYMAKER_H

#include "core/amarokcore_export.h"
#include "core/meta/forward_declarations.h"
#include "core/meta/support/MetaConstants.h"

#include <QObject>
#include <QStringList>
#include <QtGlobal>

namespace Collections {

class AMAROK_CORE_EXPORT QueryMaker : public QObject
{
    Q_OBJECT

    public:
        enum AlbumQueryMode {
            AllAlbums,
            OnlyCompilations ,
            OnlyNormalAlbums
        };

        enum LabelQueryMode {
            NoConstraint,
            OnlyWithLabels,
            OnlyWithoutLabels
        };

        enum ArtistMatchBehaviour {
            TrackArtists,
            AlbumArtists,
            AlbumOrTrackArtists
        };

        /**
         * Filters that the QueryMaker accepts for searching.
         * not all implementations will accept all filter levels, so make it possible to
         * specify which ones make sense for a given qm. Add to this as needed
         */
        enum ValidFilters {
            TitleFilter = 1,
            AlbumFilter = 2,
            ArtistFilter = 4,
            AlbumArtistFilter = 8,
            GenreFilter = 16,
            ComposerFilter = 32,
            YearFilter = 64,
            UrlFilter = 128,
            AllFilters = 65535
        };

        enum ReturnFunction {
            Count,
            Sum,
            Max,
            Min
        };

        enum NumberComparison {
            Equals,
            GreaterThan,
            LessThan
        };

        enum QueryType {
            None, // Set to faciliate using this in subclasses
            Track,
            Artist,
            Album,
            AlbumArtist,
            Genre,
            Composer,
            Year,
            Custom,
            Label
        };
        QueryMaker();
        virtual ~QueryMaker();

        /**
         *  starts the query. This method returns immediately. All processing is done in one or more
         *  separate worker thread(s). One of the newResultReady signals will be emitted at least once,
         *  followed by the queryDone() signal exactly once.
         */
        virtual void run() = 0;
        /**
         *  aborts a running query. Calling this method aborts a running query as soon as possible.
         *  This method returns immediately. No signals will be emitted after calling this method.
         *  This method has no effect if no query is running.
         */
        virtual void abortQuery() = 0;

        /**
         * Sets the type of objects the querymaker will query for.  These are mutually
         * exclusive.  The results of the query will be returned as objects of the
         * appropriate type, therefore it is necessary to connect the client to the
         * newResultReady( Meta::Type ) signal
         *
         * if you set QueryType custom, this starts a custom query. Unlike other query types, you have to set up the return
         * values yourself using addReturnValue( qint64 ) and addReturnFunction(). The results will
         * be returned as a QStringList. Threfore you have to connect to the
         * newResultReady( QStringList ) signal to receive the results.
         * @return this
         */
        virtual QueryMaker* setQueryType( QueryType type ) = 0;

        /**
          * only works after starting a custom query with setQueryType( Custom )
          * Use this to inform the query maker you are looking for results of value @param value.
          * @return this
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

        virtual QueryMaker* addMatch( const Meta::TrackPtr &track ) = 0;
        /**
         * Match given artist. Depending on @param behaviour matches:
         *   track artist if TrackArtists is given,
         *   album artist if AlbumArtists is given,
         *   any of track or album artist if AlbumOrTrackArtists is given.
         *
         * By default matches only track artist.
         */
        virtual QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists ) = 0;
        virtual QueryMaker* addMatch( const Meta::AlbumPtr &album ) = 0;
        virtual QueryMaker* addMatch( const Meta::ComposerPtr &composer ) = 0;
        virtual QueryMaker* addMatch( const Meta::GenrePtr &genre ) = 0;
        virtual QueryMaker* addMatch( const Meta::YearPtr &year ) = 0;
        virtual QueryMaker* addMatch( const Meta::LabelPtr &label );

        /**
         * Add a filter of type @p value and value @p filter. The querymaker applies this to all queries.
         * @param text the text to match
         * @param matchBegin If set then wildcard match the beginning of @p text (*text)
         * @param matchEnd If set then wildcard match the end of @p text (text*)
         * @return this
         */
        virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false ) = 0;
        /**
         * Exclude filter of type @p value and value @p filter. The querymaker applies this to all queries.
         * @param text the text to match
         * @param matchBegin If set then wildcard match the beginning of @p text (*text)
         * @param matchEnd If set then wildcard match the end of @p text (text*)
         * @return this
         */
        virtual QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false ) = 0;

        virtual QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare ) = 0;
        virtual QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare ) = 0;

        /**
         *  limit the maximum number of items in a result. the result will have [0..@p size ] items. When this function
         *  is not used, the result size is unbounded. Note: the maximum size applies to each result individually, so if
         *  the newResultReady signal is emitted multiple times, each result may have up to @p size items.
         */
        virtual QueryMaker* limitMaxResultSize( int size ) = 0;

        /**
         * select the mode for querying albums. If this method is not called,
         * QueryMaker defaults to AlbumQueryMode::AllAlbums.
         */
        virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );

        /**
          * Sets the label query mode. This method restricts a query to tracks
          * that have labels assigned to them, no labels assigned to them, or no constraint.
          * The default is no constraint.
          * @param mode The LabelQueryMode that will be used by the query.
          * @see LabelQueryMode
          */
        virtual QueryMaker* setLabelQueryMode( LabelQueryMode mode );

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

    Q_SIGNALS:
        /**
         * newResultReady will be emitted every time new results from the query maker are received.
         * This signal can be emitted zero times (in case of no results) one (the usual case) or multiple times
         * (e.g. in case when the result is received in several batches).
         * The results will be terminated by a queryDone signal.
         */
        void newResultReady( Meta::TrackList );
        void newResultReady( Meta::ArtistList );
        void newResultReady( Meta::AlbumList );
        void newResultReady( Meta::GenreList );
        void newResultReady( Meta::ComposerList );
        void newResultReady( Meta::YearList );
        void newResultReady( QStringList );
        void newResultReady( Meta::LabelList );
        void newResultReady( Meta::DataList );

        /**
         * This signal is emitted after all the results have been submitted via zero or more newResultReady signals.
         */
        void queryDone();
};

} //namespace Collections

Q_DECLARE_METATYPE( Collections::QueryMaker* )

#endif /* AMAROK_COLLECTION_QUERYMAKER_H */

