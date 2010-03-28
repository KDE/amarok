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
 
#ifndef COLLECTION_METAQUERYMAKER_H
#define COLLECTION_METAQUERYMAKER_H

#include "core/collections/QueryMaker.h"
#include "core/collections/Collection.h"

#include <QList>
#include <QMutex>

namespace Collections {

class AMAROK_EXPORT MetaQueryMaker : public QueryMaker
{
    Q_OBJECT

    public:
        MetaQueryMaker( const QList<Collections::Collection*> &collections );
        MetaQueryMaker( const QList<QueryMaker*> &queryMakers );
        ~MetaQueryMaker();

        virtual QueryMaker* reset();
        virtual void run();
        virtual void abortQuery();
        virtual int resultCount() const;

        virtual QueryMaker* setQueryType( QueryType type );

        virtual QueryMaker* setReturnResultAsDataPtrs( bool resultAsDataPtrs );

        virtual QueryMaker* addReturnValue( qint64 value);
        virtual QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );
        virtual QueryMaker* orderBy( qint64 value, bool descending = false );
        virtual QueryMaker* orderByRandom();

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

        virtual QueryMaker* includeCollection( const QString &collectionId );
        virtual QueryMaker* excludeCollection( const QString &collectionId );

        virtual QueryMaker* limitMaxResultSize( int size );

        virtual QueryMaker* beginAnd();
        virtual QueryMaker* beginOr();
        virtual QueryMaker* endAndOr();

        virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );

    private slots:
        void slotQueryDone();

    private:
        QList<QueryMaker*> builders;
        int m_queryDoneCount;
        QMutex m_queryDoneCountMutex;

};

} //namespace Collections

#endif /* COLLECTION_METAQUERYMAKER_H */
