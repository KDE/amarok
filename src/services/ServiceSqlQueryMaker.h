/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_SERVICESQLQUERYMAKER_H
#define AMAROK_SERVICESQLQUERYMAKER_H

#include "ServiceMetaBase.h"
#include "ServiceSqlRegistry.h"
#include "QueryMaker.h"

#include <threadweaver/Job.h>

class ServiceSqlCollection;

class ServiceSqlQueryMaker : public QueryMaker
{
    Q_OBJECT

    public:
       ServiceSqlQueryMaker( ServiceSqlCollection* collection, ServiceMetaFactory * metaFactory, ServiceSqlRegistry * registry );
        virtual ~ServiceSqlQueryMaker();

        virtual QueryMaker* reset();
        virtual void abortQuery();
        virtual void run();

        virtual QueryMaker* setQueryType( QueryType type );

        virtual QueryMaker* setReturnResultAsDataPtrs( bool resultAsDataPtrs );

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

        virtual QueryMaker* addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare );
        virtual QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare );

        virtual QueryMaker* addReturnValue( qint64 value );
        virtual QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );
        virtual QueryMaker* orderBy( qint64 value, bool descending = false );
        virtual QueryMaker* orderByRandom();

        virtual QueryMaker* limitMaxResultSize( int size );

        virtual QueryMaker* beginAnd();
        virtual QueryMaker* beginOr();
        virtual QueryMaker* endAndOr();

        virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );

        QString query();
        QStringList runQuery( const QString &query );
        void handleResult( const QStringList &result );

    protected:
        virtual QString escape( QString text ) const;
        virtual QString likeCondition( const QString &text, bool anyBegin, bool anyEnd ) const;

    public slots:
        void done( ThreadWeaver::Job * job );

    private:
        template<class PointerType, class ListType>
        void emitProperResult( const ListType& list );

        void linkTables();
        void buildQuery();

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

#endif /* AMAROK_COLLECTION_SQLQUERYBUILDER_H */
