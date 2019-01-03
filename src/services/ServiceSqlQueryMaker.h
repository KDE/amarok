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

#ifndef AMAROK_SERVICESQLQUERYMAKER_H
#define AMAROK_SERVICESQLQUERYMAKER_H

#include "ServiceMetaBase.h"
#include "ServiceSqlRegistry.h"
#include "core/collections/QueryMaker.h"

#include <QObject>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Queue>

namespace Collections {

class ServiceSqlCollection;

class ServiceSqlQueryMaker : public QueryMaker
{
    Q_OBJECT

    public:
       ServiceSqlQueryMaker( ServiceSqlCollection* collection, ServiceMetaFactory * metaFactory, ServiceSqlRegistry * registry );
        virtual ~ServiceSqlQueryMaker();

        void abortQuery() override;
        void run() override;

        QueryMaker* setQueryType( QueryType type ) override;

        QueryMaker* addMatch( const Meta::TrackPtr &track ) override;
        QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists ) override;
        QueryMaker* addMatch( const Meta::AlbumPtr &album ) override;
        QueryMaker* addMatch( const Meta::ComposerPtr &composer ) override;
        QueryMaker* addMatch( const Meta::GenrePtr &genre ) override;
        QueryMaker* addMatch( const Meta::YearPtr &year ) override;
        QueryMaker* addMatch( const Meta::LabelPtr &label ) override;

        QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd ) override;
        QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd ) override;

        QueryMaker* addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare ) override;
        QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare ) override;

        QueryMaker* addReturnValue( qint64 value ) override;
        QueryMaker* addReturnFunction( ReturnFunction function, qint64 value ) override;
        QueryMaker* orderBy( qint64 value, bool descending = false ) override;

        QueryMaker* limitMaxResultSize( int size ) override;

        QueryMaker* beginAnd() override;
        QueryMaker* beginOr() override;
        QueryMaker* endAndOr() override;

        QueryMaker* setAlbumQueryMode( AlbumQueryMode mode ) override;

        QString query();
        QStringList runQuery( const QString &query );
        void handleResult( const QStringList &result );

    protected:
        virtual QString escape( QString text ) const;
        virtual QString likeCondition( const QString &text, bool anyBegin, bool anyEnd ) const;

    public Q_SLOTS:
        void done( ThreadWeaver::JobPointer job );

    private:
        template<class PointerType, class ListType>
        void emitProperResult( const ListType& list );

        void linkTables();
        void buildQuery();

        bool isValidValue( qint64 value );
        QString nameForValue( qint64 value );
        QString andOr() const;

        void handleTracks( const QStringList &result );
        void handleArtists( const QStringList &result );
        void handleAlbums( const QStringList &result );
        void handleGenres( const QStringList &result );
        //void handleComposers( const QStringList &result );
        //void handleYears( const QStringList &result );

        ServiceSqlCollection *m_collection;
        ServiceSqlRegistry * m_registry;
        ServiceMetaFactory * m_metaFactory;

        struct Private;
        Private * const d;
};

} //namespace Collections

#endif /* AMAROK_COLLECTION_SQLQUERYBUILDER_H */
