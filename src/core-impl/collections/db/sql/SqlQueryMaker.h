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

#ifndef AMAROK_COLLECTION_SQLQUERYMAKER_H
#define AMAROK_COLLECTION_SQLQUERYMAKER_H

#include "core/collections/QueryMaker.h"

#include "amarok_sqlcollection_export.h"

#include <threadweaver/Job.h>

namespace Collections {

class SqlCollection;

class AMAROK_SQLCOLLECTION_EXPORT SqlQueryMaker : public QueryMaker
{
    Q_OBJECT

    public:
        SqlQueryMaker( SqlCollection* collection );
        virtual ~SqlQueryMaker();

        virtual void abortQuery();
        virtual void run();

        virtual QueryMaker* setQueryType( QueryType type );

        virtual QueryMaker* addMatch( const Meta::TrackPtr &track );
        virtual QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists );
        virtual QueryMaker* addMatch( const Meta::AlbumPtr &album );
        virtual QueryMaker* addMatch( const Meta::ComposerPtr &composer );
        virtual QueryMaker* addMatch( const Meta::GenrePtr &genre );
        virtual QueryMaker* addMatch( const Meta::YearPtr &year );
        virtual QueryMaker* addMatch( const Meta::LabelPtr &label );

        virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );
        virtual QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );

        virtual QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare );
        virtual QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare );

        virtual QueryMaker* addReturnValue( qint64 value );
        virtual QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );
        virtual QueryMaker* orderBy( qint64 value, bool descending = false );

        virtual QueryMaker* limitMaxResultSize( int size );

        virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );
        virtual QueryMaker* setLabelQueryMode( LabelQueryMode mode );

        virtual QueryMaker* beginAnd();
        virtual QueryMaker* beginOr();
        virtual QueryMaker* endAndOr();

        QString query();
        QStringList runQuery( const QString &query );
        void handleResult( const QStringList &result );

        // for using it blocking (only for collection internal use)

        void setBlocking( bool enabled );

        QStringList collectionIds() const;

        Meta::TrackList tracks() const;
        Meta::AlbumList albums() const;
        Meta::ArtistList artists() const;
        Meta::GenreList genres() const;
        Meta::ComposerList composers() const;
        Meta::YearList years() const;
        QStringList customData() const;
        Meta::LabelList labels() const;

    protected:
        virtual QString escape( QString text ) const;

        /**
         * returns a pattern for LIKE operator that will match given text with given options
         * @param text the text to match (should not be escape()'d, function does it itself)
         * @param anyBegin wildcard match the beginning of @p text (*text)
         * @param anyEnd wildcard match the end of @p text (text*)
         */
        virtual QString likeCondition( const QString &text, bool anyBegin, bool anyEnd ) const;

    public Q_SLOTS:
        void done( ThreadWeaver::Job * job );
        void blockingNewResultReady( const QStringList &customData );
        void blockingNewResultReady( const Meta::AlbumList &albums );
        void blockingNewResultReady( const Meta::ArtistList &artists );
        void blockingNewResultReady( const Meta::GenreList &genres );
        void blockingNewResultReady( const Meta::ComposerList &composers );
        void blockingNewResultReady( const Meta::YearList &years );
        void blockingNewResultReady( const Meta::TrackList &tracks );
        void blockingNewResultReady( const Meta::LabelList &labels );

    private:

        void linkTables();
        void buildQuery();

        QString nameForValue( qint64 value );
        QString andOr() const;

        SqlCollection *m_collection;

        struct Private;
        Private * const d;

};

class SqlQueryMakerFactory
{
public:
    virtual SqlQueryMaker* createQueryMaker() const = 0;
    virtual ~SqlQueryMakerFactory() {};
};

} //namespace Collections

#endif /* AMAROK_COLLECTION_SQLQUERYMAKER_H */
